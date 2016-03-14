#include "mbus.h"

extern struct mbus_serial_func ser_func;

static struct frm_para * set_item(struct argu_ser_table *table)
{
	struct frm_para *sfpara;

	sfpara = (struct frm_para *)malloc(sizeof(struct frm_para));
	if(sfpara == NULL){
		printf("Malloc faied ! %s(%d)\n", __func__, __LINE__);
		exit(0);
	}
	
	printf("RTU Modbus Slave mode !\n");
	sfpara->slvID = table->slvID;
	sfpara->straddr = 0;

	switch(table->fc){
		case 1:
			sfpara->fc = READCOILSTATUS;
			sfpara->len = table->len;	// TODO: chk len
			break;
		case 2:
			sfpara->fc = READINPUTSTATUS;
			sfpara->len = table->len;	// TODO: chk len
			break;
		case 3:
			sfpara->fc = READHOLDINGREGS;
			sfpara->len = table->len;	// TODO: chk len
			break;
		case 4:
			sfpara->fc = READINPUTREGS;
			sfpara->len = table->len;	// TODO: chk len
			break;
		case 5:
			sfpara->fc = FORCESIGLEREGS;
			/*if(table->act == 1 || table->act == 0){
				sfpara->act = table->act << 8;
			}else{
				printf("Force Single Coil, the status only 1/0\n");
				return NULL;
			}*/
			break;
		case 6:
			sfpara->fc = PRESETEXCPSTATUS;
		//	sfpara->act = table->act;
			break;
		default:
			printf("For now, only support function code :\n");
			printf("1		Read Coil Status\n");
			printf("2		Read Input Status\n");
			printf("3		Read Holding Registers\n");
			printf("4		Read Input Registers\n");
			printf("5		Force Single Coil\n");
			printf("6		Preset Single Register\n");
			return NULL;
	}
		
	return sfpara;
}
int _choose_resp_frm(unsigned char *tx_buf, struct frm_para *sfpara, int ret)
{
	int txlen = 0;

	if(!ret){
		switch(sfpara->fc){
			case READCOILSTATUS:
				txlen = ser_func.build_0102_resp(tx_buf, sfpara, READCOILSTATUS);
				break;
			case READINPUTSTATUS:
				txlen = ser_func.build_0102_resp(tx_buf, sfpara, READINPUTSTATUS);
				break;
			case READHOLDINGREGS:
				txlen = ser_func.build_0304_resp(tx_buf, sfpara, READHOLDINGREGS);
				break;
			case READINPUTREGS:
				txlen = ser_func.build_0304_resp(tx_buf, sfpara, READINPUTREGS);
				break;
			case FORCESIGLEREGS:
				txlen = ser_func.build_0506_resp(tx_buf, sfpara, FORCESIGLEREGS);
				break;
			case PRESETEXCPSTATUS:
				txlen = ser_func.build_0506_resp(tx_buf, sfpara, PRESETEXCPSTATUS);
				break;
			default:
				printf("<Modbus Serial Slave> unknown function code : %d\n", sfpara->fc);
				sleep(2);
				return -1;
		}
	}else if(ret == -1){
		return -1;
	}else if(ret == -2){
		txlen = ser_func.build_excp(tx_buf, sfpara, EXCPILLGFUNC);
		print_data(tx_buf, txlen, SENDEXCP);
	}else if(ret == -3){
		txlen = ser_func.build_excp(tx_buf, sfpara, EXCPILLGDATAVAL);
		print_data(tx_buf, txlen, SENDEXCP); 
	}else if(ret == -4){
		txlen = ser_func.build_excp(tx_buf, sfpara, EXCPILLGDATAADDR);
		print_data(tx_buf, txlen, SENDEXCP); 
	}

	return txlen;
}

