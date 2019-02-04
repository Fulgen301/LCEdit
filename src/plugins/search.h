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

#pragma once
#include <QList>
#include <QListWidgetItem>
#include <QWaitCondition>
#include "../lcedit.h"
#include "ui_search.h"

namespace Search
{
	typedef bool (*Func)(const QString &text, const QByteArray &line);
	enum Mode : int
	{
		Normal = 0,
		ID,
		Function
	};

	class Result : public QListWidgetItem
	{
		public:
		LCTreeWidgetItem *target;
		qint64 lineno;
		Result (LCTreeWidgetItem *target, qint64 lineno, QListWidget *parent) : QListWidgetItem(QString("%1: %2").arg(target->filePath()).arg(lineno), parent), target(target), lineno(lineno) {}
	};
}

namespace Ui {
class LCDlgSearch;
}

class FileSearchPlugin : public QDialog, public LCPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID LCPlugin_Iid FILE "search.json")
	Q_INTERFACES(LCPluginInterface)

public:
	void init(LCEdit *editor) override;
	~FileSearchPlugin() override;
	ExecPolicy createTree(const QDir & base, LCTreeWidgetItem *parent) override;
	ExecPolicy treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous) override;
	std::optional<QIODevice *> getDevice(LCTreeWidgetItem *item) override;
	std::optional<bool> destroyDevice(LCTreeWidgetItem *item, QIODevice *device) override;

public:
	Ui::LCDlgSearch *ui;

public slots:
	void setSearchFunc(Search::Func f);

private:
	LCEdit *m_editor;
	Search::Func searchFunc;
	QWaitCondition condition;
	QThread *searchThread = nullptr;
	bool waitForDevice(QIODevice *device);
	void search(const QString &text);
	void cleanup();
	QList<LCTreeWidgetItem *> getEditorTreeEntries(QTreeWidgetItem *root = nullptr);

private slots:
	void openSearchDialog();
	void searchTermEntered();
	void searchModeChanged(int index);
	void deviceAboutToClose() { condition.wakeAll(); }
	void itemDoubleClicked(QListWidgetItem *item);
};
