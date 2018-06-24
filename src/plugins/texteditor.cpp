#include <QMimeType>
#include <QMimeDatabase>
#include <QHash>
#include <QUrl>
#include <QApplication>
#include "texteditor.h"

ExecPolicy TextEditorPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	return EP_Continue;
}

ReturnValue<QByteArray> TextEditorPlugin::fileRead(LCTreeWidgetItem *item, off_t offset, size_t size)
{
	return ReturnValue<QByteArray>();
}

ReturnValue<int> TextEditorPlugin::fileWrite(LCTreeWidgetItem *item, const QByteArray &buf, off_t offset)
{
	return ReturnValue<int>(EP_Continue, 0);
}

void TextEditorPlugin::init(LCEdit *editor)
{
	m_editor = editor;
	m_texteditor = KTextEditor::Editor::instance();
	m_document = m_texteditor->createDocument(this);
	m_textview = m_document->createView(m_editor);
	m_editor->ui->layRight->addWidget(m_textview);
	m_textview->hide();
}

TextEditorPlugin::~TextEditorPlugin()
{
//	SAFE_DELETE(m_textview)
	SAFE_DELETE(m_document)
//	SAFE_DELETE(m_texteditor)
}

int TextEditorPlugin::priority()
{
	return 1;
}

ExecPolicy TextEditorPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	SAFE_DELETE(watcher)
	if (current)
	{
		QFileInfo info(current->text(0));
//		QByteArray text = m_editor->fileRead(current, 0, 24);
//		QMimeType type = QMimeDatabase().mimeTypeForFileNameAndData(file.fileName(), text);
		if (QSet<QString>({"c", "log", "txt", "c4m"}).contains(info.completeSuffix()))
		{
			file.open();
			file.write(m_editor->fileRead(current, 0, 0));
			file.resize(file.pos());
			file.close();

			watcher = new QFileSystemWatcher();
			watcher->addPath(file.fileName());
			QObject::connect(watcher, &QFileSystemWatcher::fileChanged, [this, current](const QString &path)
			{
				if (path == file.fileName())
				{
					QFile f(path);
					f.open(QIODevice::ReadOnly | QIODevice::Text);
					QByteArray buf = f.readAll();
					f.close();
					m_editor->fileWrite(current, buf, 0);
				}
			});
			m_document->openUrl(QUrl::fromLocalFile(file.fileName()));
			m_document->setHighlightingMode(m_document->highlightingModes().contains(info.baseName()) ? info.baseName() : info.completeSuffix() == "c" ? "C4Script" : "Normal");
			m_editor->ui->lblName->hide();
			m_editor->ui->txtDescription->hide();
			m_textview->show();
			return EP_AbortAll;
		}
	}

	m_textview->hide();
	m_editor->ui->lblName->show();
	m_editor->ui->txtDescription->show();
	return EP_Continue;
}
