#pragma once

#include "../../gsdk/engine/dt_send.hpp"
#include "../../vscript/vscript.hpp"
#include "../../vscript/class_desc.hpp"
#include "../../ffi.hpp"
#include "../../plugin.hpp"
#include "../mem/singleton.hpp"
#include "../instance.hpp"

namespace vmod::bindings::ent
{
	class sendprop final : public plugin::callable, public instance_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		~sendprop() noexcept override;

	private:
		static ffi::cif proxy_cif;

		static vscript::class_desc<sendprop> desc;

		static ffi_type *guess_type(const gsdk::SendProp *prop, gsdk::SendVarProxyFn proxy, const gsdk::SendTable *table) noexcept;

		static void closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept;

		inline gsdk::HSCRIPT script_type() noexcept
		{ return type->table(); }

		sendprop(gsdk::SendProp *prop_) noexcept;

		void on_sleep() noexcept override;
		void on_wake() noexcept override;

		bool initialize_closure() noexcept;

		void remove_closure() noexcept;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

		gsdk::HSCRIPT script_hook_proxy(gsdk::HSCRIPT func, bool post, bool per_client) noexcept;

		gsdk::SendProp *prop;
		gsdk::SendVarProxyFn old_proxy;
		ffi_type *type_ptr;
		mem::singleton::type *type;

		ffi_closure *closure{nullptr};

	private:
		sendprop() = delete;
		sendprop(const sendprop &) = delete;
		sendprop &operator=(const sendprop &) = delete;
		sendprop(sendprop &&) = delete;
		sendprop &operator=(sendprop &&) = delete;
	};

	class sendtable final : public instance_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		~sendtable() noexcept override;

	private:
		static vscript::class_desc<sendtable> desc;

		inline sendtable(gsdk::SendTable *table_) noexcept
			: table{table_}
		{
		}

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

		gsdk::SendTable *table;

	private:
		sendtable() = delete;
		sendtable(const sendtable &) = delete;
		sendtable &operator=(const sendtable &) = delete;
		sendtable(sendtable &&) = delete;
		sendtable &operator=(sendtable &&) = delete;
	};

	class allocated_server_class
	{
	private:

	public:
		std::unique_ptr<gsdk::ServerClass> svclass;

	private:
		allocated_server_class() = delete;
		allocated_server_class(const allocated_server_class &) = delete;
		allocated_server_class &operator=(const allocated_server_class &) = delete;
		allocated_server_class(allocated_server_class &&) = delete;
		allocated_server_class &operator=(allocated_server_class &&) = delete;
	};
}
