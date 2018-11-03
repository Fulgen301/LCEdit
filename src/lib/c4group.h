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

extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;

#define BLOCKSIZE 1024
#define TRANSFER_CONTENTS(x, y) while (!(x).atEnd()) { (y).write((x).read(BLOCKSIZE)); }

const qint32 HEADER_SIZE = 204;
const qint32 ENTRYCORE_SIZE = 316;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#warning C4Group uses little endian as byte order in order to maintain compatibility with \
	groups created on x86 processors.
#endif
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

class C4GroupEntry : public QObject
{
	Q_OBJECT
public:
	C4GroupEntry(C4GroupEntry *parent) : QObject(qobject_cast<QObject *>(parent)) {}
	virtual ~C4GroupEntry() = default;

public: // yes, this is public on purpose
	C4Group *group;
	virtual qint64 size() const { return fileSize; } // yes, we provide both, as the size specified in a directory's TOC'
	virtual qint32 sizeInBytes() const { return size(); } // entry is the count of children, not the size in bytes
	qint32 contentPosition();
public:
	QByteArray fileName = ""; // MAX_FNAME is 255 bytes on most filesystems.
	qint32 fileSize = 0;
	qint32 relativeContentPosition = 0;
	quint32 lastModification = 0;
	char checksumType = 0x0;
	quint32 checksum = 0;
	char executable = 0x0;
	QDataStream *stream = nullptr;
private:
	friend QDataStream &operator <<(QDataStream &, const C4GroupEntry &);
};

/* C4GroupFile has its own QIODevice, as writing directly to the group stream might damage the group file if
 * sensible data (e.g. the next file's contents) are overwritten. */

class C4GroupFile : public C4GroupEntry
{
	Q_OBJECT
public:
	C4GroupFile(C4GroupDirectory *parent);
	~C4GroupFile();
	void updateCRC32();
	QIODevice *device = nullptr;
	qint64 size() const override { return device->size(); }

	friend QDataStream &operator <<(QDataStream &, C4GroupFile &);
	friend QDataStream &operator >>(QDataStream &, C4GroupFile &);
};

class C4GroupDirectory : public C4GroupEntry
{
	Q_OBJECT
public:
	C4GroupDirectory(C4GroupDirectory *parent = nullptr) : C4GroupEntry(parent) {}
	~C4GroupDirectory() = default;
	qint32 sizeInBytes() const;
public:
	QByteArray author = "";
	quint32 creationDate = 0;
	qint32 original = 1234567;
private:
	void memScramble(QByteArray &data) const;
	friend class C4Group;
	friend QDataStream &operator <<(QDataStream &, const C4GroupDirectory &);
	friend QDataStream &operator >>(QDataStream &, C4GroupDirectory &);
};

// the first entry on CCAN (2000-06-23) already uses the new magic bytes, so this must be really old
static int gz_magic_old[2] = {0x1f, 0x8b};
static int gz_magic_new[2] = {0x1e, 0x8c};

class C4Group : public QObject
{
	Q_OBJECT
public:
	explicit C4Group(const QString &grp, QIODevice *device = nullptr); // device must be allocated on the heap, destructor takes care of deleting it
	~C4Group();
public:
	void open(bool recursive = true);
	void close();
	void explode(C4GroupDirectory *dir = nullptr, QDir target = QDir(), int *count = nullptr);
	void pack(int compression = 0);
	bool isOpen() const { return root != nullptr; }
	bool isPacked() const { return packed; }
	C4GroupEntry *getChildByGroupPath(const QString &path);
	C4GroupDirectory *root = nullptr;
public:
	QIODevice *content = nullptr;
private:
	void openFolder(QString dir = "", C4GroupDirectory *parentGroup = nullptr);
	QDataStream stream;
	QString path;
	bool packed = true;
	bool recursive = true;
	friend class C4GroupDirectory;
	friend class C4GroupFile;
	friend class C4GroupEntry;
	friend QDataStream &operator >>(QDataStream &, C4GroupDirectory &);
	Q_DISABLE_COPY(C4Group)
};
