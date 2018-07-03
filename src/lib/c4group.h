#include <QByteArray>
#include <QBuffer>
#include <QException>
#include <QDataStream>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QList>
#include <QTemporaryFile>
#include <QtDebug>
#include <errno.h>
#include <zlib.h>

#define BLOCKSIZE 1024
#define TRANSFER_CONTENTS(x, y) while (!(x).atEnd()) { (y).write((x).read(BLOCKSIZE)); }

struct NullBytes
{
	uint count;
	NullBytes(uint count) : count(count) {}
};

class C4Group;

#define EXCEPTION_CLASS(x) class x : public QException \
{ \
public: \
	x(const QString &message) : message(message) {} \
	virtual ~x() {} \
	void raise() const { throw *this; } \
	x *clone() const { return new x(*this); } \
	QString getMessage() const { return message; } \
private: \
	QString message; \
};

EXCEPTION_CLASS(C4GroupException)

class C4GroupDirectory;

class C4GroupEntry
{
	Q_GADGET
public:
	virtual ~C4GroupEntry() = default;
	int32_t contentPosition();
	int32_t size() const { return fileSize; }

	void loadEntry(QDataStream &stream);

public: // yes, this is public on purpose
	C4Group *group;
	C4GroupDirectory *parent = nullptr;
public:
	char filename[256] = {'\0'}; // MAX_FNAME is 255 bytes on most filesystems.
	int32_t fileSize = 0;
	int32_t relativeContentPosition = 0;
	uint32_t lastModification = 0;
	char checksumType = 0x0;
	uint32_t checksum = 0;
	char executable = 0x0;
	QDataStream *stream = nullptr;
};

/* C4GroupFile is its own QIODevice, as writing directly to the group stream might damage the group file if
 * sensible data (e.g. the next file's contents) are overwritten. */

class C4GroupFile : public QBuffer, public C4GroupEntry
{
	Q_OBJECT
public:
	~C4GroupFile();
};


class C4GroupDirectory : public C4GroupEntry
{
public:
	~C4GroupDirectory();
public:
	char author[32] = {'\0'};
	uint32_t creationDate = 0;
	int32_t original = 1234567;
	QList<C4GroupEntry *>children;
private:
	void memScramble(QByteArray &data) const;
	friend QDataStream &operator <<(QDataStream &, const C4GroupDirectory &);
	friend QDataStream &operator >>(QDataStream &, C4GroupDirectory &);
	bool recursive = true;
	friend class C4Group;
};

// the first entry on CCAN (2000-06-23) already uses the new magic bytes, so this must be really old
static int gz_magic_old[2] = {0x1f, 0x8b};
static int gz_magic_new[2] = {0x1e, 0x8c};

class C4Group : public QObject
{
	Q_OBJECT
public:
	explicit C4Group(const QString &grp);
	C4Group(QIODevice *device = nullptr); // must be allocated on the heap, destructor takes care of deleting it
	~C4Group();
public:
	template<class T> void open(bool recursive = true)
	{
		if (content == nullptr)
		{
			QFileInfo info(path);
			if (!info.exists())
			{
				QStringList p;
				while (!info.exists())
				{
					p << info.fileName();
					info.setFile(info.path());
				}

				std::reverse(p.begin(), p.end());
				C4Group grp(info.filePath());
				grp.open<QBuffer>();
				auto *e = dynamic_cast<C4GroupDirectory *>(grp.getChildByGroupPath(p.join("/")));
				if (e == nullptr)
				{
					grp.close();
					throw new C4GroupException("Couldn't find requested subgroup");
				}
				// if we don't do this, it and its children will get deleted upon group closing
				e->parent->children.removeAt(e->parent->children.indexOf(dynamic_cast<C4GroupEntry *>(e)));
				e->parent = nullptr;
				root = e;
				grp.close();
				return;
			}
			QString tempPath = QDir::temp().absoluteFilePath(QStringLiteral("c4group-") + QFileInfo(path).fileName());
			QFile::remove(tempPath);
			assert(QFile::copy(path, tempPath));
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
			tmp.write(QByteArray("\x1f\x8b"));
			tmp.close();
			content = new T;
			content->open(QIODevice::ReadWrite);
			{
				gzFile f = gzopen(QFile::encodeName(tempPath).data(), "r");
				if (f == Z_NULL)
				{
					throw new C4GroupException(QString("Error at gzopen (%1)").arg(strerror(errno)));
				}

				char buf[BLOCKSIZE];
				int len;
				do
				{
					len = gzread(f, buf, BLOCKSIZE);
					content->write(buf, len);
				}
				while (!gzeof(f));
				int err;
				qDebug() << "gzerror" << gzerror(f, &err) << err;
				gzclose(f);
			}
		}
		content->seek(0);
		stream.setDevice(content);
		stream.setByteOrder(QDataStream::LittleEndian);
		root = new C4GroupDirectory;
		qstrcpy(root->filename, QFile::encodeName(QFileInfo(path).fileName()).left(255).constData());
		root->group = this;
		root->stream = &stream;
		root->recursive = recursive;
		stream >> *root;
		*root;
	}
	void close();
	void extract(const QDir &target, C4GroupDirectory *dir = nullptr);
	bool isOpen() const { return root != nullptr; }
	bool isPacked() const { return packed; }
	C4GroupEntry *getChildByGroupPath(const QString &path);
	C4GroupDirectory *root = nullptr;
public:
	QIODevice *content = nullptr;
private:
	void extractGroup(C4GroupDirectory *dir);
	QDataStream stream;
	QString path;
	QFile tmp;
	bool packed = true;
};
