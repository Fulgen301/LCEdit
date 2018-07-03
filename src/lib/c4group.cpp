#include <QDir>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>
#include <cstring>
#include "./c4group.h"

QDataStream &operator <<(QDataStream &stream, const NullBytes &n)
{
	for (uint i = 0; i < n.count; i++)
	{
		stream << '\0';
	}
	return stream;
}

QDataStream &operator >>(QDataStream &stream, const NullBytes &n)
{
	qint8 tmp;
	for (uint i = 0; i < n.count; i++)
	{
		stream >> tmp;
	}
	return stream;
}

QDataStream &operator <<(QDataStream &stream, const char &c)
{
	stream.writeRawData(&c, 1);
	return stream;
}

QDataStream &operator >>(QDataStream &stream, char &c)
{
	stream.readRawData(&c, 1);
	return stream;
}

QDataStream &operator <<(QDataStream &stream, const C4GroupEntry &entry)
{
	stream.setVersion(QDataStream::Qt_1_0);
	if (entry.parent == nullptr) // top level group folder
		return stream;

	stream.writeRawData(entry.filename, 256);
	stream << NullBytes(8);
	if (dynamic_cast<const C4GroupDirectory *>(&entry))
	{
		stream << (int32_t) 1;
	}
	else
	{
		stream << (int32_t) 0;
	}
	stream << entry.size() << NullBytes(4) << entry.relativeContentPosition;
	stream << (qint32) entry.lastModification << entry.checksumType << entry.checksum;
	stream << entry.executable;
	stream << NullBytes(26);
	return stream;
}

QDataStream &operator <<(QDataStream &stream, C4GroupFile &entry)
{
	stream << *dynamic_cast<const C4GroupEntry *>(&entry);
	QIODevice *device = stream.device();
	qint64 devicePos = device->pos();
	if (device->size() <= entry.contentPosition())
	{
		device->seek(device->size());
		stream << NullBytes(entry.contentPosition() - device->size());
	}
	device->seek(entry.contentPosition());
	qint64 filePos = entry.pos();
	entry.seek(0);
	TRANSFER_CONTENTS(entry, *device)
	device->seek(devicePos);
	entry.seek(filePos);
	return stream;
}

QDataStream &operator <<(QDataStream &stream, const C4GroupDirectory &entry)
{
	if (entry.parent != nullptr)
	{
		stream << *dynamic_cast<const C4GroupEntry *>(&entry);
	}
	QByteArray buf;
	QDataStream header(&buf, QIODevice::WriteOnly);
	header.setVersion(QDataStream::Qt_1_0);

	header.writeRawData("RedWolf Design GrpFolder", 25);
	header << NullBytes(3) << (int32_t) 1 << (int32_t) 2 << (int32_t) entry.children.length();
	header.writeRawData(entry.author, 32);
	header << NullBytes(32) << (quint32) entry.creationDate << entry.original << NullBytes(92);
	entry.memScramble(buf);
	stream.writeRawData(buf.data(), buf.length());

	foreach(C4GroupEntry *e, entry.children)
	{
		if (auto *file = dynamic_cast<C4GroupFile *>(e))
		{
			stream << file;
		}
		else if (auto *dir = dynamic_cast<C4GroupDirectory *>(e))
		{
			stream << dir;
		}
	}
	return stream;
}

QDataStream &operator >>(QDataStream &stream, C4GroupFile &entry)
{
	entry.open(QIODevice::ReadWrite);
	QIODevice *device = stream.device();
	qint64 devicePos = device->pos();
	device->seek(entry.contentPosition());
	qint64 filePos = entry.pos();
	entry.seek(0);
	while (entry.pos() < entry.fileSize)
	{
		qint64 size = qMin<qint64>(BLOCKSIZE, entry.fileSize - entry.pos());
		entry.write(device->read(size));
	}
	device->seek(devicePos);
	entry.seek(filePos);
	entry.close();
	return stream;
}

