#pragma once

#include <vector>
#include "../../plugin.hpp"
#include "../../ffi.hpp"
#include "../../hacking.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/class_desc.hpp"

namespace vmod::bindings::ffi
{
	class detour final : public vmod::ffi::cif, public plugin::callable
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline detour(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: vmod::ffi::cif{ret, std::move(args)}
		{
		}

		~detour() noexcept override;

		bool initialize(mfp_or_func_t old_target_, ffi_abi abi) noexcept;

		void enable() noexcept;

		inline void disable() noexcept
		{
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
			std::memcpy(bytes, old_bytes, sizeof(old_bytes));
		}

		static bool bindings() noexcept;
		static void unbindings() noexcept;

		class callback_instance final : public plugin::callback_instance
		{
			friend class singleton;
			friend class detour;
			friend void write_docs(const std::filesystem::path &) noexcept;

		public:
			callback_instance(callable *caller_, gsdk::HSCRIPT callback_, bool post_) noexcept = delete;

			callback_instance(detour *owner_, gsdk::HSCRIPT callback_, bool post_) noexcept;

			~callback_instance() noexcept override;

		private:
			static vscript::class_desc<callback_instance> desc;

		public:
			inline bool initialize() noexcept
			{ return register_instance(&desc, this); }

		private:
			gsdk::ScriptVariant_t script_call(const vscript::variant *args, std::size_t num_args, ...) noexcept;

			detour *owner;
		};

	private:
		static void closure_binding(ffi_cif *cif, void *ret, void *args[], void *userptr) noexcept;

		void on_sleep() noexcept override;
		void on_wake() noexcept override;

		void backup_bytes() noexcept;

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

		ffi_closure *closure{nullptr};
		generic_func_t closure_func;

		mfp_or_func_t old_target;

		unsigned char old_bytes[1 + sizeof(std::uintptr_t)];

		static std::unordered_map<generic_func_t, std::unique_ptr<detour>> static_detours;
		static std::unordered_map<generic_internal_mfp_t, std::unique_ptr<detour>> member_detours;
	};
}
