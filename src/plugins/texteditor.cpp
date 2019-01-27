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

#include <QMimeType>
#include <QMimeDatabase>
#include <QHash>
#include <QUrl>
#include <QApplication>
#include "texteditor.h"

ExecPolicy TextEditorPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	return ExecPolicy::Continue;
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

ExecPolicy TextEditorPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	SAFE_DELETE(watcher)
	if (current)
	{
		QIODevice *device = m_editor->getDevice(current);
		if (device != nullptr)
		{
			if (device->open(QIODevice::ReadOnly))
			{
				QFileInfo info(current->filePath());
				if (QMimeDatabase().mimeTypeForFileNameAndData(info.fileName(), device->peek(20)).inherits("text/plain"))
				{
					QString filename;
					if (qobject_cast<QFile *>(device) == nullptr)
					{
						device->open(QIODevice::ReadOnly);
						Q_ASSERT(device->isOpen());
						file.open();
						file.resize(0);
						TRANSFER_CONTENTS(*device, file)
						file.close();
						device->close();
						filename = file.fileName();
					}
					else
					{
						filename = qobject_cast<QFile *>(device)->fileName();
					}
					filename = QFileInfo(filename).absoluteFilePath();
					watcher = new QFileSystemWatcher();
					watcher->addPath(filename);
					QObject::connect(watcher, &QFileSystemWatcher::fileChanged, [this, current, filename](const QString &path)
					{
						if (path == filename)
						{
							QIODevice *device = m_editor->getDevice(current);
							if (device == nullptr)
							{
								qWarning() << "TextEditor: No device for file";
								m_document->documentSaveAs();
							}
							if (qobject_cast<QFile *>(device) != nullptr)
							{
								return; // no need to transfer anything, contents got written to the device
							}

							QFile f(filename);
							f.open(QIODevice::ReadOnly);
							device->open(QIODevice::WriteOnly);
							TRANSFER_CONTENTS(f, *device)
							device->close();
							f.close();
							m_editor->destroyDevice(current, device);
						}
					});
					m_document->setReadWrite(true);
					m_document->openUrl(QUrl::fromLocalFile(filename));
					m_document->setHighlightingMode(m_document->highlightingModes().contains(info.baseName()) ? info.baseName() : info.completeSuffix() == "c" ? "C4Script" : "Normal");
					m_editor->ui->lblName->hide();
					m_editor->ui->txtDescription->hide();
					m_textview->show();
					m_editor->destroyDevice(current, device);
					return ExecPolicy::AbortAll;
				}
				device->close();
			}
		}
		m_editor->destroyDevice(current, device);
	}

	m_textview->hide();
	m_editor->ui->lblName->show();
	m_editor->ui->txtDescription->show();
	return ExecPolicy::Continue;
}

ReturnValue<QIODevice *> TextEditorPlugin::getDevice(LCTreeWidgetItem *item)
{
	Q_UNUSED(item);
	return ReturnValue<QIODevice *>();
}

ReturnValue<bool> TextEditorPlugin::destroyDevice(LCTreeWidgetItem *item, QIODevice *device)
{
	Q_UNUSED(item);
	Q_UNUSED(device);
	return ReturnValue<bool>(ExecPolicy::Continue, false);
}
