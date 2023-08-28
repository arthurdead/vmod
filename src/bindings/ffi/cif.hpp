#pragma once

#include <vector>
#include <cstddef>
#include "../../plugin.hpp"
#include "../../ffi.hpp"
#include "../../hacking.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/class_desc.hpp"

namespace vmod::bindings::ffi
{
	class singleton;

	class caller final : public vmod::ffi::cif, public plugin::owned_instance
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		~caller() noexcept override;

	private:
		static vscript::class_desc<caller> desc;

		inline caller(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: vmod::ffi::cif{ret, std::move(args)}, target_ptr{nullptr}, virt{false}
		{
		}

		bool initialize(ffi_abi abi) noexcept;

		vscript::variant script_call(const vscript::variant *args, std::size_t num_args, ...) noexcept;

		void script_set_func(generic_func_t func_) noexcept;
		void script_set_mfp(generic_mfp_t func_) noexcept;
		void script_set_vidx(std::size_t func_) noexcept;

		union {
			mfp_or_func_t target_ptr;
			std::size_t target_idx;
		};
		bool virt;

	private:
		caller() = delete;
		caller(const caller &) = delete;
		caller &operator=(const caller &) = delete;
		caller(caller &&) = delete;
		caller &operator=(caller &&) = delete;
	};
}
