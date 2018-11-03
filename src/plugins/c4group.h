#include <QDir>
#include <QHash>
#include <QList>
#include <QTimer>
#include "../lcedit.h"
#include "../lib/c4group.h"


class C4GroupPlugin : public QObject, public LCPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID LCPlugin_Iid)
	Q_INTERFACES(LCPluginInterface)

public:
	void init(LCEdit *editor) override;
	ExecPolicy createTree(const QDir & base, LCTreeWidgetItem * parent) override;
	int priority() override;
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
