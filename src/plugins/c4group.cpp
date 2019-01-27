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
#include <QString>
#include <QSet>
#include <new>
#include "c4group.h"

void C4GroupPlugin::init(LCEdit *editor)
{
	m_editor = editor;
	connect(m_editor->ui->treeWidget, &QTreeWidget::itemCollapsed, this, &C4GroupPlugin::itemCollapsed);
}

ExecPolicy C4GroupPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	Q_UNUSED(base);
	if (parent == nullptr || QFileInfo(parent->filePath()).isDir() || parent->data(2, Qt::UserRole).canConvert<C4GroupEntry *>())
		return ExecPolicy::Continue;

	auto *grp = new C4Group(parent->filePath());
	try
	{
		grp->open();
	}
	catch (C4GroupException e)
	{
		delete grp;
		return ExecPolicy::Continue;
	}

	createRealTree(parent, grp->root);
	rootItems[parent] = grp;
	return ExecPolicy::AbortMain;
}

void C4GroupPlugin::createRealTree(LCTreeWidgetItem *parent, C4GroupDirectory *dir)
{
	for (QObject *e : qAsConst(dir->children()))
	{
		auto *entry = qobject_cast<C4GroupEntry *>(e);
		LCTreeWidgetItem *item = m_editor->createEntry<LCTreeWidgetItem>(parent, entry->fileName, QDir(parent->filePath()).absoluteFilePath(entry->fileName));
		item->setData(2, Qt::UserRole, QVariant::fromValue<C4GroupEntry *>(entry));
		auto *d = qobject_cast<C4GroupDirectory *>(e);
		if (d != nullptr)
		{
			createRealTree(item, d);
		}
	}
}

ExecPolicy C4GroupPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	Q_UNUSED(current);
	Q_UNUSED(previous);
	return ExecPolicy::Continue;
}

ReturnValue<QIODevice *> C4GroupPlugin::getDevice(LCTreeWidgetItem* item)
{
	QVariant role = item->data(2, Qt::UserRole);
	if (!role.canConvert<C4GroupEntry *>())
	{
		return ReturnValue<QIODevice *>();
	}

	auto *file = qobject_cast<C4GroupFile *>(role.value<C4GroupEntry *>());
	if (file == nullptr || Q_UNLIKELY(file->device == nullptr))
	{
		return ReturnValue<QIODevice *>();
	}

	return ReturnValue<QIODevice *>(ExecPolicy::AbortAll, file->device);
}

ReturnValue<bool> C4GroupPlugin::destroyDevice(LCTreeWidgetItem *item, QIODevice *device)
{
	Q_UNUSED(device);

	// we don't destroy the device here, this is C4Group's concern, NOT OURS
	QVariant role = item->data(2, Qt::UserRole);
	if (role.canConvert<C4GroupEntry *>())
	{
		auto *f = qobject_cast<C4GroupFile *>(role.value<C4GroupEntry *>());
		if (f != nullptr && f->device == device)
		{
			return ReturnValue<bool>(ExecPolicy::AbortAll, true);
		}
	}

	return ReturnValue<bool>(ExecPolicy::Continue, false);
}

void C4GroupPlugin::itemCollapsed(QTreeWidgetItem *item)
{
	auto *parent = dynamic_cast<LCTreeWidgetItem *>(item);
	if (parent == nullptr || !rootItems.contains(parent))
	{
		return;
	}
	C4Group *group = rootItems[parent];
	group->close();
	delete group;
	rootItems.remove(parent);
	parent->deleteChildren();
}
