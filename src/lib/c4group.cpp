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

#include <QDateTime>
#include <QDirIterator>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>
#include <cstring>
#include "./c4group.h"

QDataStream &operator <<(QDataStream &stream, const NullBytes &n)
{
	for (uint i = 0; i < n.count; ++i)
	{
		stream << static_cast<qint8>('\0');
	}
	return stream;
}

QDataStream &operator >>(QDataStream &stream, const NullBytes &n)
{
	qint8 tmp;
	for (uint i = 0; i < n.count; ++i)
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
	if (entry.parent() == nullptr) // top level group folder
	{
		return stream;
	}

	stream.writeRawData(entry.fileName.leftJustified(255, '\0', true).constData(), 256);
	stream << NullBytes(4);
	stream << static_cast<qint32>(1234567);
	if (qobject_cast<const C4GroupDirectory *>(&entry))
	{
		stream << static_cast<qint32>(1);
	}
	else
	{
		stream << static_cast<qint32>(0);
	}
	stream << static_cast<qint32>(entry.sizeInBytes()) << NullBytes(4) << entry.relativeContentPosition;
	stream << static_cast<qint32>(entry.lastModification) << entry.checksumType << entry.checksum;
	stream << entry.executable;
	stream << NullBytes(26);
	return stream;
}

QDataStream &operator <<(QDataStream &stream, C4GroupFile &entry)
{
	QIODevice * const device = stream.device();
	const qint64 devicePos = device->pos();
	const qint64 contentPos = entry.contentPosition();

	Q_ASSERT(contentPos > ENTRYCORE_SIZE * entry.parent()->children().length());
	if (device->size() <= entry.contentPosition())
	{
		Q_ASSERT(device->seek(device->size()));
		stream << NullBytes(static_cast<uint>(entry.contentPosition() - device->size()));
	}

	device->seek(entry.contentPosition());
	entry.device->open(QIODevice::ReadOnly);
	TRANSFER_CONTENTS(*(entry.device), *device)
	Q_ASSERT(device->pos() - entry.device->size() == entry.contentPosition());
	device->seek(devicePos);

	if (entry.group->isPacked() && qobject_cast<QBuffer *>(entry.device) == nullptr)
	{
		entry.device->seek(0);
		auto * const buffer = new QBuffer;
		buffer->open(QIODevice::WriteOnly);
		TRANSFER_CONTENTS(*(entry.device), *buffer)
		entry.device->close();
		delete entry.device;
		entry.device = buffer;
		return stream;
	}
	entry.device->close();
	return stream;
}

QDataStream &operator <<(QDataStream &stream, const C4GroupDirectory &entry)
{
	QByteArray buf;
	QDataStream header(&buf, QIODevice::WriteOnly);
	header.setVersion(QDataStream::Qt_1_0);
	header.setByteOrder(QDataStream::LittleEndian);

	header.writeRawData("RedWolf Design GrpFolder", 25);
	header << NullBytes(3) << static_cast<qint32>(1) << static_cast<qint32>(2) << static_cast<qint32>(entry.children().length());
	header.writeRawData(entry.author.leftJustified(31, '\0', true).constData(), 32);
	header << NullBytes(32) << entry.creationDate << entry.original << NullBytes(92);
	entry.memScramble(buf);
	stream.writeRawData(buf.data(), buf.length());
	qint32 nextContentPosition = ENTRYCORE_SIZE * entry.children().length();
	foreach(QObject *e, entry.children())
	{
		auto *item = qobject_cast<C4GroupEntry *>(e);
		Q_ASSERT(item);
		item->relativeContentPosition = nextContentPosition;
		Q_ASSERT(item->contentPosition() != 0);
		stream << *item;
		QIODevice * const device = stream.device();
		const qint64 pos = device->pos();
		device->seek(item->contentPosition());
		if (auto * const file = qobject_cast<C4GroupFile *>(e))
		{
			stream << *file;
		}
		else if (auto * const dir = qobject_cast<C4GroupDirectory *>(e))
		{
			stream << *dir;
		}
		else
		{
			Q_ASSERT(false);
		}
		device->seek(pos);
		nextContentPosition += item->sizeInBytes();
	}
	return stream;
}

