#pragma once

#include <vector>
#include "../../plugin.hpp"
#include "../../ffi.hpp"
#include "../../hacking.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/class_desc.hpp"

namespace vmod::bindings::ffi
{
	class detour final : public vmod::ffi::cif, public plugin::owned_instance
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline detour(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: vmod::ffi::cif{ret, std::move(args)}
		{
		}

		~detour() noexcept override;

		bool initialize(mfp_or_func_t old_target_, gsdk::HSCRIPT callback, ffi_abi abi) noexcept;

		void enable() noexcept;

		inline void disable() noexcept
		{
			unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
			std::memcpy(bytes, old_bytes, sizeof(old_bytes));
		}

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		static vscript::class_desc<detour> desc;

		gsdk::ScriptVariant_t script_call(const vscript::variant *args, std::size_t num_args, ...) noexcept;

		inline void script_enable() noexcept
		{ enable(); }
		inline void script_disable() noexcept
		{ disable(); }

		bool initialize_shared(gsdk::HSCRIPT callback, ffi_abi abi) noexcept;

		static void closure_binding(ffi_cif *cif, void *ret, void *args[], void *userptr) noexcept;

		struct scope_enable final {
			inline scope_enable(detour &det_) noexcept
				: det{det_} {
				det.disable();
			}
			inline ~scope_enable() noexcept {
				det.enable();
			}
			detour &det;
		};

		void backup_bytes() noexcept;

		ffi_closure *closure{nullptr};
		generic_func_t closure_func;

		gsdk::HSCRIPT script_target{gsdk::INVALID_HSCRIPT};

		mfp_or_func_t old_target;

		unsigned char old_bytes[1 + sizeof(std::uintptr_t)];
	};
}
