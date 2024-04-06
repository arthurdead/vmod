namespace vmod
{
	namespace detail
	{
		template <typename T>
		static std::string demangle_buffer;
	}

	template <typename T>
	const std::string &demangle() noexcept
	{
		std::string &buffer{detail::demangle_buffer<T>};

		if(buffer.empty()) {
			const char *mangled{typeid(T).name()};

			int status;
			std::size_t allocated;
			char *temp_buffer{__cxxabiv1::__cxa_demangle(mangled, nullptr, &allocated, &status)};
			if(status == 0 && allocated > 0 && temp_buffer) {
				buffer = temp_buffer;
			}
			if(temp_buffer) {
				std::free(temp_buffer);
			}
		}

		return buffer;
	}

	template <typename T>
	void detour_base<T>::enable() noexcept
	{
		if(!old_target) {
			return;
		}

	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif

	#ifdef __x86_64__
		bytes[0] = 0x49;
		bytes[1] = 0xBA;

		std::uintptr_t target{reinterpret_cast<std::uintptr_t>(new_target.plain)};
		std::memcpy(bytes + 2, &target, sizeof(std::uintptr_t));

		bytes[10] = 0x41;
		bytes[11] = 0xFF;
		bytes[12] = 0xE2;
	#else
		bytes[0] = 0xE9;

		std::uintptr_t target{reinterpret_cast<std::uintptr_t>(new_target.plain) - (reinterpret_cast<std::uintptr_t>(old_target.mfp.addr) + sizeof(old_bytes))};
		std::memcpy(bytes + 1, &target, sizeof(std::uintptr_t));
	#endif
	}

	template <typename T>
	void detour_base<T>::backup_bytes() noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif

		page_info func_page{bytes, sizeof(old_bytes)};
		func_page.protect(PROT_READ|PROT_WRITE|PROT_EXEC);

		std::memcpy(old_bytes, bytes, sizeof(old_bytes));
	}
}
