#pragma once
#include <QFileSystemWatcher>
#include <QIODevice>
#include <QSettings>
#include <QTemporaryFile>

class LCSettings : public QObject
{
	Q_OBJECT
public:
	LCSettings(QIODevice *device, QSettings::Format format = QSettings::IniFormat);
	~LCSettings();
private:
	QIODevice *device = nullptr; // has to stay valid during the entire lifetime of an LCSettings instance
	QTemporaryFile temp;
	void transferContents(QIODevice *source, QIODevice *target);
public:
	QFileSystemWatcher watcher;
	QSettings *settings = nullptr;
};
