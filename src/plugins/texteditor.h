#pragma once
#undef new
#undef delete
#undef LineFeed

#include <QDir>
#include <QFileSystemWatcher>
#include <QTemporaryFile>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include "../include.h"
#include "../lcedit.h"

class TextEditorPlugin : public QObject, public LCPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID LCPlugin_Iid)
	Q_INTERFACES(LCPluginInterface)

public:
	~TextEditorPlugin() override;
	void init(LCEdit *editor) override;
	ExecPolicy createTree(const QDir & base, LCTreeWidgetItem * parent) override;
	int priority() override;
	ExecPolicy treeItemChanged(LCTreeWidgetItem * current, LCTreeWidgetItem * previous) override;
	ReturnValue<QIODevice *> getDevice(LCTreeWidgetItem *item) override;
	ReturnValue<bool> destroyDevice(LCTreeWidgetItem *item, QIODevice *device) override;

private:
	KTextEditor::Editor *m_texteditor = nullptr;
	KTextEditor::View *m_textview = nullptr;
	KTextEditor::Document *m_document = nullptr;
	LCEdit *m_editor;
	QTemporaryFile file;
	QFileSystemWatcher *watcher = nullptr;
};
