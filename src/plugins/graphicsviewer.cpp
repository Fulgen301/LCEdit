//Copyright (c) 2019, George Tokmaji

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

#include "graphicsviewer.h"
#include <QImageReader>

void GraphicsViewerPlugin::init(LCEdit *editor)
{
	m_editor = editor;

	pixmapItem = new QGraphicsPixmapItem;
	scene.addItem(pixmapItem);

	view = new QGraphicsView(&scene);
	m_editor->ui->layRight->insertWidget(0, view);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setMaximumHeight(150);
	view->hide();
}

GraphicsViewerPlugin::~GraphicsViewerPlugin()
{
}

ExecPolicy GraphicsViewerPlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	Q_UNUSED(base);
	Q_UNUSED(parent);
	return ExecPolicy::Continue;
}

ExecPolicy GraphicsViewerPlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	Q_UNUSED(previous);
	if (current == nullptr)
	{
		return ExecPolicy::Continue;
	}

	bool success = false;
	QIODevicePtr device = m_editor->getDevice(current);
	if (!device.isNull() && device->open(QIODevice::ReadOnly))
	{
		QImageReader reader(device.get());

		auto pixmap = QPixmap::fromImageReader(&reader);
		success = !pixmap.isNull();
		pixmapItem->setPixmap(success ? pixmap.scaled(view->width(), view->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation) : pixmap);
		scene.update();
		view->setSceneRect(pixmapItem->boundingRect());
		device->close();
	}
	view->setVisible(success);
	return ExecPolicy::Continue;
}

std::optional<LCDeviceInformation> GraphicsViewerPlugin::getDevice(LCTreeWidgetItem *item)
{
	Q_UNUSED(item);
	return std::nullopt;
}
