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

#include "../include.h"
#include "lcsettings.h"

LCSettings::LCSettings(QIODevice *device, QSettings::Format format) : device(device)
{
	bool wasOpen = true;
	if (!device->isOpen())
	{
		wasOpen = false;
		device->open(QIODevice::ReadOnly);
	}
	temp.open();
	TRANSFER_CONTENTS(*device, temp)
	QString fileName = temp.fileName();
	temp.close();
	if (!wasOpen)
	{
		device->close();
	}

	settings = new QSettings(fileName, format);
	watcher.addPath(fileName);
	connect(&watcher, &QFileSystemWatcher::fileChanged, [this](QString path)
	{
		Q_UNUSED(path);
		temp.open();
		bool wasOpen = true;
		if (!this->device->isOpen())
		{
			wasOpen = false;
			this->device->open(QIODevice::WriteOnly);
		}
		TRANSFER_CONTENTS(temp, *(this->device))
		if (!wasOpen)
		{
			this->device->close();
		}
	});
}

LCSettings::~LCSettings()
{
	delete settings;
}
