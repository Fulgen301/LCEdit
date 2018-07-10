#pragma once
#define TO_CSTR(x) ((x).toStdString().c_str())
#define SAFE_DELETE(x) if (x != nullptr) { delete x; x = nullptr; }
#define BLOCKSIZE 1024
#define TRANSFER_CONTENTS(x, y) while (!(x).atEnd()) { (y).write((x).read(BLOCKSIZE)); }
#include "ui_lcedit.h"

enum class ExecPolicy {
	Continue = 1,
	AbortMain,
	AbortAll
};

template<class T> struct ReturnValue {
	ExecPolicy code;
	T value;
	ReturnValue(ExecPolicy c = ExecPolicy::Continue, T v = nullptr) : code(c), value(v) {}
};
