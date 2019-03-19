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

#include <QDebug>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileDialog>
#include <QSet>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QMutex>
#include <QRegularExpression>
#include <QPluginLoader>
#include <QStaticPlugin>
#include <QTimer>
#include <algorithm>
#include "lcedit.h"
// #include "ui_lcedit.h"

#define LOAD(obj, meta) QJsonObject metaData = (meta).value("MetaData").toObject(); \
	if ((obj) != nullptr && !pluginNames.contains(metaData.value("name").toString())) \
{ \
	auto *plugin = qobject_cast<LCPluginInterface *>(obj); \
	if (plugin != nullptr) \
	{ \
		plugin->init(this); \
		plugins.append(LCPlugin { \
			.plugin = plugin, \
			.metaData = metaData \
		}); \
		pluginNames.append(metaData.value("name").toString()); \
		qDebug() << "Loaded plugin " << metaData.value("name").toString(); \
	} \
}

QString LCTreeWidgetItem::filePath()
{
	return text(1);
}

LCTreeWidgetItem *LCTreeWidgetItem::getChildByName(const QString &name, Qt::CaseSensitivity sensitivity)
{
	for (auto i = 0; i < childCount(); i++)
	{
		if (child(i)->text(0).compare(name, sensitivity) == 0)
		{
			return dynamic_cast<LCTreeWidgetItem *>(child(i));
		}
	}
	return nullptr;
}

LCEdit::LCEdit(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::LCEdit)
{
	ui->setupUi(this);
	ui->treeWidget->setColumnCount(1);
	connect(ui->treeWidget, &QTreeWidget::currentItemChanged, this, &LCEdit::setCommandLine);
	connect(ui->treeWidget, &QTreeWidget::currentItemChanged, this, &LCEdit::treeItemChanged);
	connect(ui->treeWidget, &QTreeWidget::itemExpanded, this, &LCEdit::treeItemExpanded);
	connect(ui->txtCmdLine, &QLineEdit::returnPressed, this, &LCEdit::startProcess);
	connect(ui->btnStart, &QPushButton::clicked, this, &LCEdit::startProcess);
	connect(ui->actLinks, &QAction::triggered, [this]()
	{
		const QString url = QStringLiteral("https://clonkspot.org/development");
		if (!QDesktopServices::openUrl(QUrl(url)))
		{
			QMessageBox::information(this, tr("Weiterführende Links"), url);
		}
	});
	connect(ui->actPath, &QAction::triggered, this, &LCEdit::showPathDialog);

	loadPlugins(); // Plugins might want to set the path.

	if (!QFileInfo(settings.value("Path").toString()).isDir())
	{
		showPathDialog();
	}
	else
	{
		createTree(settings.value("Path").toString());
	}
}

LCEdit::~LCEdit()
{
	delete ui;
}

void LCEdit::loadPlugins()
{
	plugins.clear();
	QStringList pluginNames;

	foreach (const QStaticPlugin obj, QPluginLoader::staticPlugins())
	{
		LOAD(obj.instance(), obj.metaData())
	}

	QStringList paths = qApp->libraryPaths();

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
	if (pluginsDir.cd("plugins"))
	{
		paths << pluginsDir.absolutePath();
	}

	QStringList errors;

	for (const QString &path : qAsConst(paths))
	{
		QDir dir(path);
		for (const QString &entry : dir.entryList({"lcedit_plugin_*"}))
		{
			QPluginLoader loader(dir.absoluteFilePath(entry));
#ifndef QT_NO_DEBUG
			qDebug() << "Loading plugin " << loader.fileName();
#endif
			LOAD(loader.instance(), loader.metaData())
			if (!loader.isLoaded())
			{
				errors << loader.errorString();
			}
		}
	}

	std::sort(plugins.begin(), plugins.end(), [](LCPlugin a, LCPlugin b) {return a.metaData.value("priority").toInt() < b.metaData.value("priority").toInt(); });

	if (errors.length() > 0)
	{
		QMessageBox messageBox;
		messageBox.setIcon(QMessageBox::Critical);
		messageBox.setText(tr("Einige Plugins konnten nicht geladen werden."));
		messageBox.setDetailedText(errors.join('\n'));
		messageBox.setStandardButtons(QMessageBox::Ok);
		messageBox.setDefaultButton(QMessageBox::Ok);
		messageBox.show();
		messageBox.exec();
	}
}

void LCEdit::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	QTimer::singleShot(0, [this]() { ui->treeWidget->sortItems(0, Qt::AscendingOrder); });
	CALL_PLUGINS(createTree(base, parent))
	if (!QFileInfo(base.path()).isDir())
		return;
	for (const QString &entry : base.entryList(QDir::AllEntries | QDir::NoDotAndDotDot))
	{
		if (parent != nullptr)
		{
			createEntry<LCTreeWidgetItem>(parent, entry, base.absoluteFilePath(entry));
		}
		else
		{
			createEntry<QTreeWidget>(ui->treeWidget, entry, base.absoluteFilePath(entry));
		}
	}
}

