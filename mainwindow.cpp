#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	
	// which mode
	table.mode = ui->comboBox->currentIndex();
	
	// mode change
	connect(ui->comboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(adjustModeArgu()));
	// function code change
	connect(ui->FCspinBox, SIGNAL(valueChanged(int)),
			this, SLOT(adjustFcArgu()));
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(takeArgu()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(tellTesterStop()));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::tellTesterStop(void)
{
	emit stopSignal();
}

/*
 * Depend on the test mode,
 * enable or disable the argument
 * such as IP address/Slave ID ...
*/
void MainWindow::adjustModeArgu(void)
{
	table.mode = ui->comboBox->currentIndex();
	
	switch(table.mode){
		case SERIAL_MASTER:
			_adjustArgu("FFFTT", FRAME_IP, FRAME_PORT,
						FRAME_UID, FRAME_PORTNAME, FRAME_SLVID);
			ui->label_10->setText(QApplication::translate("MainWindow", "Query send :", 0));
			ui->label_11->setText(QApplication::translate("MainWindow", "Respones get :", 0));
			break;
		case SERIAL_SLAVE:
			_adjustArgu("FFFTT", FRAME_IP, FRAME_PORT,
						FRAME_UID, FRAME_PORTNAME, FRAME_SLVID);
			ui->label_10->setText(QApplication::translate("MainWindow", "Query get :", 0));
			ui->label_11->setText(QApplication::translate("MainWindow", "Respones send :", 0));
			break;
		case TCP_MASTER:
			_adjustArgu("TTTFF", FRAME_IP, FRAME_PORT,
						FRAME_UID, FRAME_PORTNAME, FRAME_SLVID);
			ui->label_10->setText(QApplication::translate("MainWindow", "Query send :", 0));
			ui->label_11->setText(QApplication::translate("MainWindow", "Respones get :", 0));
			break;
		case TCP_SLAVE:
			_adjustArgu("FTTFF", FRAME_IP, FRAME_PORT,
						FRAME_UID, FRAME_PORTNAME, FRAME_SLVID);
			ui->label_10->setText(QApplication::translate("MainWindow", "Query get :", 0));
			ui->label_11->setText(QApplication::translate("MainWindow", "Respones send :", 0));
			break;
		default:
			//qDebug("combobox index %d, %s(%d)", index, __func__, __LINE__);
			exit(0);
	}	
}

void MainWindow::adjustFcArgu(void)
{
	int fc_tmp;	
	fc_tmp = ui->FCspinBox->value();
	int mode = table.mode;

	if(fc_tmp <= 4){
		_adjustArgu("TFF", FRAME_LEN,
					FRAME_STATUS, FRAME_VALUE);
	}else if(fc_tmp == 5){
		if(mode == SERIAL_SLAVE || mode == TCP_SLAVE){
			_adjustArgu("FFF", FRAME_LEN,
					FRAME_STATUS, FRAME_VALUE);
		}else{
			_adjustArgu("FTF", FRAME_LEN,
					FRAME_STATUS, FRAME_VALUE);
		}
	}else if(fc_tmp == 6){
		if(mode == SERIAL_SLAVE || mode == TCP_SLAVE){
			_adjustArgu("FFF", FRAME_LEN,
					FRAME_STATUS, FRAME_VALUE);
		}else{
			_adjustArgu("FFT", FRAME_LEN,
					FRAME_STATUS, FRAME_VALUE);
		}
	}else{
		// Other function code
	}
}
	
