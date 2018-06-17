#undef new
#undef delete
#undef LineFeed

#include <QDir>
#include "C4Group.h"
#include "../lcedit.h"

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
	ReturnValue<QByteArray> fileRead(LCTreeWidgetItem *item, off_t offset = 0, size_t size = 0) override;
	ReturnValue<int> fileWrite(LCTreeWidgetItem *item, const QByteArray &buf) override;
	
private:
	void openChildGroup(C4Group *grp, const QString &path);
	LCEdit *m_editor;
};
