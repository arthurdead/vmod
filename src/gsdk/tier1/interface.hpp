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

	enum InitReturnVal_t : int;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IAppSystem
	{
	public:
		virtual bool Connect(CreateInterfaceFn) = 0;
		virtual void Disconnect() = 0;
		virtual void *QueryInterface(const char *) = 0;
		virtual InitReturnVal_t Init() = 0;
		virtual void Shutdown() = 0;
	};
	#pragma GCC diagnostic pop
}