QDataStream &operator >>(QDataStream &stream, C4GroupFile &entry)
{
	entry.device->open(QIODevice::ReadWrite);
	QIODevice * const device = stream.device();
	const qint64 devicePos = device->pos();
	device->seek(entry.contentPosition());
	while (entry.device->pos() < entry.fileSize)
	{
		const qint64 size = qMin<qint64>(BLOCKSIZE, entry.fileSize - entry.device->pos());
		if (device->atEnd() && size > 0)
		{
			throw C4GroupException(QStringLiteral("Corrupted group file"));
		}
		entry.device->write(device->read(size));
	}
	device->seek(devicePos);
	entry.device->close();
	return stream;
}

QDataStream &operator >>(QDataStream &stream, C4GroupDirectory &entry)
{
	stream.device()->seek(entry.contentPosition());
	stream.setVersion(QDataStream::Qt_1_0);
	char buf[HEADER_SIZE];
	stream.readRawData(buf, HEADER_SIZE);
	QByteArray ar(buf, HEADER_SIZE);
	entry.memScramble(ar);
	if (!ar.startsWith("RedWolf Design GrpFolder"))
	{
		throw C4GroupException(QStringLiteral("Corrupted group folder"));
	}

	QDataStream header(&ar, QIODevice::ReadOnly);
	header.setVersion(QDataStream::Qt_1_0);
	header.setByteOrder(stream.byteOrder());
	header >> NullBytes(36) >> entry.fileSize;
	char author[32];
	header.readRawData(author, 32);
	entry.author = QByteArray(author); // field ends with a NUL byte
	header >> NullBytes(32) >> entry.creationDate >> entry.original;
	header >> NullBytes(92);

	QIODevice * const device = stream.device();
	for (qint32 i = 0; i < entry.fileSize; ++i)
	{
		const qint64 pos = entry.contentPosition() + HEADER_SIZE + ENTRYCORE_SIZE * i;
		qint32 isDir;
		device->seek(pos + 264);
		stream >> isDir;
		C4GroupEntry *e;
		if (isDir)
		{
			e = new C4GroupDirectory(&entry);
		}
		else
		{
			e = new C4GroupFile(&entry);
		}
		e->group = entry.group;
		e->stream = entry.stream;
		device->seek(pos);
		char fileName[256];
		stream.readRawData(fileName, 256);
		e->fileName = QByteArray(fileName);
		stream >> NullBytes(12) >> e->fileSize >> NullBytes(4) >> e->relativeContentPosition;
		stream >> e->lastModification >> e->checksumType >> e->checksum >> e->executable;
		stream >> NullBytes(26);
		if (isDir)
		{
			if (entry.group->recursive)
			{
				stream >> *qobject_cast<C4GroupDirectory *>(e);
			}
		}
		else
		{
			stream >> *qobject_cast<C4GroupFile *>(e);
		}
	}
	return stream;
}

qint32 C4GroupEntry::contentPosition()
{
	auto *par = qobject_cast<C4GroupDirectory *>(parent());
	return par ? par->contentPosition() + HEADER_SIZE + (ENTRYCORE_SIZE * qobject_cast<C4GroupDirectory *>(par)->fileSize) + relativeContentPosition : 0;
}

C4GroupFile::C4GroupFile(C4GroupDirectory *parent) : C4GroupEntry(parent), device(new QBuffer)
{
}

C4GroupFile::~C4GroupFile()
{
	delete device;
}

void C4GroupFile::updateCRC32()
{
	device->open(QIODevice::ReadOnly);
	checksum = static_cast<quint32>(crc32(0L, nullptr, 0));
	qint64 len;
	do
	{
		char buf[BLOCKSIZE];
		len = device->read(buf, BLOCKSIZE);
		checksum = static_cast<quint32>(crc32(checksum, reinterpret_cast<Bytef *>(buf), static_cast<quint32>(len)));
	}
	while (!device->atEnd());
	checksumType = 0x01;
	device->close();
}

void C4GroupDirectory::memScramble(QByteArray &data) const
{
	Q_ASSERT(data.length() == HEADER_SIZE);
	for (qint32 i = 0; (i + 2) < data.length(); i += 3)
	{
		const char tmp = data[i];
		data[i] = data[i + 2];
		data[i + 2] = tmp;
	}

	for (qint32 i = 0; i < data.length(); ++i)
	{
		data[i] = data[i] ^ 0xED;
	}
}

