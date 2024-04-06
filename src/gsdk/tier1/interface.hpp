#pragma once

#include "../../platform.hpp"

namespace gsdk
{
	enum : int
	{
		IFACE_OK,
		IFACE_FAILED
	};

	using CreateInterfaceFn = void *(*)(const char *, int *);

	class IBaseInterface
	{
	public:
		virtual	~IBaseInterface() = 0;
	};
}

extern "C" __attribute__((__visibility__("default"))) void  * VMOD_KATTR_CDECL CreateInterface(const char *, int *);