void MainWindow::takeArgu(void)
{
	QByteArray tmp;	// for convert type
	QByteArray tmp1;
	bool ok;
	int mode = table.mode;

	if(mode == SERIAL_MASTER || mode == SERIAL_SLAVE){
		/* port_name, QString -> char* */
		tmp = ui->COMPORTlineEdit->text().toLatin1();
		table.port_name = tmp.data();
		/* slave ID, QString -> unsigned int */
		table.slvID = ui->SLVIDlineEdit->text().toInt(&ok, 10);
		/* Function code, QString -> 8 bit unsigned char */
		table.fc = ui->FCspinBox->text().toLocal8Bit().toUShort(&ok, 10);
		/* fc 1~4, set data len */
		if(table.fc <= 4){
			/* data length, QString -> unsigned int */
			table.sLen = ui->DATALENlineEdit->text().toInt(&ok, 10);
		}else if(table.fc == 5){
			/* fc 5, set register status(on/off) */
			if(QString::compare(ui->STATUScomboBox->currentText(),
				"ON", Qt::CaseInsensitive) == 0){
				table.sAct = 1;
			}else if(QString::compare(ui->STATUScomboBox->currentText(),
				"OFF", Qt::CaseInsensitive) == 0){
				table.sAct = 0;
			}else{
				qDebug("FC 5 only support ON/OFF");
				exit(0);
			}
		}else if(table.fc == 6){
			/* fc 6, get value in action */
			table.sAct = ui->VALUElineEdit->text().toInt(&ok, 10);
		}else{/* other function code */}
	}else if(mode == TCP_MASTER){
		/* ip_addr, QString -> char* */
		tmp = ui->IPlineEdit->text().toLatin1();
		table.addr = tmp.data();
		/* port, QString -> char* */
		tmp1 = ui->PORTlineEdit->text().toLatin1();
		table.port = tmp1.data();
		/* Unit ID, QString -> unsigned char */
		table.unitID = ui->UIDlineEdit->text().toLocal8Bit().toUShort(&ok, 10);
		/* Function code, QStrng -> unsigned char */
		table.fc = ui->FCspinBox->text().toLocal8Bit().toUShort(&ok, 10);
		if(table.fc <= 4){
			table.tLen = ui->DATALENlineEdit->text().toUShort(&ok, 10);
		}else if(table.fc == 5){
			if(QString::compare(ui->STATUScomboBox->currentText(),
				"ON", Qt::CaseInsensitive) == 0){
				table.tAct = 1;
			}else if(QString::compare(ui->STATUScomboBox->currentText(),
				"OFF", Qt::CaseInsensitive) == 0){
				table.tAct = 0;
			}else{
				qDebug("FC 5 only support ON/OFF");
				exit(0);
			}
		}else if(table.fc == 6){
			table.tAct = ui->VALUElineEdit->text().toInt(&ok, 10);
		}else{/* other function code */}
	}else if(mode == TCP_SLAVE){ 
		/* port, QString -> char* */
		tmp1 = ui->PORTlineEdit->text().toLatin1();
		table.port = tmp1.data();
		/* Unit ID, QString -> unsigned char */
		table.unitID = ui->UIDlineEdit->text().toLocal8Bit().toUShort(&ok, 10);
		/* Function code, QStrng -> unsigned char */
		table.fc = ui->FCspinBox->text().toLocal8Bit().toUShort(&ok, 10);
		if(table.fc <= 4){
			table.tLen = ui->DATALENlineEdit->text().toUShort(&ok, 10);
		}else{/* other function code */}
	}else{ /* Other mode */}

	initTester();
}

void MainWindow::initTester(void)
{
	QThread *thread = new QThread;
	Tester *tester = new Tester(&table);
	
	tester->moveToThread(thread);
	connect(thread, SIGNAL(started()), tester, SLOT(startTest()));
	connect(tester, SIGNAL(res_signal(struct res_disp_table *)),
			this, SLOT(res_update(struct res_disp_table *)));
	connect(tester, SIGNAL(excp_signal(struct res_disp_table *)),
			this, SLOT(excp_update(struct res_disp_table *)));
	connect(this, SIGNAL(stopSignal()), tester, SLOT(stopTest()));
	
	thread->start();
}

