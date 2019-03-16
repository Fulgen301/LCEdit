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

#pragma once
#include <QDir>
#include <QIODevice>
#include <QJsonObject>
#include <QSharedPointer>
#include <functional>
#include <optional>

#define CALL_PLUGINS(x) bool ret = false; \
foreach(LCPlugin obj, plugins) \
{ \
	switch (obj.plugin->x) \
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

class LCEdit;
class LCTreeWidgetItem;
class LCPluginInterface;

enum class ExecPolicy {
	Continue = 1,
	AbortMain,
	AbortAll
};

struct LCDeviceInformation
{
	QIODevice *device;
	std::function<bool()> deleter;
};

typedef QSharedPointer<QIODevice> QIODevicePtr;

class LCPluginInterface
{
public:
	virtual ~LCPluginInterface() {}
	virtual void init(LCEdit *editor) = 0;
	virtual ExecPolicy createTree(const QDir &base, LCTreeWidgetItem *parent) = 0;
	virtual ExecPolicy treeItemChanged(LCTreeWidgetItem *current, LCTreeWidgetItem *previous) = 0;
	virtual std::optional<LCDeviceInformation> getDevice(LCTreeWidgetItem *item) = 0;
};

#define LCPlugin_Iid "org.legacyclonk.LegacyClonk.LCEdit.LCPluginInterface"
Q_DECLARE_INTERFACE(LCPluginInterface, LCPlugin_Iid)

struct LCPlugin {
	LCPluginInterface *plugin;
	QJsonObject metaData;
};
