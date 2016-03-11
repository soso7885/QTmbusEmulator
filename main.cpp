#include "mainwindow.h"

/* 
 * Gloable variance:
 * The QT test class pointer,
 * to update the result!
*/
void *QtClass;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MainWindow w;
	w.show();

	return app.exec();
}
