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
	Q_PLUGIN_METADATA(IID LCPlugin_Iid)
	Q_INTERFACES(LCPluginInterface)

public:
	void init(LCEdit *editor) override;
	~FileSearchPlugin() override;
	ExecPolicy createTree(const QDir & base, LCTreeWidgetItem *parent) override;
	int priority() override;
	ExecPolicy treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous) override;
	ReturnValue<QIODevice *> getDevice(LCTreeWidgetItem *item) override;
	ReturnValue<bool> destroyDevice(LCTreeWidgetItem *item, QIODevice *device) override;

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
