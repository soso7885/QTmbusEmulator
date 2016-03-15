/*
 * TCP modbus Master, send modbus query.:v
*/
#include "mbus.h"

extern struct mbus_tcp_func tcp_func;

static struct tcp_frm_para * set_item(struct argu_tcp_table *table)
{
	struct tcp_frm_para *tmfpara;

	tmfpara = (struct tcp_frm_para *)malloc(sizeof(struct tcp_frm_para));
	if(tmfpara == NULL){
		printf("Malloc failed !%s(%d)\n", __func__, __LINE__);
		exit(0);
	}

	printf("Modbus TCP Master mode !\n");
	tmfpara->transID = INITTCPTRANSID;
	tmfpara->potoID = (unsigned char)TCPMBUSPROTOCOL;
	tmfpara->msglen = (unsigned char)TCPQUERYMSGLEN;
	tmfpara->unitID = table->unitID;
	tmfpara->straddr = 1;	// force start address = 0
	switch(table->fc){
		case 1:
			tmfpara->fc = READCOILSTATUS;
			tmfpara->len = table->len;	// TODO: check len
			break;
		case 2:
			tmfpara->fc = READINPUTSTATUS;
			tmfpara->len = table->len;	// TODO: check len
			break;
		case 3:
			tmfpara->fc = READHOLDINGREGS;
			tmfpara->len = table->len;	// TODO: check len(0-110)
			break;
		case 4:
			tmfpara->fc = READINPUTREGS;
			tmfpara->len = table->len;	// TODO: check len(0-110)
			break;
		case 5:
			tmfpara->fc = FORCESIGLEREGS;
			if(table->act == 1){
				tmfpara->act = 0xff00;
			}else if(table->act == 0){
				tmfpara->act = 0;	
			}else{
				printf("Force Single Coil, the status only 1/0\n");
				return NULL;
			}
			break;
		case 6:
			tmfpara->fc = PRESETEXCPSTATUS;
			tmfpara->act = table->act;
			break;
		default:
			printf("For now, only support function code :\n");
			printf("1        Read Coil Status\n");
			printf("2        Read Input Status\n");
			printf("3        Read Holding Registers\n");
			printf("4        Read Input Registers\n");
			printf("5        Force Single Coil\n");
			printf("6        Preset Single Register\n");
			return NULL;
	}
	
	return tmfpara;
}

int _create_sk_cli(char *addr, char *port)
{
	int skfd;
	int ret;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *p;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	ret = getaddrinfo(addr, port, &hints, &res);
	if(ret != 0){
		printf("<Modbus Tcp Master> getaddrinfo : %s\n", gai_strerror(ret));
		exit(0);
	}

	for(p = res; p != NULL; p = p->ai_next){
		skfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(skfd == -1){
			continue;
		}
		printf("<Modbus Tcp Master> fd = %d\n", skfd);

		ret = connect(skfd, p->ai_addr, p->ai_addrlen);
		if(ret == -1){
			close(skfd);
			continue;
		}
		break;
	}

	if(p == NULL){
		printf("<Modbus Tcp Master> Fail to connect %d\n", __LINE__);
 		exit(0);
	}

	freeaddrinfo(res);
	
	return skfd;
}

void mbus_tcp_mstr(struct argu_tcp_table *table, void *pQtClass)
{
	int skfd;
	int ret;
	int retval;
	int rlen;
	int rlock;
	int wlock;
	char *port;
	char *addr;
	unsigned char rx_buf[FRMLEN];
	unsigned char tx_buf[FRMLEN];	
	fd_set rfds;
	fd_set wfds;
	struct timeval tv;
	struct tcp_frm_para *tmfpara = NULL;
	struct res_disp_table res_table;
	struct timespec t_send, t_recv;

	addr = table->addr;
	port = table->port;
	
	skfd = _create_sk_cli(addr, port);
	if(skfd < 0){
		printf("<Modbus Tcp Master> Fail to connect %d\n", __LINE__);
		exit(0);
	}
	
	// Set item
	tmfpara = set_item(table);
	if(tmfpara == NULL){
		printf("<Modbus TCP Master> Set parameter fail\n");
		exit(0);
	}

	tcp_func.build_qry((struct tcp_frm *)tx_buf, tmfpara);
	debug_print_data(tx_buf, TCPSENDQUERYLEN, SENDQRY);
	
	rlock = 0;
	wlock = 0;
	res_table.round = 0;
	res_table.mode = TCP_MASTER;
	res_table.tx_buf = tx_buf;
	res_table.txlen = TCPSENDQUERYLEN;
	
	do{	
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(skfd, &rfds);
		FD_SET(skfd, &wfds);
		
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		
		retval = select(skfd + 1, &rfds, &wfds, 0, &tv);
		if(retval <= 0){
			printf("<Modbus Tcp Master> select nothing ...\n");
			sleep(3);
			continue;
		}
		/* Send Query */
		if(FD_ISSET(skfd, &wfds) && !wlock && !rlock){	
			sleep(1);
			_sendAll(skfd, tx_buf, TCPSENDQUERYLEN);
			clock_gettime(CLOCK_REALTIME, &t_send);
			debug_print_data(tx_buf, TCPSENDQUERYLEN, SENDQRY);
			tmfpara->transID++; 
			wlock = 1;
		}
		/* Recv respond */
		if(FD_ISSET(skfd, &rfds) && wlock && !rlock){
			// may I use _recvAll?
			rlen = recv(skfd, rx_buf, FRMLEN, 0);
			clock_gettime(CLOCK_REALTIME, &t_recv);
			if(rlen < 1){
				printf("<Modbus TCP Master> fuckin recv empty !!\n");
				break;
			}
			res_table.rx_buf = rx_buf;
			res_table.rxlen = rlen;

			ret = tcp_func.chk_dest((struct tcp_frm *)rx_buf, tmfpara);
			if(ret == -1){
				memset(rx_buf, 0, FRMLEN);
				continue;
			}
			ret = tcp_func.resp_parser(rx_buf, tmfpara, rlen);
			if(ret == -1){
				print_data(rx_buf, TCPRESPEXCPFRMLEN, RECVEXCP);
			}
			debug_print_data(rx_buf, rlen, RECVRESP);
			poll_slvID(tmfpara->unitID);
			rlock = 1;
		}
		/* Result */
		if(rlock && wlock){
			res_table.resp_time = _get_resp_time(&t_recv, &t_send);
			//printf("Response time %f usec\n", res_table.resp_time);
			res_table.round ++;
			c_connector(&res_table, pQtClass);
			rlock = 0;
			wlock = 0;
		}
		if(isRunning == 0){
			break;
		}
	}while(isRunning);

	printf("%s is closing...\n", __func__);	
	close(skfd);
	free(tmfpara);
}
