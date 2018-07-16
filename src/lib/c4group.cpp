#include <QDateTime>
#include <QDirIterator>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>
#include <cstring>
#include "./c4group.h"

QDataStream &operator <<(QDataStream &stream, const NullBytes &n)
{
	for (uint i = 0; i < n.count; i++)
	{
		stream << static_cast<qint8>('\0');
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
	if (entry.parentGroup == nullptr) // top level group folder
	{
		__builtin_trap();
		return stream;
	}

	stream.writeRawData(entry.fileName.leftJustified(255, '\0', true).constData(), 256);
	stream << NullBytes(4);
	stream << (int32_t) 1234567;
	if (dynamic_cast<const C4GroupDirectory *>(&entry))
	{
		stream << (int32_t) 1;
	}
	else
	{
		stream << (int32_t) 0;
	}
	stream << static_cast<int32_t>(entry.sizeInBytes()) << NullBytes(4) << entry.relativeContentPosition;
	stream << (qint32) entry.lastModification << entry.checksumType << entry.checksum;
	stream << entry.executable;
	stream << NullBytes(26);
	return stream;
}

QDataStream &operator <<(QDataStream &stream, C4GroupFile &entry)
{
	QIODevice *device = stream.device();
	qint64 devicePos = device->pos();
	qint64 contentPos = entry.contentPosition();
	Q_ASSERT(contentPos > ENTRYCORE_SIZE * entry.parentGroup->children.length());
	if (device->size() <= entry.contentPosition())
	{
		Q_ASSERT(device->seek(device->size()));
		stream << NullBytes(entry.contentPosition() - device->size());
	}
	device->seek(entry.contentPosition());
	entry->open(QIODevice::ReadOnly);
	TRANSFER_CONTENTS(*(entry.device), *device)
	Q_ASSERT(device->pos() - entry->size() == entry.contentPosition());
	device->seek(devicePos);
	if (entry.group->isPacked() && qobject_cast<QBuffer *>(entry.device) == nullptr)
	{
		entry->seek(0);
		auto *buffer = new QBuffer;
		buffer->open(QIODevice::WriteOnly);
		TRANSFER_CONTENTS(*(entry.device), *buffer)
		entry->close();
		delete entry.device;
		entry.device = buffer;
		return stream;
	}
	entry->close();
	return stream;
}

QDataStream &operator <<(QDataStream &stream, const C4GroupDirectory &entry)
{
	QByteArray buf;
	QDataStream header(&buf, QIODevice::WriteOnly);
	header.setVersion(QDataStream::Qt_1_0);
	header.setByteOrder(QDataStream::LittleEndian);

	header.writeRawData("RedWolf Design GrpFolder", 25);
	header << NullBytes(3) << (int32_t) 1 << (int32_t) 2 << (int32_t) entry.children.length();
	header.writeRawData(entry.author.leftJustified(31, '\0', true).constData(), 32);
	header << NullBytes(32) << (quint32) entry.creationDate << entry.original << NullBytes(92);
	entry.memScramble(buf);
	stream.writeRawData(buf.data(), buf.length());
	int32_t nextContentPosition = ENTRYCORE_SIZE * entry.children.length();
	foreach(C4GroupEntry *e, entry.children)
	{
		e->relativeContentPosition = nextContentPosition;
		stream << *e;
		QIODevice *device = stream.device();
		qint64 pos = device->pos();
		device->seek(e->contentPosition());
		if (auto *file = dynamic_cast<C4GroupFile *>(e))
		{
			(void) file;
			int32_t p = file->contentPosition();
			stream << *file;
		}
		else if (auto *dir = dynamic_cast<C4GroupDirectory *>(e))
		{
			int32_t p = dir->contentPosition();
			stream << *dir;
		}
		else
		{
			Q_ASSERT(false);
		}
		device->seek(pos);
		nextContentPosition += e->sizeInBytes();
	}
	return stream;
}

QDataStream &operator >>(QDataStream &stream, C4GroupFile &entry)
{
	entry->open(QIODevice::ReadWrite);
	QIODevice *device = stream.device();
	qint64 devicePos = device->pos();
	device->seek(entry.contentPosition());
	while (entry->pos() < entry.fileSize)
	{
		qint64 size = qMin<qint64>(BLOCKSIZE, entry.fileSize - entry->pos());
		if (device->atEnd() && size > 0)
		{
			throw C4GroupException(QStringLiteral("Corrupted group file"));
		}
		entry->write(device->read(size));
	}
	device->seek(devicePos);
	entry->close();
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

	QIODevice *device = stream.device();
	qint64 pos = device->pos();
	for (int32_t i = 0; i < entry.fileSize; i++)
	{
		qint64 pos = entry.contentPosition() + HEADER_SIZE + ENTRYCORE_SIZE * i;
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
		e->parentGroup = &entry;
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
	return parentGroup ? parentGroup->contentPosition() + HEADER_SIZE + (ENTRYCORE_SIZE * dynamic_cast<C4GroupDirectory *>(parentGroup)->fileSize) + relativeContentPosition : 0;
}

C4GroupFile::C4GroupFile() : device(new QBuffer)
{
}

C4GroupFile::~C4GroupFile()
{
}

void C4GroupFile::updateCRC32()
{
	device->open(QIODevice::ReadOnly);
	checksum = crc32(0L, Z_NULL, 0);
	do
	{
		char buf[BLOCKSIZE];
		qint64 len = device->read(buf, BLOCKSIZE);
		checksum = crc32(checksum, (Bytef *) buf, len);
	}
	while (!device->atEnd());
	checksumType = 0x01;
	device->close();
}

C4GroupDirectory::~C4GroupDirectory()
{
	qDeleteAll(children);
}

void C4GroupDirectory::memScramble(QByteArray &data) const
{
	Q_ASSERT(data.length() == HEADER_SIZE);
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

int32_t C4GroupDirectory::sizeInBytes() const
{
	int32_t size = HEADER_SIZE + ENTRYCORE_SIZE * children.length();
	foreach (C4GroupEntry *e, children)
	{
		size += e->sizeInBytes();
	}
	return size;
}

C4Group::C4Group(const QString &grp, QIODevice *device) : path(QDir(grp).absolutePath()), content(device)
{
}

C4Group::~C4Group()
{
	close();
}

void C4Group::open(bool recursive)
{
	recursive = recursive;
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
			auto *e = dynamic_cast<C4GroupDirectory *>(grp.getChildByGroupPath(p.join("/")));
			if (e == nullptr)
			{
				grp.close();
				throw C4GroupException("Couldn't find requested subgroup");
			}
			// if we don't do this, it and its children will get deleted upon group closing
			e->parentGroup->children.removeAt(e->parentGroup->children.indexOf(dynamic_cast<C4GroupEntry *>(e)));
			e->parentGroup = nullptr;
			root = e;
			grp.close();
			return;
		}
		QString tempPath = QDir::temp().absoluteFilePath(QStringLiteral("c4group-") + QFileInfo(path).fileName());
		QFile::remove(tempPath);
		Q_ASSERT(QFile::copy(path, tempPath));
		tmp.setFileName(tempPath);
		tmp.open(QIODevice::ReadWrite);
		QByteArray magic = tmp.read(2);
		if (
				(magic[0] != gz_magic_new[0] && magic[0] != gz_magic_old[0]) ||
				(magic[1] != gz_magic_new[1] && magic[1] != gz_magic_old[1]))
		{
			throw C4GroupException("File is not a c4group file");
		}
		tmp.seek(0);
		tmp.write(QByteArrayLiteral("\x1f\x8b"));
		tmp.close();
		content = new QBuffer;
		content->open(QIODevice::ReadWrite);
		{
			gzFile f = gzopen(QFile::encodeName(tempPath).data(), "r");
			if (f == Z_NULL)
			{
				throw new C4GroupException(QStringLiteral("Error at gzopen (%1)").arg(strerror(errno)));
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
	tmp.remove();
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
	(*count)++;
	target.mkpath(target.absolutePath());
	foreach(C4GroupEntry *e, dir->children)
	{
		if (auto *d = dynamic_cast<C4GroupDirectory *>(e))
		{
			explode(d, target.absoluteFilePath(e->fileName), count);
		}
		else
		{
			auto f = *dynamic_cast<C4GroupFile *>(e);
			f->open(QIODevice::ReadOnly);
			auto *file = new QFile(target.absoluteFilePath(f.fileName));
			file->open(QIODevice::WriteOnly | QIODevice::Truncate);
			TRANSFER_CONTENTS(*(f.device), *file);
			f->close();
			SAFE_DELETE(f.device);
			f.device = file;
			f->close();
		}
	}
	(*count)--;
	if (*count == 0)
	{
		packed = false;
		delete count;
		QFile::remove(newPath);
	}
}

void C4Group::pack(int compression)
{
	packed = true;
	Q_ASSERT(root != nullptr);
	SAFE_DELETE(content);
	QFileInfo info(path);
	QString tempPath = info.dir().absoluteFilePath(info.baseName() + ".000");
	content = new QFile(tempPath);
	content->open(QIODevice::ReadWrite | QIODevice::Truncate);
	stream.setDevice(content);
	stream.setVersion(QDataStream::Qt_1_0);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream << *root;
	char mode[3];
	sprintf(mode, "w%d", compression);
	gzFile f = gzopen(QFile::encodeName(path).constData(), mode);
	if (f == Z_NULL)
	{
		throw new C4GroupException(QStringLiteral("Error at gzopen (%1)").arg(strerror(errno)));
	}

	content->seek(0);
	char buf[BLOCKSIZE];
	int len;
	do
	{
		len = content->read(buf, BLOCKSIZE);
		gzwrite(f, buf, len);
	}
	while (!content->atEnd());
	gzclose(f);
	qobject_cast<QFile *>(content)->rename(path);
	QFile output(path);
	output.open(QIODevice::ReadWrite);
	output.write(QByteArrayLiteral("\x1e\x8c"));
	output.close();
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
			if (e->fileName == parts[i])
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

void C4Group::openFolder(QString dir, C4GroupDirectory *parentGroup)
{
	if (dir == "")
	{
		dir = path;
	}
	if (parentGroup == nullptr)
	{
		parentGroup = new C4GroupDirectory;
		parentGroup->fileName = QFile::encodeName(dir);
		parentGroup->relativeContentPosition = HEADER_SIZE;
	}

//	qt_ntfs_permission_lookup++;

	QDirIterator i(dir);
	while (i.hasNext())
	{
		i.next();
		if (QSet<QString>({".", "..", ""}).contains(i.fileName()))
		{
			continue;
		}
		QFileInfo info = i.fileInfo();
		uint count = info.dir().count();
		if (info.isDir())
		{
			auto *d = new C4GroupDirectory;
			d->parentGroup = parentGroup;
			d->stream = &stream;
			d->group = this;
			d->fileName = QFile::encodeName(info.fileName());
			d->author = QFile::encodeName(info.owner());
			d->executable = info.isExecutable();
			openFolder(i.filePath(), d);
			d->fileSize = d->children.length();
			d->creationDate = static_cast<uint32_t>(info.created().toSecsSinceEpoch());
			d->lastModification = static_cast<uint32_t>(info.lastModified().toSecsSinceEpoch());
			d->original = d->author == QByteArrayLiteral("RedWolf Design") ? 1234567 : 0;
			parentGroup->children.append(dynamic_cast<C4GroupEntry *>(d));
		}
		else
		{
			auto *f = new C4GroupFile;
			f->parentGroup = parentGroup;
			f->group = this;
			f->stream = &stream;
			f->fileName = QFile::encodeName(info.fileName());
			f->fileSize = info.size();
			f->executable = info.isExecutable();
			f->lastModification = static_cast<uint32_t>(info.lastModified().toSecsSinceEpoch());
			SAFE_DELETE(f->device)
			f->device = new QFile(info.absoluteFilePath());
			f->updateCRC32();
			parentGroup->children.append(dynamic_cast<C4GroupEntry *>(f));
		}
		count++;
	}
	root = parentGroup;
	packed = false;
//	qt_ntfs_permission_lookup--;
}
#undef SAFE_DELETE