QString MainWindow::_mbusDataParser(struct res_disp_table *res_table, const int tag, bool type)
{
	unsigned char *tx;
	unsigned char *rx;
	int tlen;
	int rlen;
	int txfc, rxfc;
	QString Qres;

	if(type == TCP){	
		/* cut the MBAP Header ! */
		tx = res_table->tx_buf+6;
		rx = res_table->rx_buf+6;
		tlen = res_table->txlen - 6;
		rlen = res_table->rxlen - 6;
	}else{	// serial, do nothing
		tx = res_table->tx_buf;
		rx = res_table->rx_buf;
		tlen = res_table->txlen;
		rlen = res_table->rxlen;
	}	
	
	txfc = *(tx + 1);
	rxfc = *(rx + 1);

	switch(tag){
		case SER_TX:
			Qres = __getQStringFromUChar(tx, tlen);
			Qres.prepend("Raw data: ");
			break;
		case SER_RX:
			Qres = __getQStringFromUChar(rx, rlen);
			Qres.prepend("Raw data: ");
			break;
		case TCP_TX:
			/* Add the MBAP Header */
			Qres = __getQStringFromUChar(tx-6, tlen+6);
			Qres.prepend("Raw data: ");
			break;
		case TCP_RX:
			/* Add the MBAP Header */
			Qres = __getQStringFromUChar(rx-6, rlen+6);
			Qres.prepend("Raw data: ");
			break;
		case ROUND:
			Qres = QString::number(res_table->round);
			Qres.prepend("Round: ");
			break;
		case TXslvID:
			Qres = __getQStringFromUChar(tx, 1);
			Qres.prepend("Slave ID: ");
			break;
		case RXslvID:
			Qres = __getQStringFromUChar(rx, 1);
			Qres.prepend("Slave ID: ");
			break;
		case TXunitID:
			Qres = __getQStringFromUChar(tx, 1);
			Qres.prepend("Unit ID: ");
			break;
		case RXunitID:
			Qres = __getQStringFromUChar(rx, 1);
			Qres.prepend("Unit ID: ");
			break;
		case TXFC:
			Qres = __getQStringFromUChar(tx+1, 1);
			Qres.prepend("Function code: ");
			break;
		case RXFC:
			Qres = __getQStringFromUChar(rx+1, 1);
			Qres.prepend("Function code: ");
			break;
		case SEND_REQ:
			if(txfc <= 4){
				Qres = __getQStringFromUChar(tx+4, 2);
				Qres.prepend("Total number register/coils request: ");
			}else if(txfc == 5){
				if(*(tx+4) == 0xff){//register status
					Qres = "The status to write: ON";
				}else if(*(tx+4) == 0){//register status
					Qres = "The status to write: OFF";
				}else{
					return "Status Failed!";
				}
			}else if(txfc == 6){
				Qres = __getQStringFromUChar(tx+4, 2);
				Qres.prepend("The value to write: ");
			}else{/* other function code */}
			break;
		case GET_REQ:
			if(rxfc <= 4){
				Qres = __getQStringFromUChar(rx+4, 2);
				Qres.prepend("Total number request: ");
			}else if(rxfc == 5){
				if(*(rx+4) == 0xff){//register status
					Qres = "The status to write: ON";
				}else if(*(rx+4) == 0){//register status
					Qres = "The status to write: OFF";
				}else{
					return "Status Failed!";
				}
			}else if(rxfc == 6){
				Qres = __getQStringFromUChar(rx+4, 2);
				Qres.prepend("The value to write: ");
			}else{/* other function code */}
			break;
		case SEND_RESP_BYTE:
			if(txfc <= 4){
				Qres = __getQStringFromUChar(tx+2, 1);
				Qres.prepend("The number of data byte(s): ");
			}else{
				Qres = "The number of data byte(s): N/A";
			}
			break;
		case GET_RESP_BYTE:
			if(rxfc <= 4){
				Qres = __getQStringFromUChar(rx+2, 1);
				Qres.prepend("The number of data byte(s): ");
			}else{
				Qres = "The number of data byte(s): N/A";
			}
			break;
		case SEND_RESP_DATA:
			if(txfc <= 4){
				/* 
				 *respond data len = 
				 * all_data_len(tlen)-slvIDlen(1)-FClen(1)-dataByte(1)-CRClen(2)
				 * tlen - 5
				*/
				if(type == TCP)	//without CRC, tlen-5+2
					Qres = __getQStringFromUChar(tx+3, tlen-3);
				else	// with CRC
					Qres = __getQStringFromUChar(tx+3, tlen-5);
				Qres.prepend("Data: ");
			}else if(txfc == 5){
				if(*(tx+4) == 0xff){
					Qres = "The status written: ON";
				}else if(*(tx+4) == 0){
					Qres = "The status written: OFF";
				}else{
					return "Status Failed!";
				}
			}else if(txfc == 6){
				Qres = __getQStringFromUChar(tx+4, 2);
				Qres.prepend("The value written: ");
			}
			break;
		case GET_RESP_DATA:
			if(rxfc <= 4){
				/* 
				 *respond data len = 
				 * all data len(tlen)-slvIDlen(1)-FClen(1)-DataByte(1)-CRClen(2)
				 * tlen - 5;
				*/
				if(type == TCP)	// without CRC, tlen-5+2
					Qres = __getQStringFromUChar(rx+3, rlen-3);
				else	// with CRC
					Qres = __getQStringFromUChar(rx+3, rlen-5);
				Qres.prepend("Data: ");
			}else if(rxfc == 5){
				if(*(rx+4) == 0xff){
					Qres = "The status written: ON";
				}else if(*(rx+4) == 0){
					Qres = "The status written: OFF";
				}else{
					return "Status Failed!";
				}
			}else if(rxfc == 6){
				Qres = __getQStringFromUChar(rx+4, 2);
				Qres.prepend("The value written: ");
			}
			break;
		case RESP_TIME:
			Qres = QString::number(res_table->resp_time);
			Qres.prepend("Response time: ");
			Qres.append(" (usec)");
			break;
		case SEND_EXCP_CODE:
			switch(*(tx+2)){
				case 0x01:
					Qres = "Exception: Illegal function";
					break;
				case 0x02:
					Qres = "Exception: Illegal data address";
					break;
				case 0x03:
					Qres = "Exception: Illegal Data value";
					break;
				case 0x04:
					Qres = "Exception: Slave Device Failure";
					break;
				case 0x05:
					Qres = "Exception: Acknowledge";
					break;
				default:
					Qres = QString::number(*(tx+2));
					Qres.prepend("Unknow excption code: ");
					break;
			}
			break;
		case GET_EXCP_CODE:
			switch(*(rx+2)){
				case 0x01:
					Qres = "Exception: Illegal function";
					break;
				case 0x02:
					Qres = "Exception: Illegal data address";
					break;
				case 0x03:
					Qres = "Exception: Illegal Data value";
					break;
				case 0x04:
					Qres = "Exception: Slave Device Failure";
					break;
				case 0x05:
					Qres = "Exception: Acknowledge";
					break;
				default:
					Qres = QString::number(*(rx+2));
					Qres.prepend("Unknow excption code: ");
					break;
			}
			break;
		default:
			return "Bug: Wrong argu !!";
	}

	return Qres;
}

