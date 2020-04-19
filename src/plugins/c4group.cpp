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
#include <QMessageBox>
#include <QSharedPointer>
#include <QString>
#include <QSet>
#include <cstring>
#include <limits>
#include "c4group.h"

void C4GroupPlugin::init(LCEdit *editor)
{
	m_editor = editor;

	auto *menu = new QMenu("C4Group");
	actSave = menu->addAction(tr("Speichern"), this, &C4GroupPlugin::saveGroup);
	actSave->setDisabled(true);

	actClose = menu->addAction(tr("Schließen"), this, [this]() { closeGroup(dynamic_cast<LCTreeWidgetItem *>(m_editor->ui->treeWidget->currentItem()), true); });
	actClose->setDisabled(true);

	m_editor->ui->menuActions->addMenu(menu);
}

ExecPolicy C4GroupPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	Q_UNUSED(base);
	if (parent == nullptr || QFileInfo(parent->filePath()).isDir() || parent->data(Column::Group, Qt::UserRole).canConvert<CppC4GroupPtr>())
	{
		return ExecPolicy::Continue;
	}

	bool success = false;
	auto group = CppC4GroupPtr::create(false);

	if (QIODevicePtr device = m_editor->getDevice(parent); !device.isNull() && device->open(QIODevice::ReadOnly))
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

void C4GroupPlugin::createRealTree(LCTreeWidgetItem *parent, CppC4GroupPtr group, const std::string &path)
{
	if (auto infos = group->getEntryInfos(path); infos)
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

	if (current == nullptr)
	{
		return ExecPolicy::Continue;
	}

	CppC4GroupPtr group;

	bool enabled = getVars(current, &group);
	actSave->setEnabled(enabled);
	actClose->setEnabled(enabled);

	return ExecPolicy::Continue;
}

std::optional<LCDeviceInformation> C4GroupPlugin::getDevice(LCTreeWidgetItem* item)
{
	CppC4GroupPtr group;
	std::string path;
	if (!getVars(item, &group, &path))
	{
		return std::nullopt;
	}

	std::optional<CppC4Group::Data> data = group->getEntryData(path);
	if (!data)
	{
		return std::nullopt;
	}

	if (auto device = new QBuffer; device->open(QIODevice::WriteOnly)) // TODO: use different subclasses depending on tmp memory setting
	{
		device->write(static_cast<const char *>(data->data), static_cast<qint64>(data->size));
		device->close();
		return LCDeviceInformation{device, std::bind(&C4GroupPlugin::destroyDevice, this, item, device)};
	}
	return std::nullopt;
}

bool C4GroupPlugin::destroyDevice(LCTreeWidgetItem *item, QIODevice *device)
{
	CppC4GroupPtr group;
	std::string path;
	if (!getVars(item, &group, &path))
	{
		return false;
	}

	auto *buffer = qobject_cast<QBuffer *>(device);

	if (buffer == nullptr)
	{
		return false;
	}

	if (auto data = group->getEntryData(path);
			!data || buffer->data() != QByteArray::fromRawData(static_cast<const char *>(data->data), static_cast<int>(data->size)))
	{
		qint64 size = buffer->data().size();
		Q_ASSERT(size >= 0 && static_cast<quint64>(size) <= std::numeric_limits<size_t>::max());

		group->setEntryData(path, buffer->data().constData(), static_cast<size_t>(size), CppC4Group::MemoryManagement::Copy);
		delete buffer;
	}
	return true;
}

bool C4GroupPlugin::getVars(LCTreeWidgetItem *item, CppC4GroupPtr *group, std::string *path)
{
	if (group)
	{
		QVariant var = item->data(Column::Group, Qt::UserRole);
		if (!var.canConvert<CppC4GroupPtr>())
		{
			return false;
		}

		*group = var.value<CppC4GroupPtr>();
	}

	if (path)
	{
		QVariant var = item->data(Column::Path, Qt::UserRole);
		if (!var.canConvert<std::string>())
		{
			return false;
		}

		*path = var.value<std::string>();
	}

	return true;
}

void C4GroupPlugin::closeGroup(LCTreeWidgetItem *item, bool confirmSave)
{
	CppC4GroupPtr group;
	std::string path;
	if (item = getGroupRootItem(item, &group, &path); !item)
	{
		return;
	}

	if (confirmSave)
	{
		switch (QMessageBox::information(
					nullptr,
					tr("Gruppe schließen"),
					tr("Soll die Gruppe gespeichert werden?"),
					QMessageBox::Save | QMessageBox::Discard | QMessageBox::Abort,
					QMessageBox::Abort))
		{
		case QMessageBox::Save:
			group->save("", true);
			Q_FALLTHROUGH();

		case QMessageBox::Discard:
			break;

		default:
			return;
		}
	}

	qint32 count = item->childCount();
	item->deleteChildren();
	item->setData(Column::Group, Qt::UserRole, QVariant());
	item->setData(Column::Path, Qt::UserRole, QVariant());

	if (count >= 0)
	{
		item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	}
}

LCTreeWidgetItem *C4GroupPlugin::getGroupRootItem(LCTreeWidgetItem *item, CppC4GroupPtr *group, std::string *path)
{
	if (!getVars(item, group, path))
	{
		return nullptr;
	}

	while (path->length() > 0)
	{
		item = dynamic_cast<LCTreeWidgetItem *>(item->parent());
		Q_ASSERT(item);

		if (!getVars(item, nullptr, path))
		{
			return nullptr;
		}
	}

	return item;
}

void C4GroupPlugin::saveGroup()
{
	CppC4GroupPtr group;
	std::string path;
	if (!getVars(dynamic_cast<LCTreeWidgetItem *>(m_editor->ui->treeWidget->currentItem()), &group, &path))
	{
		return;
	}

	group->save(path, true);
}
