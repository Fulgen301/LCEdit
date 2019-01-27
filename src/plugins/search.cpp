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

#include <QMimeDatabase>
#include <QMutex>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QStringList>
#include <QTextCursor>
#include <QThread>
#include "search.h"

void FileSearchPlugin::init(LCEdit *editor)
{
	m_editor = editor;
	QAction *actSearch = m_editor->ui->menuActions->addAction(m_editor->tr("In Dateien suchen..."));
	connect(actSearch, &QAction::triggered, this, &FileSearchPlugin::openSearchDialog);

	ui = new Ui::LCDlgSearch;
	ui->setupUi(this);
	connect(ui->btnStart, &QPushButton::clicked, this, &FileSearchPlugin::searchTermEntered);

	QStringList modes;
	modes << m_editor->tr("Normal") << m_editor->tr("IDs") << m_editor->tr("Funktionen");
	ui->cmbMode->addItems(modes);
	connect(ui->cmbMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FileSearchPlugin::searchModeChanged);
	ui->cmbMode->setCurrentIndex(0);
	searchModeChanged(0);

	connect(ui->lstResults, &QListWidget::itemDoubleClicked, this, &FileSearchPlugin::itemDoubleClicked);
}

FileSearchPlugin::~FileSearchPlugin()
{
	cleanup();
	SAFE_DELETE(searchThread)
	delete ui;
}

ExecPolicy FileSearchPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	Q_UNUSED(base);
	Q_UNUSED(parent);
	return ExecPolicy::Continue;
}

ExecPolicy FileSearchPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	Q_UNUSED(current);
	Q_UNUSED(previous);
	return ExecPolicy::Continue;
}

ReturnValue<QIODevice *> FileSearchPlugin::getDevice(LCTreeWidgetItem *item)
{
	Q_UNUSED(item);
	return ReturnValue<QIODevice *>();
}

ReturnValue<bool> FileSearchPlugin::destroyDevice(LCTreeWidgetItem *item, QIODevice *device)
{
	Q_UNUSED(item);
	Q_UNUSED(device);
	return ReturnValue<bool>(ExecPolicy::Continue, false);
}

void FileSearchPlugin::setSearchFunc(Search::Func f)
{
	searchFunc = f;
}

bool FileSearchPlugin::waitForDevice(QIODevice *device)
{
	if (device == nullptr)
	{
		return false;
	}

	if (device->isOpen())
	{
		QMutex mutex;
		mutex.lock();
		connect(device, &QIODevice::aboutToClose, this, &FileSearchPlugin::deviceAboutToClose);
		bool ret = condition.wait(&mutex, 2);
		disconnect(device, &QIODevice::aboutToClose, this, &FileSearchPlugin::deviceAboutToClose);
		mutex.unlock();
		return ret;
	}
	return true;
}

void FileSearchPlugin::cleanup()
{
	QListWidgetItem *item = nullptr;
	while (item = ui->lstResults->takeItem(0))
	{
		SAFE_DELETE(item)
	}
}

#define ADD_TO_LIST(x) list.append(dynamic_cast<LCTreeWidgetItem *>(x)); list.append(getEditorTreeEntries(x));

QList<LCTreeWidgetItem *> FileSearchPlugin::getEditorTreeEntries(QTreeWidgetItem *root)
{
	QList<LCTreeWidgetItem *> list;
	if (root == nullptr)
	{
		for (auto i = 0; i < m_editor->ui->treeWidget->topLevelItemCount(); i++)
		{
			ADD_TO_LIST(m_editor->ui->treeWidget->topLevelItem(i))
		}
		return list;
	}
	for (auto i = 0; i < root->childCount(); i++)
	{
		ADD_TO_LIST(root->child(i))
	}
	return list;
}

void FileSearchPlugin::openSearchDialog()
{
	show();
	exec();
}

void FileSearchPlugin::searchTermEntered()
{
	if (searchThread != nullptr || ui->txtSearchTerm->text().length() == 0)
	{
		return;
	}
	ui->txtSearchTerm->setEnabled(false);
	ui->cmbMode->setEnabled(false);
	cleanup();
	searchThread = QThread::create([this](){ search(ui->txtSearchTerm->text()); });
	connect(searchThread, &QThread::finished, [this]() { ui->txtSearchTerm->setEnabled(true); });
	connect(searchThread, &QThread::finished, [this]() { ui->cmbMode->setEnabled(true); });
	connect(searchThread, &QThread::finished, [this]() { searchThread->deleteLater(); searchThread = nullptr; });
	searchThread->start();
}

void FileSearchPlugin::searchModeChanged(int index)
{
	switch (index)
	{
	case Search::Mode::Normal:
		setSearchFunc([](const QString &term, const QByteArray &line)
		{
			return line.contains(QFile::encodeName(term));
		});
		break;

	case Search::Mode::ID:
		setSearchFunc([](const QString &term, const QByteArray &line)
		{
			QString regex = QStringLiteral(R"((?:^id=(%1)$|CreateObject\((%1)(,.*|\))))").arg(term);
			return QRegularExpression(regex).match(line).hasMatch();
		});
		break;

	case Search::Mode::Function:
		setSearchFunc([](const QString &term, const QByteArray &line)
		{
			QString regex = QStringLiteral(R"(func (%1))").arg(term);
			return QRegularExpression(regex).match(line).hasMatch();
		});
	}
}

void FileSearchPlugin::search(const QString &text)
{
	foreach (QTreeWidgetItem *i, getEditorTreeEntries())
	{
		auto *item = dynamic_cast<LCTreeWidgetItem *>(i);
		if (item == nullptr)
		{
			continue;
		}
		qDebug() << item->text(0);
		QIODevice *device = m_editor->getDevice(item);
		if (!waitForDevice(device))
		{
			continue;
		}
		if (device->open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QString name = item->text(0);
			QByteArray buf = device->peek(20);
			if (QMimeDatabase().mimeTypeForFileNameAndData(item->text(0), buf).inherits("text/plain"))
			{
				qint64 lineno = 1;
				do
				{
					QByteArray line = device->readLine();
					if (searchFunc(text, line))
					{
						auto *res = new Search::Result(item, lineno, ui->lstResults);
						ui->lstResults->addItem(dynamic_cast<QListWidgetItem *>(res));
					}
					lineno++;
				}
				while (!device->atEnd());
			}
			device->close();
		}
		m_editor->destroyDevice(item, device);
	}
}

void FileSearchPlugin::itemDoubleClicked(QListWidgetItem *item)
{
	auto *res = dynamic_cast<Search::Result *>(item);
	if (res == nullptr)
	{
		return;
	}

	m_editor->ui->treeWidget->setCurrentItem(dynamic_cast<QTreeWidgetItem *>(res->target));
	QTextCursor cursor;
	for (decltype(res->lineno) i = 0; i < res->lineno; i++)
	{
		cursor.movePosition(QTextCursor::Down);
	}

	cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
	m_editor->ui->txtDescription->setTextCursor(cursor);
}
