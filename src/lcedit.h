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
//OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.#pragma once

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIODevice>
#include <QMainWindow>
#include <QMutex>
#include <QProcess>
#include <QSettings>
#include <QTemporaryFile>
#include <QTreeWidgetItem>
#include <cstddef>
#include <cstdint>
#include "include.h"

namespace Ui {
class LCEdit;
}

enum class ExecPolicy {
	Continue = 1,
	AbortMain,
	AbortAll
};

template<class T> class ReturnValue {
public:
	ExecPolicy code;
	T value;
	ReturnValue(ExecPolicy c = ExecPolicy::Continue, T v = nullptr) : code(c), value(v) {}
	ReturnValue(const ReturnValue &ret) : code(ret.code), value(ret.value) { }
	ReturnValue &operator =(const ReturnValue &ret){ code = ret.code; value = ret.value; return *this; }
};

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

class LCTreeWidgetItem : public QTreeWidgetItem
{
#define CONSTRUCTOR(x) LCTreeWidgetItem(x *parent = nullptr) : QTreeWidgetItem(parent) {}

public:
	CONSTRUCTOR(QTreeWidgetItem)
	CONSTRUCTOR(QTreeWidget)
	~LCTreeWidgetItem() = default;
	QString filePath();
	LCTreeWidgetItem *getChildByName(const QString &name, Qt::CaseSensitivity sensitivity = Qt::CaseSensitive);
	inline void deleteChildren()
	{
		qDeleteAll(takeChildren());
	}
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
		return root;
	}

	[[nodiscard]]
	QIODevice *getDevice(LCTreeWidgetItem *item);
	bool destroyDevice(LCTreeWidgetItem *item, QIODevice *device);
	LCTreeWidgetItem *getItemByPath(const QString &path, LCTreeWidgetItem *parent = nullptr);

private slots:
	void setCommandLine(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void startProcess();

public slots:
	void treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
};
