#include "gsdk_library.hpp"
#include <dlfcn.h>

namespace vmod
{
	bool gsdk_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;

		dl = dlopen(path.c_str(),
		#ifndef __VMOD_COMPILING_VTABLE_DUMPER
			RTLD_LAZY|RTLD_LOCAL|RTLD_NODELETE|RTLD_NOLOAD
		#else
			RTLD_LAZY|RTLD_LOCAL
		#endif
		);
		if(!dl) {
			const char *err{dlerror()};
			if(err && err[0] != '\0') {
				err_str = err;
			} else {
				err_str = "unknown error"s;
			}
			return false;
		} else {
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			iface_fac = reinterpret_cast<gsdk::CreateInterfaceFn>(dlsym(dl, "CreateInterface"));
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
			if(!iface_fac) {
				err_str = "missing CreateInterface export"s;
				return false;
			}

			Dl_info base_info;
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			if(dladdr(reinterpret_cast<const void *>(iface_fac), &base_info) == 0) {
				err_str = "failed to get base address"s;
				return false;
			}
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif

			base_addr = base_info.dli_fbase;
		}

		return true;
	}

	void *gsdk_library::find_iface(std::string_view name) noexcept
	{
		int status{gsdk::IFACE_OK};
		void *iface{iface_fac(name.data(), &status)};
		if(!iface || status != gsdk::IFACE_OK) {
			return nullptr;
		}
		return iface;
	}

	void *gsdk_library::find_addr(std::string_view name) noexcept
	{
		return dlsym(dl, name.data());
	}

	void gsdk_library::unload() noexcept
	{
		if(dl) {
			dlclose(dl);
			dl = nullptr;
		}
	}

	gsdk_library::~gsdk_library() noexcept
	{
		if(dl) {
			dlclose(dl);
		}
	}
}