void MainWindow::excp_update(struct res_disp_table *res_table)
{
	int mode = res_table->mode;
	/* 
	 * XXX: WARRING!!!
	 * Should `new` QryListWidget first,
	 * I new 8 QryListWidget for each ListWidget 
	 * in QTdesigner, that is not cool, 
	 * but I'm in hurry!
	*/
	switch(mode){
		case SERIAL_MASTER:
			ui->QryListWidget->item(0)->
				setText(_mbusDataParser(res_table, SER_TX, SERIAL));
			ui->QryListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, SERIAL));
			ui->QryListWidget->item(2)->
				setText(_mbusDataParser(res_table, TXslvID, SERIAL));
			ui->QryListWidget->item(3)->
				setText(_mbusDataParser(res_table, TXFC, SERIAL));
			ui->QryListWidget->item(4)->
				setText(_mbusDataParser(res_table, SEND_REQ, SERIAL));

			ui->RespListWidget->item(0)->
				setText(_mbusDataParser(res_table, SER_RX, SERIAL));
			ui->RespListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, SERIAL));
			ui->RespListWidget->item(2)->
				setText(_mbusDataParser(res_table, RXslvID, SERIAL));
			ui->RespListWidget->item(3)->
				setText(_mbusDataParser(res_table, RXFC, SERIAL));
			ui->RespListWidget->item(4)->
				setText(_mbusDataParser(res_table, GET_EXCP_CODE, SERIAL));
			ui->RespListWidget->item(5)->
				setText(_mbusDataParser(res_table, RESP_TIME, SERIAL));
			break;
		case SERIAL_SLAVE:
			ui->QryListWidget->item(0)->
				setText(_mbusDataParser(res_table, SER_RX, SERIAL));
			ui->QryListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, SERIAL));
			ui->QryListWidget->item(2)->
				setText(_mbusDataParser(res_table, RXslvID, SERIAL));
			ui->QryListWidget->item(3)->
				setText(_mbusDataParser(res_table, RXFC, SERIAL));
			ui->QryListWidget->item(4)->
				setText(_mbusDataParser(res_table, GET_REQ, SERIAL));

			ui->RespListWidget->item(0)->
				setText(_mbusDataParser(res_table, SER_TX, SERIAL));
			ui->RespListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, SERIAL));
			ui->RespListWidget->item(2)->
				setText(_mbusDataParser(res_table, TXslvID, SERIAL));
			ui->RespListWidget->item(3)->
				setText(_mbusDataParser(res_table, TXFC, SERIAL));
			ui->RespListWidget->item(4)->
				setText(_mbusDataParser(res_table, SEND_EXCP_CODE, SERIAL));
			break;
		case TCP_MASTER:
			ui->QryListWidget->item(0)->
				setText(_mbusDataParser(res_table, TCP_TX, TCP));
			ui->QryListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, TCP));
			ui->QryListWidget->item(2)->
				setText(_mbusDataParser(res_table, TXunitID, TCP));
			ui->QryListWidget->item(3)->
				setText(_mbusDataParser(res_table, TXFC, TCP));
			ui->QryListWidget->item(4)->
				setText(_mbusDataParser(res_table, SEND_REQ, TCP));

			ui->RespListWidget->item(0)->
				setText(_mbusDataParser(res_table, TCP_RX, TCP));
			ui->RespListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, TCP));
			ui->RespListWidget->item(2)->
				setText(_mbusDataParser(res_table, RXunitID, TCP));
			ui->RespListWidget->item(3)->
				setText(_mbusDataParser(res_table, RXFC, TCP));
			ui->RespListWidget->item(4)->
				setText(_mbusDataParser(res_table, GET_EXCP_CODE, TCP));
			ui->RespListWidget->item(5)->
				setText(_mbusDataParser(res_table, RESP_TIME, TCP));
			break;
		case TCP_SLAVE:
			ui->QryListWidget->item(0)->
				setText(_mbusDataParser(res_table, TCP_RX, TCP));
			ui->QryListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, TCP));
			ui->QryListWidget->item(2)->
				setText(_mbusDataParser(res_table, RXunitID, TCP));
			ui->QryListWidget->item(3)->
				setText(_mbusDataParser(res_table, RXFC, TCP));
			ui->QryListWidget->item(4)->
				setText(_mbusDataParser(res_table, GET_REQ, TCP));

			ui->RespListWidget->item(0)->
				setText(_mbusDataParser(res_table, TCP_TX, TCP));
			ui->RespListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, TCP));
			ui->RespListWidget->item(2)->
				setText(_mbusDataParser(res_table, TXunitID, TCP));
			ui->RespListWidget->item(3)->
				setText(_mbusDataParser(res_table, TXFC, TCP));
			ui->RespListWidget->item(4)->
				setText(_mbusDataParser(res_table, SEND_EXCP_CODE, TCP));
			break;
		default:
			break;
	}
}

