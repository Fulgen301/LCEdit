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
		return EP_Continue;
	if (parent->childCount() == 0)
	{
		QString suffix = QFileInfo(parent->filePath()).completeSuffix();
		if (QSet<QString>({"c4d", "c4f", "c4s", "c4p"}).contains(suffix))
		{
			C4Group grp(parent->filePath());
			try
			{
				grp.open<QTemporaryFile>();
				qDebug() << qobject_cast<QTemporaryFile *>(grp.content)->fileName();
			}
			catch (C4GroupException e)
			{
				qCritical() << "C4Group:" << e.getMessage();
				return EP_Continue;
			}

			qDebug() << "here";

			assert(grp.root->children.length());
			foreach (C4GroupEntry *e, grp.root->children)
			{
				m_editor->createEntry<LCTreeWidgetItem>(parent, e->filename, QDir(parent->filePath()).filePath(e->filename));
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
#if 0
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
#endif
}

int C4GroupPlugin::priority()
{
	return 1;
}

ReturnValue<QByteArray> C4GroupPlugin::fileRead(LCTreeWidgetItem* item, off_t offset, size_t size)
{
	QFileInfo info(item->filePath());
	C4Group grp(info.filePath());
	try
	{
		grp.open<QBuffer>();
	}
	catch (C4GroupException e)
	{
		return ReturnValue<QByteArray>();
	}
	auto *f = dynamic_cast<C4GroupFile *>(grp.getChildByGroupPath(info.fileName()));
	if (f == nullptr)
	{
		return ReturnValue<QByteArray>();
	}
	qint64 pos = f->pos();
	f->seek(offset);
	QByteArray buf = f->read(size);
	f->seek(pos);
	return ReturnValue<QByteArray>(EP_AbortAll, buf);
}

ReturnValue<int> C4GroupPlugin::fileWrite(LCTreeWidgetItem *item, const QByteArray &buf, off_t offset)
{
	return ReturnValue<int>(EP_Continue, 0);
}
