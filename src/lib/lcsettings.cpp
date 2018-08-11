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
