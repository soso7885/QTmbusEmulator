/*
 * Modbus TCP Slave.
 * send respond
*/
#include "mbus.h" 

#define BACKLOG		10

extern struct mbus_tcp_func tcp_func;

static struct tcp_frm_para * set_item(struct argu_tcp_table *table)
{

	struct tcp_frm_para *tsfpara;
	
	tsfpara = (struct tcp_frm_para *)malloc(sizeof(struct tcp_frm_para));
	if(tsfpara == NULL){
		printf("Malloc failed !\n");
		exit(0);
	}

	printf("Modbus TCP Slave !\n");
	tsfpara->unitID = table->unitID;
	tsfpara->potoID = (unsigned char)TCPMBUSPROTOCOL;
	tsfpara->straddr = 1;
	switch(table->fc){
		case 1:
			tsfpara->fc = READCOILSTATUS;
			tsfpara->len = table->len;	// TODO: chk len
			break;
		case 2:
			tsfpara->fc = READINPUTSTATUS;
			tsfpara->len = table->len;	// TODO: chk len
			break;
		case 3:
			tsfpara->fc = READHOLDINGREGS;
			tsfpara->len = table->len;	// TODO: chk len
			break;
		case 4:
			tsfpara->fc = READINPUTREGS;
			tsfpara->len = table->len;	// TODO: chk len
			break;
		case 5:
			tsfpara->fc = FORCESIGLEREGS;
			/*if(table->act == 1 || table->act == 0){ 
				tsfpara->act = table->act << 8;
			}else{
				printf("Force Single Coil, the status only 1/0\n");
				return NULL;
			}*/
			break;
		case 6:
			tsfpara->fc = PRESETEXCPSTATUS;
			//tsfpara->act = table->act;
			break;
		default:
			printf("For now only support function code :\n");
			printf("1        Read Coil Status\n");
			printf("2        Read Input Status\n");
			printf("3        Read Holding Registers\n");
			printf("4        Read Input Registers\n");
			printf("5        Force Single Coil\n");
			printf("6        Preset Single Register\n");
			return NULL;
	}	
	return tsfpara;
}

static int _choose_resp_frm(unsigned char *tx_buf, struct thread_pack *tpack, int phaser_ret)
{
	int txlen = 0;
	struct tcp_frm_para *tsfpara;
	
	tsfpara = tpack->tsfpara;	
	if(!phaser_ret){
		switch(tsfpara->fc){
			case READCOILSTATUS:
				txlen = tcp_func.build_0102_resp((struct tcp_frm_rsp *)tx_buf, tpack, READCOILSTATUS);
				break;
			case READINPUTSTATUS:
				txlen = tcp_func.build_0102_resp((struct tcp_frm_rsp *)tx_buf, tpack, READINPUTSTATUS);
				break;
			case READHOLDINGREGS:
				txlen = tcp_func.build_0304_resp((struct tcp_frm_rsp *)tx_buf, tpack, READHOLDINGREGS);
				break;
			case READINPUTREGS:
				txlen = tcp_func.build_0304_resp((struct tcp_frm_rsp *)tx_buf, tpack, READINPUTREGS);
				break;
			case FORCESIGLEREGS:
				txlen = tcp_func.build_0506_resp((struct tcp_frm *)tx_buf, tpack, FORCESIGLEREGS);
				break;
			case PRESETEXCPSTATUS:
				txlen = tcp_func.build_0506_resp((struct tcp_frm *)tx_buf, tpack, PRESETEXCPSTATUS);
				break;
			default:
				printf("<Modbus TCP Slave> unknown function code : %d\n", tpack->tsfpara->fc);
				return -1;
			}
	}else if(phaser_ret == -1){
		txlen = tcp_func.build_excp((struct tcp_frm_excp *)tx_buf, tsfpara, EXCPILLGFUNC);
		print_data(tx_buf, txlen, SENDEXCP);	
	}else if(phaser_ret == -2){
		txlen = tcp_func.build_excp((struct tcp_frm_excp *)tx_buf, tsfpara, EXCPILLGDATAADDR);
		print_data(tx_buf, txlen, SENDEXCP);
	}else if(phaser_ret == -3){
		txlen = tcp_func.build_excp((struct tcp_frm_excp *)tx_buf, tsfpara, EXCPILLGDATAVAL);
		print_data(tx_buf, txlen, SENDEXCP);
	}

	return txlen;
}

