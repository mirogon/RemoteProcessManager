#include <iostream>

#include "remote_process_manager.h"



int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	m1::remote_process_manager rpm;

	return app.exec();
}