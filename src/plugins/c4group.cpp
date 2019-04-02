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

#include <QObject>
#include <QtPlugin>
#include <QtDebug>
#include <QDir>
#include <QFileInfo>
#include <QSharedPointer>
#include <QString>
#include <QSet>
#include <cstring>
#include <limits>
#include "c4group.h"

void C4GroupPlugin::init(LCEdit *editor)
{
	m_editor = editor;
	connect(m_editor->ui->treeWidget, &QTreeWidget::itemCollapsed, this, &C4GroupPlugin::itemCollapsed);
}

ExecPolicy C4GroupPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	Q_UNUSED(base);
	if (parent == nullptr || QFileInfo(parent->filePath()).isDir() || parent->data(Column::Group, Qt::UserRole).canConvert<QSharedPointer<CppC4Group>>())
	{
		return ExecPolicy::Continue;
	}

	bool success = false;
	auto group = QSharedPointer<CppC4Group>::create();

	QIODevicePtr device = m_editor->getDevice(parent);
	if (!device.isNull() && device->open(QIODevice::ReadOnly))
	{
		if (!(success = group->openWithReadCallback(&C4GroupPlugin::readFromDevice, reinterpret_cast<void *>(device.get()))))
		{
			qDebug() << QStringLiteral("Couldn't open group file (code %1): %2")
							.arg(group->getErrorCode())
							.arg(group->getErrorMessage().c_str());
		}
		device->close();
	}

	if (!success)
	{
		return ExecPolicy::Continue;
	}

	createRealTree(parent, group);
	parent->setData(Column::Path, Qt::UserRole, QVariant::fromValue(std::string("")));
	parent->setData(Column::Group, Qt::UserRole, QVariant::fromValue(group));
	return ExecPolicy::AbortMain;
}

void C4GroupPlugin::createRealTree(LCTreeWidgetItem *parent, QSharedPointer<CppC4Group> group, const std::string &path)
{
	std::optional<std::vector<CppC4Group::EntryInfo>> infos = group->getEntryInfos(path);
	if (infos)
	{
		for (auto &info : *infos)
		{
			auto *entry = m_editor->createEntry<QTreeWidgetItem>(parent,
												   QString::fromStdString(info.fileName),
												   QDir(parent->filePath()).absoluteFilePath(QString::fromStdString(info.fileName))
												   );
			entry->setData(Column::Path, Qt::UserRole, QVariant::fromValue(path + '/' + info.fileName));
			entry->setData(Column::Group, Qt::UserRole, QVariant::fromValue(group));
			if (info.directory)
			{
				createRealTree(entry, group, path + '/' + info.fileName);
			}
		}
	}
}

bool C4GroupPlugin::readFromDevice(const void ** const data, size_t * const size, void * const arg)
{
	QIODevice *device = reinterpret_cast<QIODevice *>(arg);
	QByteArray buffer = device->read(BLOCKSIZE);

	void *buf = std::malloc(BLOCKSIZE);
	if (buf == nullptr)
	{
		return true;
	}
	*size = static_cast<size_t>(buffer.size());
	std::memcpy(buf, buffer.constData(), *size);
	*data = buf;
	return device->atEnd();
}

ExecPolicy C4GroupPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	Q_UNUSED(current);
	Q_UNUSED(previous);
	return ExecPolicy::Continue;
}

std::optional<LCDeviceInformation> C4GroupPlugin::getDevice(LCTreeWidgetItem* item)
{
	QSharedPointer<CppC4Group> group;
	std::string path;
	if (!getVars(item, group, path))
	{
		return std::nullopt;
	}

	std::optional<CppC4Group::Data> data = group->getEntryData(path);
	if (!data)
	{
		return std::nullopt;
	}

	auto *device = new QBuffer; // TODO: use different subclasses depending on tmp memory setting
	if (device->open(QIODevice::WriteOnly))
	{
		device->write(static_cast<const char *>(data->data), static_cast<qint64>(data->size));
		device->close();
		return LCDeviceInformation {
			.device = device,
			.deleter = std::bind(&C4GroupPlugin::destroyDevice, this, item, device)
		};
	}
	return std::nullopt;
}

bool C4GroupPlugin::destroyDevice(LCTreeWidgetItem *item, QIODevice *device)
{
	QSharedPointer<CppC4Group> group;
	std::string path;
	if (!getVars(item, group, path))
	{
		return false;
	}

	auto *buffer = qobject_cast<QBuffer *>(device);

	if (buffer == nullptr)
	{
		return false;
	}

	std::optional<CppC4Group::Data> data = group->getEntryData(path);

	if (!data || buffer->data() != QByteArray::fromRawData(reinterpret_cast<const char *>(data->data), static_cast<int>(data->size)))
	{
		qint64 size = buffer->data().size();
		Q_ASSERT(size >= 0 && static_cast<quint64>(size) <= std::numeric_limits<size_t>::max());
		group->setEntryData(path, buffer->data().data(), static_cast<size_t>(size), CppC4Group::MemoryManagement::Copy);
	}
	delete buffer;
	return true;
}

void C4GroupPlugin::itemCollapsed(QTreeWidgetItem *item)
{
	auto *parent = dynamic_cast<LCTreeWidgetItem *>(item);
	if (parent == nullptr)
	{
		return;
	}

	QSharedPointer<CppC4Group> group;
	std::string path;
	if (!getVars(parent, group, path))
	{
		return;
	}

	if (path.length() <= 0)
	{
		qint32 count = parent->childCount();
		parent->deleteChildren();
		parent->setData(Column::Group, Qt::UserRole, QVariant());
		parent->setData(Column::Path, Qt::UserRole, QVariant());
		group->save(parent->filePath().toStdString(), true);

		if (count >= 0)
		{
			parent->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		}
	}
}

bool C4GroupPlugin::getVars(LCTreeWidgetItem *item, QSharedPointer<CppC4Group> &group, std::string &path)
{
	QVariant var = item->data(Column::Group, Qt::UserRole);
	if (!var.canConvert<QSharedPointer<CppC4Group>>())
	{
		return false;
	}
	group = var.value<QSharedPointer<CppC4Group>>();
	var = item->data(Column::Path, Qt::UserRole);
	if (!var.canConvert<std::string>())
	{
		return false;
	}
	path = var.value<std::string>();
	return true;
}
