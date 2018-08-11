#pragma once
#define TO_CSTR(x) ((x).toStdString().c_str())
#define SAFE_DELETE(x) if (x != nullptr) { delete x; x = nullptr; }
#define BLOCKSIZE 1024
#define TRANSFER_CONTENTS(x, y) while (!(x).atEnd()) { (y).write((x).read(BLOCKSIZE)); }

#include <QDir>
#include "ui_lcedit.h"

class LCEdit;
class LCTreeWidgetItem;
class LCPluginInterface;
enum class ExecPolicy;

#define LCPlugin_Iid "org.legacyclonk.LegacyClonk.LCEdit.LCPluginInterface"
Q_DECLARE_INTERFACE(LCPluginInterface, LCPlugin_Iid)

#define CALL_PLUGINS(x) bool ret = false; \
foreach(LCPluginInterface *obj, plugins) \
{ \
	switch (obj->x) \
	{ \
		case ExecPolicy::Continue: \
			continue; \
		case ExecPolicy::AbortMain: \
			ret = true; \
			[[fallthrough]]; \
		case ExecPolicy::AbortAll: \
			return; \
	} \
} \
if (ret) return;

#define CALL_PLUGINS_WITH_RET(t, x) \
	ReturnValue<t> ret(ExecPolicy::Continue, static_cast<t>(NULL)); \
	foreach (LCPluginInterface *plugin, plugins) \
	{ \
		ret = plugin->x; \
		if (ret.code != ExecPolicy::Continue) \
		{ \
			break; \
		} \
	}
