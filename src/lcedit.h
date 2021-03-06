//Copyright (c) 2018-2019, George Tokmaji

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

#pragma once
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonObject>
#include <QMainWindow>
#include <QMutex>
#include <QProcess>
#include <QSettings>
#include <QTemporaryFile>
#include <QTreeWidgetItem>
#include <cstddef>
#include <cstdint>

#include "plugin.h"
#include "ui_lcedit.h"

const int BLOCKSIZE = 1024;
#define TRANSFER_CONTENTS(x, y) while (!(x).atEnd()) { (y).write((x).read(BLOCKSIZE)); }

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
	explicit LCEdit(QWidget *parent = nullptr);
	~LCEdit();

public:
	Ui::LCEdit *ui;
	QSettings settings;
	QList<LCPlugin> plugins;
	template<class T> LCTreeWidgetItem *createEntry(T *parent, QString fileName, QString filePath)
	{
		auto *root = new LCTreeWidgetItem(parent);
		root->setText(0, fileName);
		root->setText(1, filePath);
		root->setIcon(0, QFileIconProvider().icon(QFileInfo(filePath)));
		return root;
	}

	[[nodiscard]] QIODevicePtr getDevice(LCTreeWidgetItem *item);
	LCTreeWidgetItem *getItemByPath(const QString &path, LCTreeWidgetItem *parent = nullptr);

private:
	QProcess *proc = nullptr;
	QHash<QIODevice *, LCTreeWidgetItem *> deviceHash;

	void createTree(const QDir &base, LCTreeWidgetItem *parent = nullptr);
	void loadPlugins();

private slots:
	void setCommandLine(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void treeItemExpanded(QTreeWidgetItem *item);
	void startProcess();
	void showPathDialog();

public slots:
	void treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void reload();
};
