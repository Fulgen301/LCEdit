#include "lcedit.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(),QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);

	QTranslator myappTranslator;
	myappTranslator.load("lcedit_" + QLocale::system().name());
	app.installTranslator(&myappTranslator);
	
	LCEdit w(app.arguments().length() >= 2 ? app.arguments()[1] : QDir::currentPath());
	w.show();

	return app.exec();
}

