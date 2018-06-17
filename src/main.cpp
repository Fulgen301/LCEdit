#include "lcedit.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
	assert(GetFirstExistingPath(QFileInfo("/home/tokgeo/CR/Adventure.c4s")) == QString("/home/tokgeo/CR/Adventure.c4s"));
	assert(GetFirstExistingPath(QFileInfo("/home/tokgeo/CR/Adventure.c4s/Scenario.txt")) == QString("/home/tokgeo/CR/Adventure.c4s"));
	QApplication app(argc, argv);
	
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(),QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);

	QTranslator myappTranslator;
	myappTranslator.load("lcedit_" + QLocale::system().name());
	app.installTranslator(&myappTranslator);
	
	LCEdit w("/home/tokgeo/CR");
	w.show();

	return app.exec();
}

