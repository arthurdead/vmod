#pragma once

namespace gsdk
{
	enum SpewType_t : int
	{
		SPEW_MESSAGE,
		SPEW_WARNING,
		SPEW_ASSERT,
		SPEW_ERROR,
		SPEW_LOG,
		SPEW_TYPE_COUNT
	};

	enum SpewRetval_t : int
	{
		SPEW_DEBUGGER,
		SPEW_CONTINUE,
		SPEW_ABORT
	};

	using SpewOutputFunc_t = SpewRetval_t(*)(SpewType_t, const char *);
}

extern "C" void __attribute__((__visibility__("default"))) SpewOutputFunc(gsdk::SpewOutputFunc_t);
extern "C" gsdk::SpewOutputFunc_t __attribute__((__visibility__("default"))) GetSpewOutputFunc();
