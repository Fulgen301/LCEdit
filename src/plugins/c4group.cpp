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
}

ExecPolicy C4GroupPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	if (parent == nullptr)
		return ExecPolicy::Continue;

	auto *grp = new C4Group(parent->filePath());
	try
	{
		grp->open();
	}
	catch (C4GroupException e)
	{
		return ExecPolicy::Continue;
	}

	if (!grp->isPacked())
	{
		delete grp;
		return ExecPolicy::Continue;
	}

	createRealTree(parent, grp->root);

	return ExecPolicy::AbortMain;
}

void C4GroupPlugin::createRealTree(LCTreeWidgetItem *parent, C4GroupDirectory *dir)
{
	foreach (C4GroupEntry *e, dir->children)
	{
		LCTreeWidgetItem *entry = m_editor->createEntry<LCTreeWidgetItem>(parent, e->fileName, QDir(parent->filePath()).absoluteFilePath(e->fileName));
		map[entry] = e;
		auto *d = dynamic_cast<C4GroupDirectory *>(e);
		if (d != nullptr)
		{
			createRealTree(entry, d);
		}
	}
}

ExecPolicy C4GroupPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	if (current == nullptr || previous == nullptr || !map.contains(previous))
		return ExecPolicy::Continue;

	if (map.contains(current) && &(*(map[current]->group)) == &(*(map[previous]->group)))
		return ExecPolicy::Continue;

	C4Group *grp = map[previous]->group;
	if (grp->isPacked())
	{
		grp->pack();
	}

	grp->close();
	delete grp;
	map.remove(previous);
	return ExecPolicy::Continue;
}

int C4GroupPlugin::priority()
{
	return 1;
}

ReturnValue<QIODevice *> C4GroupPlugin::getDevice(LCTreeWidgetItem* item)
{
	if (!map.contains(item))
	{
		return ReturnValue<QIODevice *>();
	}

	auto *file = dynamic_cast<C4GroupFile *>(map[item]);
	if (file == nullptr || file->device == nullptr)
	{
		return ReturnValue<QIODevice *>();
	}

	return ReturnValue<QIODevice *>(ExecPolicy::AbortAll, file->device);
}

ReturnValue<bool> C4GroupPlugin::destroyDevice(LCTreeWidgetItem *item, QIODevice *device)
{
	Q_UNUSED(device);
	// we don't destroy the device here, this is C4Group's concern, NOT OURS
	if (map.contains(item))
	{
		auto *f = dynamic_cast<C4GroupFile *>(map[item]);
		if (f != nullptr && f->device == device)
		{
			map.remove(item);
			return ReturnValue<bool>(ExecPolicy::AbortAll, true);
		}
	}
	return ReturnValue<bool>(ExecPolicy::Continue, false);
}
