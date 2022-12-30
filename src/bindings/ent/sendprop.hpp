#pragma once

#include "../../gsdk/engine/dt_send.hpp"
#include "../../vscript/vscript.hpp"
#include "../../vscript/class_desc.hpp"
#include "../../ffi.hpp"

namespace vmod::bindings::ent
{
	class sendprop final
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		~sendprop() noexcept;

	private:
		static ffi::cif proxy_cif;

		static vscript::class_desc<sendprop> desc;

		static ffi_type *guess_type(const gsdk::SendProp *prop, gsdk::SendVarProxyFn proxy, const gsdk::SendTable *table) noexcept;

		static void closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept;

		inline sendprop(gsdk::SendProp *prop_) noexcept
			: prop{prop_}, old_proxy{prop_->m_ProxyFn}, type{guess_type(prop_, prop_->m_ProxyFn, nullptr)}
		{
		}

		bool initialize_closure() noexcept;

		bool initialize() noexcept;

		gsdk::HSCRIPT script_hook_proxy(gsdk::HSCRIPT func, bool post, bool per_client) noexcept;

		gsdk::SendProp *prop;
		gsdk::SendVarProxyFn old_proxy;
		ffi_type *type;

		ffi_closure *closure{nullptr};

		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};

	private:
		sendprop() = delete;
		sendprop(const sendprop &) = delete;
		sendprop &operator=(const sendprop &) = delete;
		sendprop(sendprop &&) = delete;
		sendprop &operator=(sendprop &&) = delete;
	};
}
