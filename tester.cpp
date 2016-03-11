#include "mainwindow.h"
#include "ui_mainwindow.h"

Tester::Tester(struct argu_table *table)
{
	this->table = table;

	switch(table->mode){
		case SERIAL_MASTER:
			tcp_table = NULL;
			ser_table = new struct argu_ser_table;
			ser_table->port_name = table->port_name;
			ser_table->slvID = table->slvID;
			ser_table->fc = table->fc;
			if(ser_table->fc <= 4){
				ser_table->len = table->sLen;
			}else if(ser_table->fc == 5 || ser_table->fc == 6){
				ser_table->act = table->sAct;
			}else{
				qDebug("Serial master: for now, only support FC 1~6");
			}
			break;
		case SERIAL_SLAVE:
			tcp_table = NULL;
			ser_table = new struct argu_ser_table;
			ser_table->port_name = table->port_name;
			ser_table->slvID = table->slvID;
			ser_table->fc = table->fc;
			if(ser_table->fc <= 4){
				ser_table->len = table->sLen;
			}else if(ser_table->fc == 5 || ser_table->fc == 6){
				/* In slave mode, FC=5 and FC = 6 no need act */
			}else{
				qDebug("Serial slave: for now, only support FC 1~6");
			}
			break;
		case TCP_MASTER:
			ser_table = NULL;
			tcp_table = new struct argu_tcp_table;
			tcp_table->addr = table->addr;
			tcp_table->port = table->port;
			tcp_table->unitID = table->unitID;
			tcp_table->fc = table->fc;
			if(tcp_table->fc <= 4){
				tcp_table->len = table->tLen;
			}else if(tcp_table->fc == 5 || tcp_table->fc == 6){
				tcp_table->act = table->tAct;
			}else{
				qDebug("TCP master: for now, only support FC 1~6");
			}
			break;	
		case TCP_SLAVE:
			ser_table = NULL;
			tcp_table = new struct argu_tcp_table;
			tcp_table->port = table->port;
			tcp_table->unitID = table->unitID;
			tcp_table->fc = table->fc;
			if(tcp_table->fc <= 4){
				tcp_table->len = table->tLen;
			}
			break;
	}
}

Tester::~Tester(void)
{
	qDebug("Free tester");
	if(table->mode == SERIAL_MASTER || table->mode == SERIAL_SLAVE){
		delete ser_table;
	}else{
		delete tcp_table;
	}
}

void Tester::startTest(void)
{
	switch(table->mode){
		case SERIAL_MASTER:
			qDebug("Start modbus serial master !");
			mbus_ser_mstr(ser_table, this);
			break;
		case SERIAL_SLAVE:
			qDebug("Start modbus serial slave !");
			mbus_ser_slv(ser_table, this);
			break;
		case TCP_MASTER:
			qDebug("Start modbus TCP master !");
			mbus_tcp_mstr(tcp_table, this);
			break;
		case TCP_SLAVE:
			qDebug("Start modbus TCP slave !");
			mbus_tcp_slv(tcp_table, this);
			break;
		default:
			qDebug("Failed !");
	}
}

extern "C"
{
	void c_connector(struct res_disp_table *res_table, void *QtClass)
	{	
		Tester *tester = (Tester *)QtClass;
		switch(res_table->mode){
			case SERIAL_MASTER:
				if(*(res_table->rx_buf+1) & EXCPTIONCODE)
					emit tester->excp_signal(res_table);
				else
					emit tester->res_signal(res_table);
				break;
			case SERIAL_SLAVE:
				if(*(res_table->tx_buf+1) & EXCPTIONCODE)
					emit tester->excp_signal(res_table);
				else
					emit tester->res_signal(res_table);
				break;
			case TCP_MASTER:
				// shift 7 byte
				if(*(res_table->rx_buf+MBAPLEN+1) & EXCPTIONCODE)
					emit tester->excp_signal(res_table);
				else
					emit tester->res_signal(res_table);
				break;
			case TCP_SLAVE:
				// shift 7 byte
				if(*(res_table->tx_buf+MBAPLEN+1) & EXCPTIONCODE)
					emit tester->excp_signal(res_table);
				else
					emit tester->res_signal(res_table);
				break;
			default:
				qDebug("No such mode !\n");
		}
		
	}
}

