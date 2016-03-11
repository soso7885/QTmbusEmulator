#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QMessageBox>
#include <QApplication>
#include <QTextStream>
#include <QRadioButton>
#include <QComboBox>

extern "C"{		// C++ call C
	#include "mbus.h"
}

#define SERIAL 0
#define TCP 1

#define SER_TX	0
#define SER_RX	1
#define TCP_TX	2
#define TCP_RX	3
#define ROUND	4
#define TXslvID	5
#define RXslvID	6
#define TXunitID	7
#define RXunitID	8
#define TXFC	9
#define RXFC	10
#define SEND_REQ	11
#define GET_REQ	12
#define SEND_RESP_BYTE	13
#define GET_RESP_BYTE	14
#define SEND_RESP_DATA	15
#define GET_RESP_DATA	16
#define RESP_TIME		17
#define SEND_EXCP_CODE	18
#define GET_EXCP_CODE	19

#define SERIAL_MASTER	0
#define SERIAL_SLAVE	1
#define TCP_MASTER		2
#define TCP_SLAVE		3

#define FRAME_IP	0
#define FRAME_PORT	1
#define FRAME_UID	2
#define FRAME_PORTNAME	3
#define FRAME_SLVID	4
#define FRAME_LEN	5
#define FRAME_STATUS	6
#define FRAME_VALUE	7

namespace Ui{
	class MainWindow;
}

/* 
 * All the modbus serial test and
 * modbus tcp test item
*/
struct argu_table{
	int mode;
	char *addr;		// TCP
	char *port;		// TCP
	char *port_name;	// Serial
	unsigned int slvID;	// Serial
	unsigned char unitID;	// TCP
	unsigned char fc;	// All
	unsigned int sLen;	// Serial
	unsigned int sAct;	// Serial 
	unsigned short tLen;	// TCP
	unsigned short tAct;	// TCP
};
	
class MainWindow : public QMainWindow
{
	Q_OBJECT

private:
	Ui::MainWindow *ui;
 	
	void _adjustArgu(const char *_switch, ...);
	void initTester(void);

	QString _mbusDataParser(struct res_disp_table *res_table, const int tag, bool type);
	QString __getQStringFromUChar(unsigned char *src, int len);

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow(void);

	struct argu_table table;	
	void startTest(void);

public slots:
	void res_update(struct res_disp_table *res_table);
	void excp_update(struct res_disp_table *res_table);

private slots:
	void adjustModeArgu(void);
	void adjustFcArgu(void);
	void takeArgu(void);
};

class Tester : public QObject
{
	Q_OBJECT

private:
	struct argu_table *table;
	struct argu_ser_table *ser_table;
	struct argu_tcp_table *tcp_table;
	
public:
	explicit Tester(struct argu_table *table);
	~Tester(void);


private slots:

public slots:
	void startTest(void);

signals:
	void res_signal(struct res_disp_table *res_table);
	void excp_signal(struct res_disp_table *res_table);
};

#endif
