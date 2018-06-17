#include <QObject>
#include <QtPlugin>
#include <QtDebug>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QSet>
#include <QMutexLocker>
#include "c4group.h"

void C4GroupPlugin::init(LCEdit *editor)
{
	m_editor = editor;
}

ExecPolicy C4GroupPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	return EP_Continue;
}

ExecPolicy C4GroupPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	if (current == nullptr)
		return EP_Continue;
	
	if (current->childCount() == 0)
	{	QString suffix = QFileInfo(current->filePath()).completeSuffix();
		if (QSet<QString>({"c4d", "c4f", "c4s", "c4p"}).contains(suffix))
		{
			QMutexLocker locker(&m_editor->mutex);
			
			// C4Group's path is the root folder where we reside, so we need to access the entry first
			C4Group grp;
			openChildGroup(&grp, current->filePath());
			
			char filename[_MAX_PATH + 1];
			grp.FindEntry("*", filename);
			assert(filename);
			while (grp.FindNextEntry("*", filename))
			{
				m_editor->createEntry<LCTreeWidgetItem>(current, QString(filename), QDir(current->filePath()).filePath(filename));
			}
			
			return EP_AbortMain;
		}
	}
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
	char buffer[size];
	grp.Read(buffer, size ? size : grp.EntrySize());
	return ReturnValue<QByteArray>(EP_AbortAll, QByteArray(buffer));
}

ReturnValue<int> C4GroupPlugin::fileWrite(LCTreeWidgetItem *item, const QByteArray &buf)
{
	return ReturnValue<int>(EP_Continue, 0);
}