void MainWindow::res_update(struct res_disp_table *res_table)
{
	int mode = res_table->mode;
	/* 
	 * XXX: WARRING!!!
	 * Should `new` QryListWidget first,
	 * I new 8 QryListWidget for each ListWidget 
	 * in QTdesigner, that is not cool, 
	 * but I'm in hurry!
	*/
	switch(mode){
		case SERIAL_MASTER:
			ui->QryListWidget->item(0)->
				setText(_mbusDataParser(res_table, SER_TX, SERIAL));
			ui->QryListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, SERIAL));
			ui->QryListWidget->item(2)->
				setText(_mbusDataParser(res_table, TXslvID, SERIAL));
			ui->QryListWidget->item(3)->
				setText(_mbusDataParser(res_table, TXFC, SERIAL));
			ui->QryListWidget->item(4)->
				setText(_mbusDataParser(res_table, SEND_REQ, SERIAL));

			ui->RespListWidget->item(0)->
				setText(_mbusDataParser(res_table, SER_RX, SERIAL));
			ui->RespListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, SERIAL));
			ui->RespListWidget->item(2)->
				setText(_mbusDataParser(res_table, RXslvID, SERIAL));
			ui->RespListWidget->item(3)->
				setText(_mbusDataParser(res_table, RXFC, SERIAL));
			ui->RespListWidget->item(4)->
				setText(_mbusDataParser(res_table, GET_RESP_BYTE, SERIAL));
			ui->RespListWidget->item(5)->
				setText(_mbusDataParser(res_table, GET_RESP_DATA, SERIAL));
			ui->RespListWidget->item(6)->
				setText(_mbusDataParser(res_table, RESP_TIME, SERIAL));
			break;
		case SERIAL_SLAVE:
			ui->QryListWidget->item(0)->
				setText(_mbusDataParser(res_table, SER_RX, SERIAL));
			ui->QryListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, SERIAL));
			ui->QryListWidget->item(2)->
				setText(_mbusDataParser(res_table, RXslvID, SERIAL));
			ui->QryListWidget->item(3)->
				setText(_mbusDataParser(res_table, RXFC, SERIAL));
			ui->QryListWidget->item(4)->
				setText(_mbusDataParser(res_table, GET_REQ, SERIAL));

			ui->RespListWidget->item(0)->
				setText(_mbusDataParser(res_table, SER_TX, SERIAL));
			ui->RespListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, SERIAL));
			ui->RespListWidget->item(2)->
				setText(_mbusDataParser(res_table, TXslvID, SERIAL));
			ui->RespListWidget->item(3)->
				setText(_mbusDataParser(res_table, TXFC, SERIAL));
			ui->RespListWidget->item(4)->
				setText(_mbusDataParser(res_table, SEND_RESP_BYTE, SERIAL));
			ui->RespListWidget->item(5)->
				setText(_mbusDataParser(res_table, SEND_RESP_DATA, SERIAL));
			break;
		case TCP_MASTER:
			ui->QryListWidget->item(0)->
				setText(_mbusDataParser(res_table, TCP_TX, TCP));
			ui->QryListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, TCP));
			ui->QryListWidget->item(2)->
				setText(_mbusDataParser(res_table, TXunitID, TCP));
			ui->QryListWidget->item(3)->
				setText(_mbusDataParser(res_table, TXFC, TCP));
			ui->QryListWidget->item(4)->
				setText(_mbusDataParser(res_table, SEND_REQ, TCP));

			ui->RespListWidget->item(0)->
				setText(_mbusDataParser(res_table, TCP_RX, TCP));
			ui->RespListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, TCP));
			ui->RespListWidget->item(2)->
				setText(_mbusDataParser(res_table, RXunitID, TCP));
			ui->RespListWidget->item(3)->
				setText(_mbusDataParser(res_table, RXFC, TCP));
			ui->RespListWidget->item(4)->
				setText(_mbusDataParser(res_table, GET_RESP_BYTE, TCP));
			ui->RespListWidget->item(5)->
				setText(_mbusDataParser(res_table, GET_RESP_DATA, TCP));
			ui->RespListWidget->item(6)->
				setText(_mbusDataParser(res_table, RESP_TIME, TCP));
			break;
		case TCP_SLAVE:
			ui->QryListWidget->item(0)->
				setText(_mbusDataParser(res_table, TCP_RX, TCP));
			ui->QryListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, TCP));
			ui->QryListWidget->item(2)->
				setText(_mbusDataParser(res_table, RXunitID, TCP));
			ui->QryListWidget->item(3)->
				setText(_mbusDataParser(res_table, RXFC, TCP));
			ui->QryListWidget->item(4)->
				setText(_mbusDataParser(res_table, GET_REQ, TCP));

			ui->RespListWidget->item(0)->
				setText(_mbusDataParser(res_table, TCP_TX, TCP));
			ui->RespListWidget->item(1)->
				setText(_mbusDataParser(res_table, ROUND, TCP));
			ui->RespListWidget->item(2)->
				setText(_mbusDataParser(res_table, TXunitID, TCP));
			ui->RespListWidget->item(3)->
				setText(_mbusDataParser(res_table, TXFC, TCP));
			ui->RespListWidget->item(4)->
				setText(_mbusDataParser(res_table, SEND_RESP_BYTE, TCP));
			ui->RespListWidget->item(5)->
				setText(_mbusDataParser(res_table, SEND_RESP_DATA, TCP));
			break;
		default:
			break;
	}
}
	
