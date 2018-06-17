#include <QDebug>
#include <QDirIterator>
#include <QSet>
#include <QMutex>
#include <QRegularExpression>
#include <QPluginLoader>
#include <algorithm>
#include "lcedit.h"
// #include "ui_lcedit.h"

#define LOAD(x) auto *plugin = qobject_cast<LCPluginInterface *>((x)); \
		if (plugin != nullptr) \
		{ \
			plugin->init(this); \
			plugins.append(plugin); \
		}

QString LCTreeWidgetItem::filePath()
{
	return text(1);
}


LCEdit::LCEdit(const QString &path, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::LCEdit),
	m_path(path)
{
	ui->setupUi(this);
	QTreeWidgetItem *header = new QTreeWidgetItem();
	header->setText(0, "LCEdit");
	ui->treeWidget->setHeaderItem(header);
	ui->treeWidget->setColumnCount(1);
	QObject::connect(ui->treeWidget, &QTreeWidget::currentItemChanged, this, &LCEdit::treeItemChanged);
	loadPlugins();
	createTree(m_path);
}

LCEdit::~LCEdit()
{
	delete ui;
}

void LCEdit::loadPlugins()
{
	plugins.clear();
	foreach (QObject *obj, QPluginLoader::staticInstances())
	{
		LOAD(obj);
	}
	
	// from http://doc.qt.io/qt-5/qtwidgets-tools-plugandpaint-app-example.html
	QDir pluginsDir = QDir(qApp->applicationDirPath());
	
#if defined(Q_OS_WIN)
	if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
		pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
	if (pluginsDir.dirName() == "MacOS") {
		pluginsDir.cdUp();
		pluginsDir.cdUp();
		pluginsDir.cdUp();
	}
#endif
	//pluginsDir.cd("plugins");
	
	foreach (QString filename, pluginsDir.entryList(QDir::Files))
	{
		QPluginLoader loader(pluginsDir.absoluteFilePath(filename));
		LOAD(loader.instance()); 
	}
	
	std::sort(plugins.begin(), plugins.end(), [](LCPluginInterface *a, LCPluginInterface *b) {return a->priority() > b->priority(); });
}

void LCEdit::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	CALL_PLUGINS(createTree(base, parent))
	QDirIterator i(base);
	while (i.hasNext())
	{
		if (QSet<QString>({".", "..", ""}).contains(i.fileName()))
		{
			i.next();
			continue;
		}
		
		if (parent != nullptr)
		{
			auto *item = createEntry<LCTreeWidgetItem>(parent, i.fileName(), i.filePath());
		}
		else
		{
			createEntry<QTreeWidget>(ui->treeWidget, i.fileName(), i.filePath());
		}
		i.next();
	}
	ui->treeWidget->sortItems(0, Qt::AscendingOrder);
}

void LCEdit::treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	CALL_PLUGINS(treeItemChanged(dynamic_cast<LCTreeWidgetItem *>(current), dynamic_cast<LCTreeWidgetItem *>(previous)))
	ui->lblName->setText("");
	ui->txtDescription->setText("");
	
	auto *root = dynamic_cast<LCTreeWidgetItem *>(current); // no qobject_cast, as QTreeWidgetItem doesn't inherit from QObject
	if (root == nullptr)
		return;
	
	QFileInfo info(root->filePath());
	if (root->childCount() == 0 && info.isDir())
	{
		createTree(root->filePath(), root);
	}
	
	ui->lblName->setText(current->text(0));
	if (QSet<QString>({"txt", "c", "log"}).contains(info.completeSuffix()) /*&& info.size() < 8092*/)
	{
		QFile file(info.filePath());
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;
		
		qDebug() << "Reading file";
		ui->txtDescription->setPlainText(file.readAll());
		file.close();
	}
}

QString GetFirstExistingPath(QFileInfo path)
{
	while (!path.exists()) path.setFile(path.path());
	return path.filePath();
}