int _create_sk_svr(char *port)
{
	int skfd = 0;
	int ret;
	int opt;
	struct addrinfo hints;	
	struct addrinfo *res;	
	struct addrinfo *p;
	
	printf("PORT : %s\n", port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	
	ret = getaddrinfo(NULL, port, &hints, &res);
	if(ret != 0){
		printf("<Modbus Tcp Slave> getaddrinfo : %s\n", gai_strerror(ret));
		exit(0);
	}
	
	for(p = res; p != NULL; p = p->ai_next){
		skfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(skfd == -1){
			continue;
		}else{
			printf("<Modbus Tcp Slave> sockFD = %d\n", skfd);
		}
		
		opt = 1;
		ret = setsockopt(skfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if(ret == -1){
			printf("<Modbus Tcp Slave> setsockopt : %s\n", strerror(errno));
			close(skfd);
			exit(0);
		}
	
		ret = bind(skfd, p->ai_addr, p->ai_addrlen);
		if(ret == -1){
			printf("<Modbus Tcp Slave> bind : %s\n", strerror(errno));
			close(skfd);
			continue;
		}
		printf("<Modbus Tcp Slave> bind success\n");
	
		break;
	}
	
	if(p == NULL){
		printf("<Modbus Tcp Slave> create socket fail ...\n");
		close(skfd);
		exit(0);
	}
	
	ret = listen(skfd, BACKLOG);
	if(ret == -1){
		printf("<Modbus Tcp Slave> listen : %s\n", strerror(errno));
		close(skfd);
		exit(0);
	}

	freeaddrinfo(res);
	printf("<Modbus Tcp Slave> Waiting for connect ...\n");
	
	return skfd;
}

int _sk_accept(int skfd)
{
	int rskfd;
	char addr[INET6_ADDRSTRLEN];
	socklen_t addrlen;
	struct sockaddr_storage acp_addr;
	struct sockaddr_in *p;
	struct sockaddr_in6 *s;
	
	addrlen = sizeof(acp_addr);
	
	rskfd = accept(skfd, (struct sockaddr*)&acp_addr, &addrlen);
	if(rskfd == -1){
		close(rskfd);
		printf("<Modbus Tcp Slave> accept : %s\n", strerror(errno));
		exit(0);
	}
	
	if(acp_addr.ss_family == AF_INET){
		p = (struct sockaddr_in *)&acp_addr;
		inet_ntop(AF_INET, &p->sin_addr, addr, sizeof(addr));
		printf("<Modbus Tcp Slave> recv from IP : %s\n", addr);
	}else if(acp_addr.ss_family == AF_INET6){
		s = (struct sockaddr_in6 *)&acp_addr;
		inet_ntop(AF_INET6, &s->sin6_addr, addr, sizeof(addr));
		printf("<Modbus Tcp Slave> recv from IP : %s\n", addr);
	}else{
		printf("<Modbus Tcp Slave> fuckin wried ! What is the recv addr family?");
		return -1;
	}
	
	return rskfd;
}

void *work_thread(void *data)
{
	int txlen;
	int retval;
	int ret = 0;
	int phaser_ret = 0;
	int rskfd;
	void *pQtClass;
	int wlock;
	int rlock;	
	fd_set rfds;
	fd_set wfds;
	struct timeval tv;
	struct thread_pack *tpack;
	struct tcp_frm_para *tsfpara;
	unsigned char rx_buf[FRMLEN];
	unsigned char tx_buf[FRMLEN];
	struct res_disp_table res_table;

	tpack = (struct thread_pack *)data;
	rskfd = tpack->rskfd;
	tsfpara = tpack->tsfpara;
	pQtClass = tpack->pQtClass;

	wlock = 0;
	rlock = 0;
	res_table.round = 0;
	res_table.mode = TCP_SLAVE;
	printf("<Modbus Tcp Slave> Create work thread, connect fd = %d | thread ID = %lu\n",
			 rskfd, pthread_self());

	do{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(rskfd, &rfds);
		FD_SET(rskfd, &wfds);

		tv.tv_sec = 3;
		tv.tv_usec = 0;

		retval = select(rskfd + 1, &rfds, &wfds, 0, &tv);
		if(retval <= 0){
			printf("<Modbus Tcp Slave> Watting query ...\n");
			sleep(3);
			continue;
		}

		/* Recv Query */
		if(FD_ISSET(rskfd, &rfds) && !rlock && !wlock){
			ret = _recvAll(rskfd, rx_buf, TCPSENDQUERYLEN);
			if(ret == -1){
				printf("<Modbus Tcp Slave> disconnect, thread ID = %lu\n", pthread_self());
				close(rskfd);
				pthread_exit(NULL);
			}
			res_table.rx_buf = rx_buf;
			res_table.rxlen = TCPSENDQUERYLEN;

			ret = tcp_func.chk_dest((struct tcp_frm *)rx_buf, tsfpara);
			if(ret == -1){
				memset(rx_buf, 0, FRMLEN);
				continue;
			}
			debug_print_data(rx_buf, TCPSENDQUERYLEN, RECVQRY);
			phaser_ret = tcp_func.qry_parser((struct tcp_frm *)rx_buf, tpack);
			rlock = 1;
		}
		/* Send Respond */	
		if(FD_ISSET(rskfd, &wfds) && rlock && !wlock){
			txlen = _choose_resp_frm(tx_buf, tpack, phaser_ret);
			if(txlen == -1){
				break;
			}
			_sendAll(rskfd, tx_buf, txlen);
			debug_print_data(tx_buf, txlen, SENDRESP);
			res_table.tx_buf = tx_buf;
			res_table.txlen = txlen;
			poll_slvID(tsfpara.unitID);
			wlock = 1;
		}
		/* Result */
		if(rlock && wlock){
			res_table.round++;
			c_connector(&res_table, pQtClass);
			debug_print_data(tx_buf, txlen, SENDRESP);
			rlock = 0;
			wlock = 0;
		}
	}while(isRunning);
	
	printf("%s is closing...\n", __func__);
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

void mbus_tcp_slv(struct argu_tcp_table *table, void *pQtClass)
{
	int ret;
	int skfd;
	int rskfd;
	char *port;
	pthread_t tid;
	pthread_attr_t attr;
	struct sched_param param;
	struct thread_pack tpack;
	struct tcp_frm_para *tsfpara = NULL;
	struct tcp_tmp_frm *tmpara = NULL;
	
	port = table->port;
	
	tmpara = (struct tcp_tmp_frm *)malloc(sizeof(struct tcp_tmp_frm));
	if(tmpara == NULL){
		printf("Malloc failed %s(%d)\n", __func__, __LINE__);
		exit(0);
	}

	tsfpara = set_item(table);
	if(tsfpara == NULL){
		printf("<Modbus Tcp Slave> set parameter fail !!\n");
		exit(0);
	}

	skfd = _create_sk_svr(port);
	if(skfd == -1){
		printf("<Modbus Tcp Slave> god damn wried !!\n");
		exit(0);
	}
	
	tpack.tmpara = tmpara;
	tpack.tsfpara = tsfpara;
	tpack.pQtClass = pQtClass;
	pthread_mutex_init(&(tpack.mutex), NULL);
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);	// use RR scheduler
	param.sched_priority = 6;						
	pthread_attr_setschedparam(&attr, &param);
	
	do{	
		rskfd = _sk_accept(skfd);
		if(rskfd == -1){
			printf("<Modbus Tcp Slave> god damn wried !!\n");
			break;
		}
	
		tpack.rskfd = rskfd;
		tpack.tsfpara = tsfpara;
		ret = pthread_create(&tid, NULL, work_thread, (void *)&tpack);	
		if(ret != 0){
			handle_error_en(ret, "pthread_create");
		}	
	}while(0);
	pthread_join(tid, NULL);

	close(skfd);
	pthread_mutex_destroy(&(tpack.mutex));
	pthread_attr_destroy(&attr);
	free(tsfpara);
	free(tmpara);
}



