/*
 * @fn      void MainWindow::_adjustArgu(const char *_switch, ...)
 * @brief   To enable/disable the frame on UI
 * @param   The switch (enable/disable)
 * @param   The frame which you want to switch(Uncertain parameters)
 * @return  void
 *
 * To enable, use 'T'
 * To disable, use 'F'
 *
 * behind the `_switch` argument, use those argument to decide which 
 * frame you want to modify:
 * FRAME_IP			: the IP address frame
 * FRAME_PORT		: the port number frame
 * FRAME_UID		: the unit ID frame
 * FRAME_PORTNAME	: the serial port name frame
 * FRAME_SLVID		: the slave ID frame
 * FRAME_LEN		: the total data length frame
 * FRAME_STATUS		: the register status combobox
 * FRAME_VALUE		: the register value frame	
*/
void MainWindow::_adjustArgu(const char *_switch, ...)
{
	va_list ap;
	const char *pSwitch;
	
	va_start(ap, _switch);
	
	for(pSwitch = _switch; *pSwitch != 0; ++pSwitch){
		switch(va_arg(ap, int)){
			case FRAME_IP:
				if(*pSwitch == 'T'){
					ui->IPlineEdit->setEnabled(true);
				}else if(*pSwitch == 'F'){
					ui->IPlineEdit->clear();
					ui->IPlineEdit->setEnabled(false);
				}else{
					qDebug("(_adjustArgu) T for enable, F for disable");
					exit(0);
				}
				break;
			case FRAME_PORT:
				if(*pSwitch == 'T'){
					ui->PORTlineEdit->setEnabled(true);
				}else if(*pSwitch == 'F'){
					ui->PORTlineEdit->clear();
					ui->PORTlineEdit->setEnabled(false);
				}else{
					qDebug("(_adjustArgu) T for enable, F for disable");
					exit(0);
				}
				break;
			case FRAME_UID:
				if(*pSwitch == 'T'){
					ui->UIDlineEdit->setEnabled(true);
				}else if(*pSwitch == 'F'){
					ui->UIDlineEdit->clear();
					ui->UIDlineEdit->setEnabled(false);
				}else{
					qDebug("(_adjustArgu) T for enable, F for disable");
					exit(0);
				}
				break;
			case FRAME_PORTNAME:
				if(*pSwitch == 'T'){
		
					ui->COMPORTlineEdit->setEnabled(true);
				}else if(*pSwitch == 'F'){
					ui->COMPORTlineEdit->clear();
					ui->COMPORTlineEdit->setEnabled(false);
				}else{
					qDebug("(_adjustArgu) T for enable, F for disable");
					exit(0);
				}
				break;
			case FRAME_SLVID:
				if(*pSwitch == 'T'){
					ui->SLVIDlineEdit->setEnabled(true);
				}else if(*pSwitch == 'F'){
					ui->SLVIDlineEdit->clear();
					ui->SLVIDlineEdit->setEnabled(false);
				}else{
					qDebug("(_adjustArgu) T for enable, F for disable");
					exit(0);
				}
				break;
			case FRAME_LEN:
				if(*pSwitch == 'T'){
					ui->DATALENlineEdit->setEnabled(true);
				}else if(*pSwitch == 'F'){
					ui->DATALENlineEdit->clear();
					ui->DATALENlineEdit->setEnabled(false);
				}else{
					qDebug("(_adjustArgu) T for enable, F for disable");
					exit(0);
				}
				break;
			case FRAME_STATUS:
				if(*pSwitch == 'T'){
					ui->STATUScomboBox->setEnabled(true);
				}else if(*pSwitch == 'F'){
					ui->STATUScomboBox->setEnabled(false);
				}else{
					qDebug("(_adjustArgu) T for enable, F for disable");
					exit(0);
				}
				break;
			case FRAME_VALUE:
				if(*pSwitch == 'T'){
					ui->VALUElineEdit->setEnabled(true);
				}else if(*pSwitch == 'F'){
					ui->VALUElineEdit->clear();
					ui->VALUElineEdit->setEnabled(false);
				}else{
					qDebug("(_adjustArgu) T for enable, F for disable");
					exit(0);
				}
				break;
		}
	}
	va_end(ap);
}

QString MainWindow::__getQStringFromUChar(unsigned char *src, int len)
{
	QString result = "";
	QString s;

	for(int i = 0; i< len; i++){
		s = QString("%1").arg(src[i], 0, 16);
		if(s.length() == 1)
			result.append("0");

		result.append(s);
	}
	return result;
}
	
