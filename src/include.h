#pragma once
#define TO_CSTR(x) ((x).toStdString().c_str())
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
