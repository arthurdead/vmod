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

		static bool script_create_cif_shared(std::vector<ffi_type *> &args_types, vscript::array_handle_ref args) noexcept;
		static vscript::instance_handle_ref script_create_static_cif(ffi_abi abi, ffi_type *ret, vscript::array_handle_ref args) noexcept;
		static vscript::instance_handle_ref script_create_member_cif(ffi_type *ret, ffi_type *this_type, vscript::array_handle_ref args) noexcept;

		static bool script_create_detour_shared(mfp_or_func_t old_target, ffi_type *ret, vscript::array_handle_ref args, std::vector<ffi_type *> &args_types, std::vector<std::string> &args_names) noexcept;
		static vscript::instance_handle_ref script_create_detour_member(mfp_or_func_t old_target, ffi_type *ret, ffi_type *this_type, vscript::array_handle_ref args) noexcept;
		static vscript::instance_handle_ref script_create_detour_static(mfp_or_func_t old_target, ffi_abi abi, ffi_type *ret, vscript::array_handle_ref args) noexcept;

		vscript::table_handle_wrapper types_table{};
		vscript::table_handle_wrapper this_types_table{};
		vscript::table_handle_wrapper abi_table{};
	};
}
