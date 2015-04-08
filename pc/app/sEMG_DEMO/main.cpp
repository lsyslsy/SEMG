#include "semg_demo.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	sEMG_DEMO w;
	w.show();
	return a.exec();
	
}
