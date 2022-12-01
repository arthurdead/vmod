#include "hacking.hpp"

namespace vmod
{
	detour::~detour()
	{
		if(old_func) {
			disable();
		}
	}

	void detour::initialize() noexcept
	{
		page_info func_page{reinterpret_cast<void *>(old_func), 2};
		func_page.protect(PROT_READ|PROT_WRITE|PROT_EXEC);

		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_func)};

		std::memcpy(old_bytes, bytes, sizeof(old_bytes));
	}

	void detour::enable() noexcept
	{
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_func)};

		bytes[0] = 0xE9;

		std::uintptr_t target{reinterpret_cast<std::uintptr_t>(new_func) - (reinterpret_cast<std::uintptr_t>(old_func) + sizeof(old_bytes))};
		std::memcpy(bytes + 1, &target, sizeof(std::uintptr_t));
	}
}
