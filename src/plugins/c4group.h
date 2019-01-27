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

#include <QDir>
#include <QHash>
#include <QList>
#include <QTimer>
#include "../lcedit.h"
#include "../lib/c4group.h"


class C4GroupPlugin : public QObject, public LCPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID LCPlugin_Iid FILE "c4group.json")
	Q_INTERFACES(LCPluginInterface)

public:
	void init(LCEdit *editor) override;
	ExecPolicy createTree(const QDir & base, LCTreeWidgetItem * parent) override;
	ExecPolicy treeItemChanged(LCTreeWidgetItem * current, LCTreeWidgetItem * previous) override;
	ReturnValue<QIODevice *> getDevice(LCTreeWidgetItem *item) override;
	ReturnValue<bool> destroyDevice(LCTreeWidgetItem *item, QIODevice *device) override;

private:
	void createRealTree(LCTreeWidgetItem *parent, C4GroupDirectory *dir);
	LCEdit *m_editor;
	QHash<LCTreeWidgetItem *, C4Group *> rootItems;
private slots:
	void itemCollapsed(QTreeWidgetItem *item);
};
