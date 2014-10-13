#include "MainWin.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	try
	{
		QApplication a(argc, argv);
		Main w;
		w.show();
	
		return a.exec();
	}
	catch(...)
	{
		std::cerr << "Unhandled exception!" << std::endl <<
				 boost::current_exception_diagnostic_information();
	}
}
