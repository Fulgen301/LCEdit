#include "texteditor.h"

ExecPolicy TextEditorPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	return EP_Continue;
}

ReturnValue<QByteArray> TextEditorPlugin::fileRead(LCTreeWidgetItem *item, off_t offset, size_t size)
{
	return ReturnValue<QByteArray>();
}

ReturnValue<int> TextEditorPlugin::fileWrite(LCTreeWidgetItem *item, const QByteArray &buf)
{
	return ReturnValue<int>(EP_Continue, 0);
}

void TextEditorPlugin::init(LCEdit *editor)
{
	m_editor = editor;
	m_texteditor = KTextEditor::Editor::instance();
	m_document = m_texteditor->createDocument(this);
	m_textview = m_document->createView(editor);
	editor->ui->layRight->addWidget(m_textview);
	m_textview->hide();
}

int TextEditorPlugin::priority()
{
	return 1;
}

ExecPolicy TextEditorPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	if (current && QSet<QString>({"txt", "c", "log"}).contains(QFileInfo(current->text(0)).completeSuffix()))
	{
		QByteArray text = m_editor->fileRead(current, 0, 0);
		m_document->setText(QString(text));
		m_editor->ui->lblName->hide();
		m_editor->ui->txtDescription->hide();
		m_textview->show();
		return EP_AbortAll;
	}
	else
	{
		m_textview->hide();
		m_editor->ui->lblName->show();
		m_editor->ui->txtDescription->show();
		return EP_Continue;
	}
}
