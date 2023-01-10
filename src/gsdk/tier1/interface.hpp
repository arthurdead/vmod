#pragma once

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

extern "C" __attribute__((__visibility__("default"))) void  * __attribute__((__cdecl__)) CreateInterface(const char *, int *);
