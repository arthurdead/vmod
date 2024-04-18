#pragma once

#include <vector>
#include "../../plugin.hpp"
#include "../../ffi.hpp"
#include "../../hacking.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/class_desc.hpp"

namespace vmod::bindings::ffi
{
	class detour final : public vmod::ffi::cif, public plugin::callable, public plugin::shared_instance
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline detour(ffi_type *ret, std::vector<ffi_type *> &&args, std::vector<std::string> &&args_names) noexcept
			: vmod::ffi::cif{ret, std::move(args)}
		{
		}

		~detour() noexcept override;

		bool initialize(mfp_or_func_t old_target_, ffi_abi abi, bool member_) noexcept;

		void enable() noexcept;
		void disable() noexcept;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

		static vscript::class_desc<detour> desc;

		vscript::variant script_call(const vscript::variant *args, std::size_t num_args, ...) noexcept;
		vscript::handle_ref script_hook(vscript::handle_wrapper callback, bool post) noexcept;

		class callback_instance final : public plugin::callback_instance
		{
			friend class singleton;
			friend class detour;
			friend void write_docs(const std::filesystem::path &) noexcept;

		public:
			callback_instance(callable *caller_, vscript::handle_wrapper &&callback_, bool post_) noexcept = delete;

			callback_instance(detour *owner_, vscript::handle_wrapper &&callback_, bool post_) noexcept;

			~callback_instance() noexcept override;

		public:
			inline bool initialize() noexcept
			{ return register_instance(&desc, this); }

			detour *owner;
		};

	private:
		static void closure_binding(ffi_cif *cif, void *ret, void *args[], void *userptr) noexcept;

		static std::vector<vscript::variant> sargs;
		static const char uninitalized_str[17];

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

		unsigned char old_bytes[
		#ifdef __x86_64__
			5
		#else
			1
		#endif
		 + sizeof(std::uintptr_t)];

		bool member{false};
		bool enabled{false};

		static std::unordered_map<generic_func_t, std::unique_ptr<detour>> static_detours;
		static std::unordered_map<generic_internal_mfp_t, std::unique_ptr<detour>> member_detours;
	};
}
