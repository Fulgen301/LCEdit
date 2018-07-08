#pragma once
#define TO_CSTR(x) ((x).toStdString().c_str())
#define SAFE_DELETE(x) if (x != nullptr) { delete x; x = nullptr; }
#define BLOCKSIZE 1024
#define TRANSFER_CONTENTS(x, y) while (!(x).atEnd()) { (y).write((x).read(BLOCKSIZE)); }
#include "ui_lcedit.h"

enum ExecPolicy {
	EP_Continue = 1,
	EP_AbortMain,
	EP_AbortAll
};

template<class T> struct ReturnValue {
	ExecPolicy code;
	T value;
	ReturnValue(ExecPolicy c = EP_Continue, T v = nullptr) : code(c), value(v) {}
};
