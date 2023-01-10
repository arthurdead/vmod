#include "hacking.hpp"

namespace vmod
{
	const std::string &demangle(std::string_view mangled) noexcept
	{
		std::string &buffer{detail::demangle_buffer<void>};

		int status;
		std::size_t allocated;
		char *temp_buffer{__cxxabiv1::__cxa_demangle(mangled.data(), nullptr, &allocated, &status)};
		if(status == 0 && allocated > 0 && temp_buffer) {
			buffer = temp_buffer;
		}
		if(temp_buffer) {
			std::free(temp_buffer);
		}

		return buffer;
	}

	std::unique_ptr<__cxxabiv1::vtable_prefix> copy_prefix(__cxxabiv1::vtable_prefix *other, std::size_t num_funcs) noexcept
	{
		std::size_t funcs_size{sizeof(generic_plain_mfp_t) * num_funcs};
		std::unique_ptr<__cxxabiv1::vtable_prefix> copy{new (static_cast<__cxxabiv1::vtable_prefix *>(std::malloc(sizeof(__cxxabiv1::vtable_prefix) + funcs_size))) __cxxabiv1::vtable_prefix{*other}};
		std::memcpy(const_cast<void **>(&copy->origin), &other->origin, funcs_size);
		return copy;
	}
}
