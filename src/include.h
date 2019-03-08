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

#pragma once
#define TO_CSTR(x) ((x).toStdString().c_str())
#define SAFE_DELETE(x) if (x != nullptr) { delete x; x = nullptr; }
#define BLOCKSIZE 1024
#define TRANSFER_CONTENTS(x, y) while (!(x).atEnd()) { (y).write((x).read(BLOCKSIZE)); }

#include <QDir>
#include <optional>
#include "ui_lcedit.h"

class LCEdit;
class LCTreeWidgetItem;
class LCPluginInterface;
enum class ExecPolicy;

#define LCPlugin_Iid "org.legacyclonk.LegacyClonk.LCEdit.LCPluginInterface"
Q_DECLARE_INTERFACE(LCPluginInterface, LCPlugin_Iid)

#define CALL_PLUGINS(x) bool ret = false; \
foreach(LCPlugin obj, plugins) \
{ \
	switch (obj.plugin->x) \
	{ \
		case ExecPolicy::Continue: \
			continue; \
		[[fallthrough]] case ExecPolicy::AbortMain: \
			ret = true; \
		case ExecPolicy::AbortAll: \
			return; \
	} \
} \
if (ret) return;

#define CALL_PLUGINS_WITH_RET(t, x) \
	std::optional<t> ret; \
	foreach (LCPlugin obj, plugins) \
	{ \
		ret = obj.plugin->x; \
		if (ret) \
		{ \
			break; \
		} \
	}
