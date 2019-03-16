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

#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QSettings>
#include "workspaces.h"

ExecPolicy WorkspacePlugin::createTree(const QDir &base, LCTreeWidgetItem *parent)
{
	Q_UNUSED(base);
	Q_UNUSED(parent);
	return ExecPolicy::Continue;
}

void WorkspacePlugin::init(LCEdit *editor)
{
	m_editor = editor;
	delete m_editor->ui->actPath;

	QSettings settings;
	settings.beginGroup("Workspaces");
	auto *menu = new QMenu(m_editor->tr("Arbeitsverzeichnisse"));

	for (const QString &key : settings.childKeys())
	{
		menu->addAction(key, [key = settings.value(key).toString(), this]()
		{
			m_editor->settings.setValue("Path", key);
			m_editor->reload();
		});
	}

	m_editor->ui->menuActions->insertMenu(
				m_editor->ui->menuActions->addAction(m_editor->tr("Arbeitsverzeichnis hinzufügen..."), [menu, this]()
	{
		QString path = QFileDialog::getExistingDirectory(
							  nullptr,
							  tr("Arbeitsverzeichnis hinzufügen"),
							  m_editor->settings.value("Path").toString()
							  );
		if (path.isEmpty())
		{
			return;
		}
		QString title = QInputDialog::getText(
					nullptr,
					m_editor->tr("Arbeitsverzeichnis hinzufügen"),
					m_editor->tr("Titel für das Arbeitsverzeichnis:")
					);

		if (title.isEmpty())
		{
			return;
		}

		m_editor->settings.setValue(QStringLiteral("Workspaces/%1").arg(title), path);
		menu->addAction(title, [path, this]()
		{
			m_editor->settings.setValue("Path", path);
			m_editor->reload();
		});
	}), menu);
}

WorkspacePlugin::~WorkspacePlugin()
{
}

ExecPolicy WorkspacePlugin::treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous)
{
	Q_UNUSED(current);
	Q_UNUSED(previous);
	return ExecPolicy::Continue;
}

std::optional<LCDeviceInformation> WorkspacePlugin::getDevice(LCTreeWidgetItem *item)
{
	Q_UNUSED(item);
	return std::nullopt;
}
