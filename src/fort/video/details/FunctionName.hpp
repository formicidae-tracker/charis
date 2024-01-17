#pragma once

#include <cpptrace/cpptrace.hpp>
#include <dlfcn.h>

template <typename Function> std::string FunctionName(Function &&fn) {
	Dl_info infos;
	if (dladdr(reinterpret_cast<void *>(&fn), &infos) != 0) {
		return cpptrace::demangle(infos.dli_sname);
	}
	return cpptrace::demangle(typeid(fn).name());
}
