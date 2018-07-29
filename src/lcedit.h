#pragma once

#include "include.h"
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIODevice>
#include <QMainWindow>
#include <QMutex>
#include <QProcess>
#include <QSettings>
#include <QTreeWidgetItem>
#include <cstddef>
#include <cstdint>

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
	virtual ReturnValue<QIODevice *> getDevice(LCTreeWidgetItem *item) = 0;
	virtual ReturnValue<bool> destroyDevice(LCTreeWidgetItem *item, QIODevice *device) = 0;
};

#define LCPlugin_Iid "org.legacyclonk.LegacyClonk.LCEdit.LCPluginInterface"
Q_DECLARE_INTERFACE(LCPluginInterface, LCPlugin_Iid)

#define CALL_PLUGINS(x) bool ret = false; \
foreach(LCPluginInterface *obj, plugins) \
{ \
	switch (obj->x) \
	{ \
		case ExecPolicy::Continue: \
			continue; \
		case ExecPolicy::AbortMain: \
			ret = true; \
			[[fallthrough]]; \
		case ExecPolicy::AbortAll: \
			return; \
	} \
} \
if (ret) return;

#define CALL_PLUGINS_WITH_RET(t, x) \
	ReturnValue<t> ret(ExecPolicy::Continue, static_cast<t>(NULL)); \
	foreach (LCPluginInterface *plugin, plugins) \
	{ \
		ret = plugin->x; \
		if (ret.code != ExecPolicy::Continue) \
		{ \
			break; \
		} \
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
	QMutex mutex;
};

#undef CONSTRUCTOR

class LCEdit : public QMainWindow
{
	Q_OBJECT

public:
	explicit LCEdit(const QString &path = "", QWidget *parent = nullptr);
	~LCEdit();

private:
	QProcess *proc = nullptr;

	void createTree(const QDir &base, LCTreeWidgetItem *parent = nullptr);
	void loadPlugins();
	void loadPlugin(LCPluginInterface *plugin);

public:
	Ui::LCEdit *ui;
	QDir m_path;
	QList<LCPluginInterface *> plugins;
	QString filePath() { return m_path.path(); }
	template<class T> LCTreeWidgetItem *createEntry(T *parent, QString fileName, QString filePath)
	{
		auto *root = new LCTreeWidgetItem(parent);
		root->setText(0, fileName);
		root->setText(1, filePath);
		root->setIcon(0, QFileIconProvider().icon(QFileInfo(filePath)));
		ui->treeWidget->sortItems(0, Qt::AscendingOrder);
		return root;
	}

	[[nodiscard]]
	QIODevice *getDevice(LCTreeWidgetItem *item);
	bool destroyDevice(LCTreeWidgetItem *item, QIODevice *device);

private slots:
	void setCommandLine(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void startProcess();

public slots:
	void treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
};
