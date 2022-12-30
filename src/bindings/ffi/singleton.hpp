#pragma once

#include "../../ffi.hpp"
#include "../../hacking.hpp"
#include "../../vscript/vscript.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../singleton.hpp"
#include <vector>

namespace vmod::bindings::ffi
{
	class detour;

	class singleton final : public singleton_base
	{
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline singleton() noexcept
			: singleton_base{"ffi"}
		{
		}

		~singleton() noexcept override;

		bool bindings() noexcept;
		void unbindings() noexcept;

		static singleton &instance() noexcept;

	private:
		static vscript::singleton_class_desc<singleton> desc;

		static bool script_create_cif_shared(std::vector<ffi_type *> &args_types, gsdk::HSCRIPT args) noexcept;
		static gsdk::HSCRIPT script_create_static_cif(ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept;
		static gsdk::HSCRIPT script_create_member_cif(ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept;

		static detour *script_create_detour_shared(ffi_type *ret, gsdk::HSCRIPT args, bool member) noexcept;
		static gsdk::HSCRIPT script_create_detour_member(mfp_or_func_t old_target, gsdk::HSCRIPT callback, ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept;
		static gsdk::HSCRIPT script_create_detour_static(mfp_or_func_t old_target, gsdk::HSCRIPT callback, ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept;

		gsdk::HSCRIPT types_table{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT abi_table{gsdk::INVALID_HSCRIPT};
	};
}
