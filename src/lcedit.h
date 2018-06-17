#pragma once

#include <QDir>
#include <QDebug>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QMutex>
#include "include.h"
#include <QByteArray>
#include <QPair>
#include <QFile>
#include <cstddef>
#include <sys/types.h>

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
	virtual ReturnValue<int> fileWrite(LCTreeWidgetItem *item, const QByteArray &data) = 0;
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
};

#undef CONSTRUCTOR

class LCEdit : public QMainWindow
{
	Q_OBJECT

public:
	explicit LCEdit(const QString &path, QWidget *parent = nullptr);
	~LCEdit();

private:
	QDir m_path;
	QList<LCPluginInterface *> plugins;
	
	void createTree(const QDir &base, LCTreeWidgetItem *parent = nullptr);
	void loadPlugins();

public:
	Ui::LCEdit *ui;
	QMutex mutex;
	QString filePath() { return m_path.path(); }
	template<class T> LCTreeWidgetItem *createEntry(T *parent, QString fileName, QString filePath)
	{
		auto *root = new LCTreeWidgetItem(parent);
		root->setText(0, fileName);
		root->setText(1, filePath);
		return root;
	}
	
	CALL_PLUGINS_WITH_RET(QByteArray, fileRead(LCTreeWidgetItem *item, off_t offset, size_t size), fileRead(item, offset, size))
		QFile file(item->filePath());
		qDebug() << "Opening file";
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return nullptr;
		qDebug() << "File opened";
		file.seek(offset);
		QByteArray buf = QByteArray(file.read(size ? size : file.size()));
		file.close();
		return buf;
	}
	
	CALL_PLUGINS_WITH_RET(int, fileWrite(LCTreeWidgetItem *item, const QByteArray &buf), fileWrite(item, buf))
		QFile file(item->filePath());
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
			return -1;
		
		qint64 c = file.write(buf);
		file.close();
		return c;
	}
	
public slots:
	void treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
};

QString GetFirstExistingPath(QFileInfo path);
