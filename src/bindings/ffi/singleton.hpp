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

		static bool script_create_cif_shared(std::vector<ffi_type *> &args_types, vscript::handle_ref args) noexcept;
		static vscript::handle_ref script_create_static_cif(ffi_abi abi, ffi_type *ret, vscript::handle_wrapper args) noexcept;
		static vscript::handle_ref script_create_member_cif(ffi_type *ret, ffi_type *this_type, vscript::handle_wrapper args) noexcept;

		static bool script_create_detour_shared(mfp_or_func_t old_target, vscript::handle_ref callback, ffi_type *ret, vscript::handle_ref args, std::vector<ffi_type *> &args_types) noexcept;
		static vscript::handle_ref script_create_detour_member(mfp_or_func_t old_target, vscript::handle_wrapper callback, ffi_type *ret, ffi_type *this_type, vscript::handle_wrapper args, bool post) noexcept;
		static vscript::handle_ref script_create_detour_static(mfp_or_func_t old_target, vscript::handle_wrapper callback, ffi_abi abi, ffi_type *ret, vscript::handle_wrapper args, bool post) noexcept;

		vscript::handle_wrapper types_table{};
		vscript::handle_wrapper this_types_table{};
		vscript::handle_wrapper abi_table{};
	};
}