qint32 C4GroupDirectory::sizeInBytes() const
{
	qint32 size = HEADER_SIZE + ENTRYCORE_SIZE * children().length();
	foreach (QObject *e, children())
	{
		size += qobject_cast<C4GroupEntry *>(e)->sizeInBytes();
	}
	return size;
}

C4Group::C4Group(const QString &grp, QIODevice *device) : content(device), path(QDir(grp).absolutePath())
{
}

C4Group::~C4Group()
{
	close();
}

void C4Group::open(bool recursive)
{
	this->recursive = recursive;
	if (content == nullptr)
	{
		QFileInfo info(path);
		if (info.isDir())
		{
			openFolder();
			return;
		}
		else if (!info.exists())
		{
			QStringList p;
			while (!info.exists())
			{
				p << info.fileName();
				info.setFile(info.path());
			}

			std::reverse(p.begin(), p.end());
			C4Group grp(info.filePath());
			grp.open();
			auto * const e = qobject_cast<C4GroupDirectory *>(grp.getChildByGroupPath(p.join("/")));
			if (e == nullptr)
			{
				grp.close();
				throw C4GroupException("Couldn't find requested subgroup");
			}
			// if we don't do this, it and its children will get deleted upon group closing
			e->setParent(nullptr);
			root = e;
			grp.close();
			return;
		}
		QTemporaryFile temp;
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly))
		{
			throw C4GroupException(QStringLiteral("Error at opening file path: %1").arg(file.errorString()));
		}
		temp.open();
		TRANSFER_CONTENTS(file, temp)
		file.close();
		temp.seek(0);
		const QByteArray magic = temp.peek(2);
		if (
				(static_cast<uchar>(magic[0]) != gz_magic_new[0] && static_cast<uchar>(magic[0]) != gz_magic_old[0]) ||
				(static_cast<uchar>(magic[1]) != gz_magic_new[1] && static_cast<uchar>(magic[1]) != gz_magic_old[1]))
		{
			throw C4GroupException("File is not a c4group file");
		}

		temp.write(QByteArrayLiteral("\x1f\x8b"));
		temp.close();
		content = new QBuffer;
		content->open(QIODevice::ReadWrite);
		{
			gzFile f = gzopen(QFile::encodeName(temp.fileName()).data(), "r");
			if (f == nullptr)
			{
				throw C4GroupException(QStringLiteral("Error at gzopen (%1)").arg(strerror(errno)));
			}

			char buf[BLOCKSIZE];
			int len;
			do
			{
				len = gzread(f, buf, BLOCKSIZE);
				content->write(buf, len);
			}
			while (!gzeof(f));
			gzclose(f);
		}
	}
	content->seek(0);
	stream.setDevice(content);
	stream.setByteOrder(QDataStream::LittleEndian);
	root = new C4GroupDirectory;
	root->fileName = QFile::encodeName(QFileInfo(path).fileName());
	root->group = this;
	root->stream = &stream;
	stream >> *root;
}

#define SAFE_DELETE(x) if (x != nullptr) { delete x; x = nullptr; }
void C4Group::close()
{
	SAFE_DELETE(root)
	SAFE_DELETE(content)
}

void C4Group::explode(C4GroupDirectory *dir, QDir target, int *count)
{
	if (!(isOpen() && isPacked()))
	{
		return;
	}

	if (dir == nullptr)
	{
		dir = root;
	}

	if (count == nullptr)
	{
		count = new int(0);
	}

	QString newPath = path;

	if (target == QDir())
	{
		QFileInfo info(path);
		newPath = info.dir().absoluteFilePath(info.baseName() + "000");
		QDir(newPath).removeRecursively();
		target = path;
		QFile::rename(path, newPath);
	}
	++(*count);
	target.mkpath(target.absolutePath());
	foreach(QObject *e, dir->children())
	{
		if (auto *d = qobject_cast<C4GroupDirectory *>(e))
		{
			explode(d, target.absoluteFilePath(d->fileName), count);
		}
		else
		{
			auto f = qobject_cast<C4GroupFile *>(e);
			f->device->open(QIODevice::ReadOnly);
			auto *file = new QFile(target.absoluteFilePath(f->fileName));
			file->open(QIODevice::WriteOnly | QIODevice::Truncate);
			TRANSFER_CONTENTS(*(f->device), *file);
			f->device->close();
			SAFE_DELETE(f->device);
			f->device = file;
			f->device->close();
		}
	}
	--(*count);
	if (*count == 0)
	{
		packed = false;
		delete count;
		QFile::remove(newPath);
	}
}