void LCEdit::setCommandLine(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	Q_UNUSED(previous);
	QStringList cmdLine = ui->txtCmdLine->text().split(" ", QString::SkipEmptyParts);
	if (cmdLine.length() < 1)
		cmdLine << "legacyclonk";

	for (auto i = 1; i < cmdLine.length(); i++)
	{
		if (!(cmdLine[i].startsWith("/") || cmdLine[i].startsWith("--")))
		{
			cmdLine.removeAt(i);
		}
	}
	if (current)
	{
		cmdLine << QDir(settings.value("Path").toString()).relativeFilePath(current->text(1)).replace('/', '\\');
	}
	ui->txtCmdLine->setText(cmdLine.join(" "));
}

void LCEdit::treeItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	auto *root = dynamic_cast<LCTreeWidgetItem *>(current);
	if (root != nullptr)
	{
		bool state = ui->lblName->isVisible();
		ui->lblName->setText(root->text(0));
		state ? ui->lblName->show() : ui->lblName->hide();

		if (root->childCount() == 0)
		{
			createTree(root->filePath(), root);
		}
	}

	CALL_PLUGINS(treeItemChanged(root, dynamic_cast<LCTreeWidgetItem *>(previous)))

	if (root == nullptr) // we only return here, because some plugin might want to do something nevertheless
	{
		ui->lblName->setText("");
		ui->txtDescription->setPlainText("");
		return;
	}

	QIODevicePtr device = getDevice(root);
	if (!device.isNull()
			&& device->open(QIODevice::ReadOnly)
			&& QMimeDatabase().mimeTypeForFileNameAndData(root->text(0), device->peek(20)).inherits("text/plain"))
	{
		ui->txtDescription->setPlainText(device->readAll());
		device->close();
	}
	else
	{
		ui->txtDescription->setPlainText("");
	}
}

void LCEdit::treeItemExpanded(QTreeWidgetItem *item)
{
	auto *root = dynamic_cast<LCTreeWidgetItem *>(item);
	if (root != nullptr && root->childCount() == 0)
	{
		createTree(root->filePath(), root);
	}
}

void LCEdit::startProcess()
{
	QStringList cmdLine = ui->txtCmdLine->text().split(" ", QString::SkipEmptyParts);
	if (cmdLine.length() < 2)
		return;

	if (proc != nullptr)
	{
		proc->terminate();
	}
	delete proc;
	proc = new QProcess;
	proc->start(QDir(settings.value("Path").toString()).absoluteFilePath(cmdLine.takeFirst()), cmdLine);
	proc->waitForStarted();
}

void LCEdit::showPathDialog()
{
	QString oldPath = settings.value("Path").toString();
	QString newPath = QFileDialog::getExistingDirectory(
						  this,
						  tr("Wählen Sie den Pfad Ihrer Clonk-Installation aus"),
						  oldPath
						  );
	if (!newPath.isEmpty() && oldPath != newPath)
	{
		settings.setValue("Path", newPath);
		reload();
	}
}

QIODevicePtr LCEdit::getDevice(LCTreeWidgetItem *item)
{
#ifndef QT_NO_DEBUG
	qDebug() << "getDevice called for" << item << "(" << item->text(0) << ")";
#endif
	std::optional<LCDeviceInformation> ret;
	for(const LCPlugin &obj : plugins)
	{
		ret = obj.plugin->getDevice(item);
		if (ret)
		{
			item->mutex.lock();
			return QIODevicePtr(ret->device, [deleter = ret->deleter, item](QIODevice *device)
			{
				if (!deleter())
				{
#ifdef QT_NO_DEBUG
					qWarning()
#else
					qCritical()
#endif
					<< "Destructor for" << device << "failed!";
				}
				item->mutex.unlock();
			});

		}
	}
	if (QFileInfo(item->filePath()).isFile())
	{
		item->mutex.lock();
		return QIODevicePtr(new QFile(item->filePath()), [item](QFile *file)
		{
			item->mutex.unlock();
			delete file;
		});
	}
	else
	{
		return QIODevicePtr(nullptr);
	}
}

void LCEdit::reload()
{
	ui->treeWidget->clear();
	treeItemChanged(nullptr, ui->treeWidget->currentItem());
	createTree(settings.value("Path").toString());
}

LCTreeWidgetItem *LCEdit::getItemByPath(const QString &path, LCTreeWidgetItem *parent)
{
	QStringList parts = QDir::fromNativeSeparators(path).split('/', QString::SkipEmptyParts);
	if (parts.length() == 0)
	{
		return nullptr;
	}
	for (const QString &part : qAsConst(parts))
	{
		if (parent == nullptr)
		{
			QList<QTreeWidgetItem *> result = ui->treeWidget->findItems(part, Qt::MatchFixedString);
			Q_ASSERT(result.length() < 2);
			if (result.length() == 0)
			{
				return nullptr;
			}

			parent = dynamic_cast<LCTreeWidgetItem *>(result[0]);
		}
		else
		{
			parent = parent->getChildByName(part);
			if (parent == nullptr)
			{
				return nullptr;
			}
		}
	}
	return parent;
}
