#include "mbus.h"

extern struct mbus_serial_func ser_func;

static struct frm_para * set_item(struct argu_ser_table *table)
{
	struct frm_para *mfpara;
	
	mfpara = (struct frm_para *)malloc(sizeof(struct frm_para));
	if(mfpara == NULL){
		printf("Malloc Failed ! %s(%d)\n", __func__, __LINE__);
		exit(0);
	}
	printf("RTU Master mode !\n");
	mfpara->slvID = table->slvID;
	mfpara->straddr = 1;	// force 
	switch(table->fc){
		case 1:
			mfpara->fc = READCOILSTATUS;
			mfpara->len = table->len;	// TODO: check len
			break;
		case 2:
			mfpara->fc = READINPUTSTATUS;	
			mfpara->len = table->len;	// TODO: check len
			break;
		case 3:
			mfpara->fc = READHOLDINGREGS;
			mfpara->len = table->len;	// TODO: check len
			break;
		case 4:
			mfpara->fc = READINPUTREGS;
			mfpara->len = table->len;	// TODO: check len	
			break;
		case 5:
			mfpara->fc = FORCESIGLEREGS;
			if(table->act == 1){ 
				mfpara->act = 0xff00;
			}else if(table->act == 0){
				mfpara->act = 0;
			}else{
				printf("Force Single Coil, the status only 1/0\n");
				return NULL;
			}
			break;
		case 6:
			mfpara->fc = PRESETEXCPSTATUS;
			mfpara->act = table->act;
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

	return mfpara;
}

int _set_termois(int fd, struct termios2 *newtio)
{
	int ret;

	ret = ioctl(fd, TCGETS2, newtio);
	if(ret < 0){
		printf("<Modbus Serial Master> ioctl : %s\n", strerror(errno));
		return -1;
	}
	printf("<Modbus Serial Master> BEFORE setting : ospeed %d ispeed %d\n", newtio->c_ospeed, newtio->c_ispeed);
	newtio->c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|IXOFF|PARMRK);
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
		printf("<Modbus Serial Master> ioctl : %s\n", strerror(errno));
		return -1;
	}
	printf("<Modbus Serial Master> AFTER setting : ospeed %d ispeed %d\n", newtio->c_ospeed, newtio->c_ispeed);
	
	return 0;
}

void mbus_ser_mstr(struct argu_ser_table *table, void *pQtClass)
{	
	int fd;
	int retval;
	int ret;
	int rlen;
	int txlen;
	int rlock, wlock;
	char *port_name;
	fd_set wfds;
	fd_set rfds;
	struct termios2 newtio;
	struct timeval tv;
	unsigned char tx_buf[FRMLEN];
	unsigned char rx_buf[FRMLEN];
	struct frm_para *mfpara = NULL;
	struct res_disp_table res_table;
	struct timespec t_write, t_read;
	
	port_name = table->port_name;

	mfpara = set_item(table);
	if(mfpara == NULL){
		printf("<Modbus Serial Master> Set parameters fail\n");
		exit(0);
	}
	
	fd = open(port_name, O_RDWR);
	if(fd == -1){
		printf("<Modbus Serial Master> open : %s\n", strerror(errno));
		exit(0);
	}
	printf("<Modbus Serial Master> Com port fd = %d\n", fd);

	if(_set_termois(fd, &newtio) == -1){
		printf("<Modbus Serial Master> Set termios fail\n");
		exit(0);
	} 

	txlen = ser_func.build_qry(tx_buf, mfpara);
	print_data(tx_buf, txlen, SENDQRY);

	rlock = 0;
	wlock = 0;
	rlen = 0;
	res_table.round = 0;
	res_table.mode = SERIAL_MASTER;
	res_table.tx_buf = tx_buf;
	res_table.txlen = txlen;

	do{
		FD_ZERO(&wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &wfds);
		FD_SET(fd, &rfds);		
		
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		retval = select(fd+1, &rfds, &wfds, 0, &tv);
		if(retval <= 0){
			printf("<Modbus Serial Master> Select nothing\n");
			sleep(3);
			continue;
		}

		/* Send Query */
		if(FD_ISSET(fd, &wfds) && !wlock && !rlock){
			sleep(1);
			_writeAll(fd, tx_buf, txlen);
			clock_gettime(CLOCK_REALTIME, &t_write);
			wlock = 1;
		}
		/* Recv Respond */
		if(FD_ISSET(fd, &rfds) && !rlock && wlock){
			if(ioctl(fd, FIONREAD, &rlen) == -1){
				printf("%s(%d): ioctl failed!\n", __func__, __LINE__);
				break;
			}
			_readAll(fd, rx_buf, rlen);
			clock_gettime(CLOCK_REALTIME, &t_read);
			res_table.rx_buf = rx_buf;
			res_table.rxlen = rlen;
			ret = ser_func.chk_dest(rx_buf, mfpara);
			if(ret == -1){
				memset(rx_buf, 0, FRMLEN);
				continue;
			}
			ret = ser_func.resp_parser(rx_buf, mfpara, rlen);
			if(ret == -1){
				print_data(rx_buf, rlen, RECVEXCP);
			}
			poll_slvID(mfpara->slvID);
			rlock = 1;
		}
		/* Result */
		if(rlock == 1 && wlock == 1){
			res_table.resp_time = _get_resp_time(&t_read, &t_write);
			//printf("Response time %f usec\n", res_table.resp_time);
			res_table.round ++;
			c_connector(&res_table, pQtClass);
			debug_print_data(rx_buf, rlen, RECVRESP);
			rlock = 0;
			wlock = 0;
		}
	}while(isRunning);

	printf("%s is closing...\n", __func__);
	close(fd);
	free(mfpara);
}
