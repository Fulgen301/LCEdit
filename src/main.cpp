//Copyright (c) 2018, George Tokmaji

//Permission to use, copy, modify, and/or distribute this software for any
//purpose with or without fee is hereby granted, provided that the above
//copyright notice and this permission notice appear in all copies.

//THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "lcedit.h"
#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	if (QFontDatabase().families().contains("Endeavour"))
	{
		QFont font = app.font();
		font.setFamily("Endeavour");
		app.setFont(font);
	}

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

