#include <QObject>
#include <QtPlugin>
#include <QtDebug>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QSet>
#include "c4group.h"

void C4GroupPlugin::init(LCEdit *editor)
{
	m_editor = editor;
}

ExecPolicy C4GroupPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	if (parent == nullptr)
		return EP_Continue;
	if (parent->childCount() == 0)
	{
		QString suffix = QFileInfo(parent->filePath()).completeSuffix();
		if (QSet<QString>({"c4d", "c4f", "c4s", "c4p"}).contains(suffix))
		{
			C4Group grp;
			try
			{
				openChildGroup(&grp, parent->filePath());
			}
			catch (QString e)
			{
				return parent->childCount() ? EP_AbortMain : EP_Continue;
			}

			char filename[_MAX_PATH + 1];
			grp.FindEntry("*", filename);
			while (grp.FindNextEntry("*", filename))
			{
				m_editor->createEntry<LCTreeWidgetItem>(parent, QString(filename), QDir(parent->filePath()).filePath(filename));
			}

			return EP_AbortMain;
		}
	}
	return EP_Continue;
}

ExecPolicy C4GroupPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	return EP_Continue;
}

void C4GroupPlugin::openChildGroup(C4Group* grp, const QString& path)
{
	if (!grp->IsOpen())
	{
		grp->Open(TO_CSTR(GetFirstExistingPath(QFileInfo(path))));
	}

	QString notOpenedPath = path.right(path.length() - grp->GetFullName().getLength());
	notOpenedPath = notOpenedPath.remove(0, 1);

	foreach(const QString &part, notOpenedPath.split(QDir::separator(), QString::SkipEmptyParts))
	{
		if (!grp->OpenChild(TO_CSTR(part)))
		{
			throw QString("C4Group error.");
		}
	}
}

int C4GroupPlugin::priority()
{
	return 1;
}

ReturnValue<QByteArray> C4GroupPlugin::fileRead(LCTreeWidgetItem* item, off_t offset, size_t size)
{
	C4Group grp;
	QFileInfo info(item->filePath());
	try
	{
		openChildGroup(&grp, info.path());
		if (!(grp.IsPacked() && grp.AccessEntry(TO_CSTR(info.fileName()))))
			throw QString("C4Group error.");
	}
	catch (QString e)
	{
		if (e == "C4Group error.")
		{
			return ReturnValue<QByteArray>();
		}
	}

	grp.Advance(offset);
	size_t realSize = size ? qMin<size_t>(size, grp.AccessedEntrySize()) : grp.AccessedEntrySize();
	if (offset + realSize > grp.AccessedEntrySize())
		realSize = static_cast<size_t>(qMax<int>(grp.AccessedEntrySize() - offset, 0)); // size_t is unsigned, so qMax<size_t> fails if one argument is negative

	if (realSize == 0)
		return ReturnValue<QByteArray>(EP_AbortAll, QByteArray());

	char buffer[realSize];
	grp.Read(buffer, realSize);
	return ReturnValue<QByteArray>(EP_AbortAll, QByteArray(buffer, realSize));
}

ReturnValue<int> C4GroupPlugin::fileWrite(LCTreeWidgetItem *item, const QByteArray &buf, off_t offset)
{
	return ReturnValue<int>(EP_Continue, 0);
}
