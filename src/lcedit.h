#pragma once

#include "include.h"
#include <QDir>
#include <QDebug>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QByteArray>
#include <QProcess>
#include <QFile>
#include <QSettings>
#include <cstddef>
#include <sys/types.h>

#if 0 // FIXME: Open the right directory
#ifdef Q_OS_WIN
#define C4CFG_ConfigPath R"(HKEY_CURRENT_USERS\Software\RedWolf Design\LegacyClonk)";
#define C4CFG_Format QSettings::NativeFormat
#else
#define C4CFG_Format QSettings::IniFormat
#if defined(Q_OS_MACOS)
#define C4CFG_ConfigPath QDir(qEnvironmentVariable("HOME")).absoluteFilePath("Library/Preferences/legacyclonk.config")
#else
#define C4CFG_ConfigPath QDir(qEnvironmentVariable("HOME")).absoluteFilePath(".legacyclonk/config")
#ifndef Q_OS_UNIX
#warning Using $HOME/.legacyclonk/config as Clonk config file. This might not be the right choice for your operating system.
#endif // Q_OS_UNIX
#endif // Q_OS_MACOS
#endif // Q_OS_WIN
#endif
class LCEdit;
class LCTreeWidgetItem;

class LCPluginInterface
{
public:
	virtual ~LCPluginInterface() {}
	virtual void init(LCEdit *editor) = 0;
	virtual ExecPolicy createTree(const QDir &base, LCTreeWidgetItem *parent) = 0;
	virtual int priority() = 0;
	virtual ExecPolicy treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous) = 0;
	virtual ReturnValue<QByteArray> fileRead(LCTreeWidgetItem *item, off_t offset = 0, size_t size = 0) = 0;
	virtual ReturnValue<int> fileWrite(LCTreeWidgetItem *item, const QByteArray &buf, off_t offset = 0) = 0;
};

#define LCPlugin_Iid "org.legacyclonk.LegacyClonk.LCEdit.LCPluginInterface"
Q_DECLARE_INTERFACE(LCPluginInterface, LCPlugin_Iid)

#define CALL_PLUGINS(x) bool ret = false; \
foreach(LCPluginInterface *obj, plugins) \
{ \
	switch (obj->x) \
	{ \
		case EP_Continue: \
			continue; \
		case EP_AbortMain: \
			ret = true; \
		case EP_AbortAll: \
			return; \
	} \
} \
if (ret) return;

#define CALL_PLUGINS_WITH_RET(t, f, x) t f \
{ \
	foreach (LCPluginInterface *obj, plugins) \
	{ \
		ReturnValue<t> ret = obj->x; \
		switch (ret.code) \
		{ \
			case EP_Continue: \
				continue; \
			case EP_AbortMain: \
			case EP_AbortAll: \
				return ret.value; \
		} \
	}

#define CALL_PLUGINS_WITH_RET_NULLPTR(t, f, x) CALL_PLUGINS_WITH_RET(t, f, x) \
	return nullptr; \
}

namespace Ui {
class LCEdit;
}

class LCTreeWidgetItem : public QTreeWidgetItem
{
#define CONSTRUCTOR(x) LCTreeWidgetItem(x *parent = nullptr) : QTreeWidgetItem(parent) {}
public:
	CONSTRUCTOR(QTreeWidgetItem)
	CONSTRUCTOR(QTreeWidget)
	~LCTreeWidgetItem() = default;
	QString filePath();
	LCTreeWidgetItem *getChildByName(const QString &name);
};

#undef CONSTRUCTOR

class LCEdit : public QMainWindow
{
	Q_OBJECT

public:
	explicit LCEdit(const QString &path = "", QWidget *parent = nullptr);
	~LCEdit();

private:
	QDir m_path;
	QList<LCPluginInterface *> plugins;
	QProcess *proc = nullptr;

	void createTree(const QDir &base, LCTreeWidgetItem *parent = nullptr);
	void loadPlugins();
	void loadPlugin(LCPluginInterface *plugin);

public:
	Ui::LCEdit *ui;
	QString filePath() { return m_path.path(); }
	template<class T> LCTreeWidgetItem *createEntry(T *parent, QString fileName, QString filePath)
	{
		auto *root = new LCTreeWidgetItem(parent);
		root->setText(0, fileName);
		root->setText(1, filePath);
		ui->treeWidget->sortItems(0, Qt::AscendingOrder);
		return root;
	}

	CALL_PLUGINS_WITH_RET(QByteArray, fileRead(LCTreeWidgetItem *item, off_t offset, size_t size), fileRead(item, offset, size))
		QFile file(item->filePath());
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return nullptr;
		file.seek(offset);
		QByteArray buf = QByteArray(file.read(size ? size : file.size()));
		file.close();
		return buf;
	}

	CALL_PLUGINS_WITH_RET(int, fileWrite(LCTreeWidgetItem *item, const QByteArray &buf, off_t offset), fileWrite(item, buf, offset))
		QFile file(item->filePath());
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
			return -1;

		file.seek(offset);
		qint64 c = file.write(buf);
		file.close();
		return c;
	}

private slots:
	void setCommandLine(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void startProcess();

public slots:
	void treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
};

QString GetFirstExistingPath(QFileInfo path);