int _set_termios(int fd, struct termios2 *newtio)
{
	int ret;

	ret = ioctl(fd, TCGETS2, newtio);
	if(ret < 0){
		printf("<Modbus Serial Slave> ioctl : %s\n", strerror(errno));
		return -1;
	}
	printf("<Modbus Serial Slave> BEFORE setting : ospeed %d ispeed %d\n", newtio->c_ospeed, newtio->c_ispeed);
	newtio->c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|IXOFF|IXANY|PARMRK);
	newtio->c_iflag |= (IGNBRK|IGNPAR);
	newtio->c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio->c_cflag &= ~CBAUD;
	newtio->c_cflag |= BOTHER;
	/*
	 * Close termios 'OPOST' flag, thus when write serial data
	 * which first byte is '0a', it will not add '0d' automatically !! 
	 * Becaues of '\n' in ASCII is '0a' and '\r' in ASCII is '0d'.
	 * On usual, \n\r is linked, if you only send 0a(\n), 
	 * Kernel will think you forget \r(0d), so add \r(0d) automatically.                       
	*/
	newtio->c_oflag &= ~(OPOST);
	newtio->c_ospeed = 9600;
	newtio->c_ispeed = 9600;
	ret = ioctl(fd, TCSETS2, newtio);
	if(ret < 0){
		printf("<Modbus Serial Slave> ioctl : %s\n", strerror(errno));
		return -1;
	}
	printf("<Modbus Serial Slave> AFTER setting : ospeed %d ispeed %d\n", newtio->c_ospeed, newtio->c_ispeed);
	
	return 0;
}

void mbus_ser_slv(struct argu_ser_table *table, void *pQtClass)
{
	int fd;
	int retval;
	int ret = 0;
	int txlen;
	char *port_name;
	int rlock;
	int wlock;
	fd_set wfds;
	fd_set rfds;
	struct termios2 newtio;
	struct timeval tv;
	unsigned char tx_buf[FRMLEN];
	unsigned char rx_buf[FRMLEN];
	struct frm_para *sfpara = NULL;	
	struct res_disp_table res_table;	// passing to QT
	QtClass = pQtClass;

	port_name = table->port_name;

	sfpara = set_item(table);
	if(sfpara == NULL){
		printf("<Modbus Serial Slave> Set item fail\n");
		exit(0);
	}

	fd = open(port_name, O_RDWR);
	if(fd == -1){
		printf("<Modbus Serial Slave> open : %s\n", strerror(errno));
		exit(0);
	}
	printf("<Modbus Serial Slave> Com port fd = %d\n", fd);

	if(_set_termios(fd, &newtio) == -1){
		printf("<Modbus Serial Slave> set termios fail\n");
		exit(0);
	} 
	
	rlock = 0;
	wlock = 0;
	res_table.round = 0;
	res_table.mode = SERIAL_SLAVE;

	do{
		FD_ZERO(&wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &wfds);
		FD_SET(fd, &rfds);		
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		retval = select(fd+1, &rfds, &wfds, 0, &tv);
		if(retval == 0){
			printf("<Modbus Serial Slave> Watting for query\n");
			sleep(3);
			continue;
		}
		/* Recv Query */
		if(FD_ISSET(fd, &rfds) && !rlock){
			_readAll(fd, rx_buf, SERRECVQRYLEN);
			res_table.rx_buf = rx_buf;
			res_table.rxlen = SERRECVQRYLEN;
			ret = ser_chk_dest(rx_buf, sfpara);
			if(ret == -1){
				memset(rx_buf, 0, FRMLEN);
				continue;
			}
			ret = ser_query_parser(rx_buf, sfpara);
			rlock = 1;
		}
		/* Send Respond */
		if(FD_ISSET(fd, &wfds) && !wlock && rlock){
			txlen = _choose_resp_frm(tx_buf, sfpara, ret);
			if(txlen == -1){
				rlock = 0;
				wlock = 0;
				continue;
			}
			_writeAll(fd, tx_buf, txlen);
			res_table.tx_buf = tx_buf;
			res_table.txlen = txlen;	
			poll_slvID(sfpara->slvID);
			wlock = 1;
		}
		/* Result */	
		if(rlock == 1 && wlock == 1){
			res_table.round ++;
			c_connector(&res_table, pQtClass);
			debug_print_data(rx_buf, SERRECVQRYLEN, RECVQRY);
			debug_print_data(tx_buf, txlen, SENDRESP);
			rlock = 0;
			wlock = 0;
		}
	}while(isRunning);
	
	close(fd);
	free(sfpara);
	
}