QDataStream &operator >>(QDataStream &stream, C4GroupDirectory &entry)
{
	stream.device()->seek(entry.contentPosition());
	stream.setVersion(QDataStream::Qt_1_0);
	char buf[204];
	stream.readRawData(buf, 204);
	QByteArray ar(buf, 204);
	entry.memScramble(ar);
	assert(ar.startsWith("RedWolf Design GrpFolder"));
	QDataStream header(&ar, QIODevice::ReadOnly);
	header.setVersion(QDataStream::Qt_1_0);
	header.setByteOrder(stream.byteOrder());
	header >> NullBytes(36) >> entry.fileSize;
	header.readRawData(entry.author, 32);
	header >> NullBytes(32) >> entry.creationDate >> entry.original;
	header >> NullBytes(92);

	QIODevice *device = stream.device();
	qint64 pos = device->pos();
	for (int32_t i = 0; i < entry.fileSize; i++)
	{
		qint64 pos = entry.contentPosition() + 204 + 316 * i;
		int32_t isDir;
		device->seek(pos + 264);
		stream >> isDir;
		C4GroupEntry *e;
		if (isDir)
		{
			e = new C4GroupDirectory;
		}
		else
		{
			e = new C4GroupFile;
		}
		e->parent = &entry;
		e->group = entry.group;
		e->stream = entry.stream;
		device->seek(pos);
		QByteArray ar = device->read(316);
		device->seek(pos);
		stream.readRawData(e->filename, 256);
		stream >> NullBytes(12) >> e->fileSize >> NullBytes(4) >> e->relativeContentPosition;
		stream >> e->lastModification >> e->checksumType >> e->checksum >> e->executable;
		stream >> NullBytes(26);
		if (isDir)
		{
			if (entry.recursive)
			{
				stream >> *dynamic_cast<C4GroupDirectory *>(e);
			}
		}
		else
		{
			stream >> *dynamic_cast<C4GroupFile *>(e);
		}
		entry.children.append(e);
	}
	return stream;
}

int32_t C4GroupEntry::contentPosition()
{
	return parent ? parent->contentPosition() + 204 + (316 * dynamic_cast<C4GroupDirectory *>(parent)->fileSize) + relativeContentPosition : 0;
}

C4GroupFile::~C4GroupFile()
{
}

C4GroupDirectory::~C4GroupDirectory()
{
	qDeleteAll(children);
}

void C4GroupDirectory::memScramble(QByteArray &data) const
{
	for (int32_t i = 0; (i + 2) < data.length(); i += 3)
	{
		char tmp = data[i];
		data[i] = data[i + 2];
		data[i + 2] = tmp;
	}

	for (int32_t i = 0; i < data.length(); i++)
	{
		data[i] = data[i] ^ 0xED;
	}
}

C4Group::C4Group(const QString &grp) : path(grp)
{
}

C4Group::C4Group(QIODevice *device) : content(device)
{
}

C4Group::~C4Group()
{
	close();
}

void C4Group::close()
{
	if (root != nullptr)
	{
		delete root;
	}
}

void C4Group::extract(const QDir &target, C4GroupDirectory *dir)
{
	if (!isOpen())
		return;

	if (dir == nullptr)
	{
		dir = root;
	}
	target.mkpath(target.absolutePath());
	foreach(C4GroupEntry *e, dir->children)
	{
		if (auto *d = dynamic_cast<C4GroupDirectory *>(e))
		{
			extract(target.absoluteFilePath(e->filename), d);
		}
		else
		{
			auto *f = dynamic_cast<C4GroupFile *>(e);
			f->open(QIODevice::ReadOnly);
			QFile file(target.absoluteFilePath(f->filename));
			file.open(QIODevice::WriteOnly);
			qint64 pos = f->pos();
			f->seek(0);
			TRANSFER_CONTENTS(*f, file);
			f->seek(pos);
			file.close();
			f->close();
		}
	}
}

C4GroupEntry *C4Group::getChildByGroupPath(const QString &path)
{
	QStringList parts = path.split("/");
	C4GroupEntry *ret = nullptr;
	C4GroupDirectory *dir = root;
	for (int32_t i = 0; i < parts.length(); i++)
	{
		bool found = false;
		foreach(C4GroupEntry *e, dir->children)
		{
			if (e->filename == parts[i])
			{
				found = true;
				if (auto *d = dynamic_cast<C4GroupDirectory *>(e))
				{
					dir = d;
				}
				break;
			}
		}
		if (!found)
		{
			return nullptr;
		}
	}
	return ret;
}