void C4Group::pack(int compression)
{
	Q_ASSERT(root != nullptr);
	content->close();
	SAFE_DELETE(content);
	const QFileInfo info(path);
	const QString tempPath = info.dir().absoluteFilePath(info.baseName() + ".000");
	content = new QFile(tempPath);
	if (Q_UNLIKELY(!content->open(QIODevice::ReadWrite | QIODevice::Truncate)))
	{
		throw C4GroupException(QStringLiteral("Error at opening file %1: %2").arg(tempPath).arg(content->errorString()));
	}
	stream.setDevice(content);
	stream.setVersion(QDataStream::Qt_1_0);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream << *root;
	content->seek(0);

	QDir(path).removeRecursively();

	char mode[3];
	sprintf(mode, "w%d", compression);
	gzFile f = gzopen(QFile::encodeName(path).constData(), mode);
	if (f == nullptr)
	{
		throw C4GroupException(QStringLiteral("Error at gzopen (%1)").arg(strerror(errno)));
	}

	char buf[BLOCKSIZE];
	qint64 len;
	do
	{
		len = content->read(buf, BLOCKSIZE);
		if (Q_UNLIKELY(gzwrite(f, buf, static_cast<quint32>(qMax<qint64>(len, 0))) < 0))
		{
			throw C4GroupException(QStringLiteral("Error at gzwrite (%1)").arg(gzerror(f, nullptr)));
		}
	}
	while (!content->atEnd());
	gzclose(f);
	QFile::remove(tempPath);
	packed = true;
}

C4GroupEntry *C4Group::getChildByGroupPath(const QString &path)
{
	const QStringList parts = path.split("/");
	C4GroupEntry *ret = nullptr;
	C4GroupDirectory *dir = root;
	for (qint32 i = 0; i < parts.length(); ++i)
	{
		bool found = false;
		foreach(QObject *e, dir->children())
		{
			auto *item = qobject_cast<C4GroupEntry *>(e);
			if (item->fileName == parts[i])
			{
				found = true;
				if (auto *d = qobject_cast<C4GroupDirectory *>(e))
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
#define TIMESTAMP(x) static_cast<quint32>((x).toSecsSinceEpoch())
#else
#define TIMESTAMP(x) static_cast<quint32>((x).toMSecsSinceEpoch() / 1000)
#endif

void C4Group::openFolder(QString dir, C4GroupDirectory *parentGroup)
{
	if (dir == "")
	{
		dir = path;
	}
	if (parentGroup == nullptr)
	{
		parentGroup = new C4GroupDirectory(nullptr);
		parentGroup->fileName = QFile::encodeName(dir);
		parentGroup->relativeContentPosition = HEADER_SIZE;
	}

//	++qt_ntfs_permission_lookup;

	QDirIterator i(dir);
	while (i.hasNext())
	{
		i.next();
		if (QSet<QString>({".", "..", ""}).contains(i.fileName()))
		{
			continue;
		}
		QFileInfo info = i.fileInfo();
		if (info.isDir())
		{
			auto *d = new C4GroupDirectory(parentGroup);
			d->stream = &stream;
			d->group = this;
			d->fileName = QFile::encodeName(info.fileName());
			d->author = QFile::encodeName(info.owner());
			d->executable = info.isExecutable();
			openFolder(i.filePath(), d);
			d->fileSize = d->children().length();
			d->creationDate = TIMESTAMP(info.created());
			d->lastModification = TIMESTAMP(info.lastModified());
			d->original = d->author == QByteArrayLiteral("RedWolf Design") ? 1234567 : 0;
		}
		else
		{
			auto *f = new C4GroupFile(parentGroup);
			f->group = this;
			f->stream = &stream;
			f->fileName = QFile::encodeName(info.fileName());
			f->fileSize = static_cast<qint32>(info.size());
			f->executable = info.isExecutable();
			f->lastModification = TIMESTAMP(info.lastModified());
			SAFE_DELETE(f->device)
			f->device = new QFile(info.absoluteFilePath());
			f->updateCRC32();
		}
	}
	root = parentGroup;
	packed = false;
//	--qt_ntfs_permission_lookup;
}

#undef SAFE_DELETE
