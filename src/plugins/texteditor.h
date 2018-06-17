#pragma once
#undef new
#undef delete
#undef LineFeed

#include <QDir>
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
	void init(LCEdit *editor) override;
	ExecPolicy createTree(const QDir & base, LCTreeWidgetItem * parent) override;
	int priority() override;
	ExecPolicy treeItemChanged(LCTreeWidgetItem * current, LCTreeWidgetItem * previous) override;
	ReturnValue<QByteArray> fileRead(LCTreeWidgetItem *item, off_t offset, size_t size) override;
	ReturnValue<int> fileWrite(LCTreeWidgetItem *item, const QByteArray &buf) override;
	
private:
	KTextEditor::Editor *m_texteditor;
	KTextEditor::View *m_textview;
	KTextEditor::Document *m_document;
	LCEdit *m_editor;
	
};
