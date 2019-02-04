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
	Q_PLUGIN_METADATA(IID LCPlugin_Iid FILE "texteditor.json")
	Q_INTERFACES(LCPluginInterface)

public:
	~TextEditorPlugin() override;
	void init(LCEdit *editor) override;
	ExecPolicy createTree(const QDir & base, LCTreeWidgetItem * parent) override;
	ExecPolicy treeItemChanged(LCTreeWidgetItem * current, LCTreeWidgetItem * previous) override;
	std::optional<QIODevice *> getDevice(LCTreeWidgetItem *item) override;
	std::optional<bool> destroyDevice(LCTreeWidgetItem *item, QIODevice *device) override;

private:
	KTextEditor::Editor *m_texteditor = nullptr;
	KTextEditor::View *m_textview = nullptr;
	KTextEditor::Document *m_document = nullptr;
	LCEdit *m_editor;
	QTemporaryFile file;
	QFileSystemWatcher *watcher = nullptr;
};
