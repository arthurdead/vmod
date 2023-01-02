#include "singleton.hpp"
#include "../../main.hpp"

namespace vmod::bindings::syms
{
	vscript::singleton_class_desc<singleton> singleton::desc{"syms"};
	vscript::class_desc<singleton::qualification_it> singleton::qualification_it::desc{"syms::qualification"};
	vscript::class_desc<singleton::name_it> singleton::name_it::desc{"syms::name"};

	static sv sv_;

	singleton::qualification_it::~qualification_it() noexcept {}
	singleton::name_it::~name_it() noexcept {}

	bool singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		desc.func(&singleton::script_lookup, "script_lookup"sv, "lookup"sv);
		desc.func(&singleton::script_lookup_global, "script_lookup_global"sv, "lookup_global"sv);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register syms singleton class\n"sv);
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		
	}

	bool singleton::qualification_it::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&qualification_it::script_name, "script_name"sv, "name"sv);
		desc.func(&qualification_it::script_lookup, "script_lookup"sv, "lookup"sv);

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register syms qualification class\n"sv);
			return false;
		}

		return true;
	}

	void singleton::qualification_it::unbindings() noexcept
	{
		
	}

	bool singleton::name_it::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&name_it::script_name, "script_name"sv, "name"sv);
		desc.func(&name_it::script_addr, "script_addr"sv, "addr"sv);
		desc.func(&name_it::script_func, "script_func"sv, "func"sv);
		desc.func(&name_it::script_mfp, "script_mfp"sv, "mfp"sv);
		desc.func(&name_it::script_size, "script_size"sv, "size"sv);
		desc.func(&name_it::script_vindex, "script_vindex"sv, "vidx"sv);
		desc.func(&name_it::script_lookup, "script_lookup"sv, "lookup"sv);

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register syms name class\n"sv);
			return false;
		}

		return true;
	}

	void singleton::name_it::unbindings() noexcept
	{
		
	}

	gsdk::HSCRIPT singleton::script_lookup_shared(symbol_cache::const_iterator it) noexcept
	{
		qualification_it *script_it{new qualification_it{it}};
		if(!script_it->initialize()) {
			delete script_it;
			return gsdk::INVALID_HSCRIPT;
		}

		return script_it->instance;
	}

	gsdk::HSCRIPT singleton::script_lookup_shared(symbol_cache::qualification_info::const_iterator it) noexcept
	{
		name_it *script_it{new name_it{it}};
		if(!script_it->initialize()) {
			delete script_it;
			return gsdk::INVALID_HSCRIPT;
		}

		return script_it->instance;
	}

	gsdk::HSCRIPT singleton::qualification_it::script_lookup(std::string_view symname) const noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(symname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", symname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		std::string symname_tmp{symname};
		auto tmp_it{it->second->find(symname_tmp)};
		if(tmp_it == it->second->end()) {
			return gsdk::INVALID_HSCRIPT;
		}

		return script_lookup_shared(tmp_it);
	}

	gsdk::HSCRIPT singleton::name_it::script_lookup(std::string_view symname) const noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(symname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", symname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		std::string symname_tmp{symname};
		auto tmp_it{it->second->find(symname_tmp)};
		if(tmp_it == it->second->end()) {
			return gsdk::INVALID_HSCRIPT;
		}

		return script_lookup_shared(tmp_it);
	}

	gsdk::HSCRIPT singleton::script_lookup(std::string_view symname) const noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(symname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", symname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		const symbol_cache &syms{cache()};

		std::string symname_tmp{symname};
		auto it{syms.find(symname_tmp)};
		if(it == syms.end()) {
			return gsdk::INVALID_HSCRIPT;
		}

		return script_lookup_shared(it);
	}

	gsdk::HSCRIPT singleton::script_lookup_global(std::string_view symname) const noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(symname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", symname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		const symbol_cache::qualification_info &syms{cache().global()};

		std::string symname_tmp{symname};
		auto it{syms.find(symname_tmp)};
		if(it == syms.end()) {
			return gsdk::INVALID_HSCRIPT;
		}

		return script_lookup_shared(it);
	}

	bool singleton::initialize() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		instance = vm->RegisterInstance(&desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to register '%s' syms singleton instance\n"sv, name.data());
			return false;
		}

		std::string_view obfuscated_name;

		if(desc.m_pNextDesc == reinterpret_cast<const gsdk::ScriptClassDesc_t *>(uninitialized_memory)) {
			const vscript::extra_class_desc &extra{reinterpret_cast<const vscript::detail::base_class_desc<generic_class> *>(&desc)->extra()};
			obfuscated_name = extra.obfuscated_name();
		} else {
			error("vmod: invalid class desc for '%s' syms singleton\n", name.data());
			return false;
		}

		{
			std::string id_root{obfuscated_name};
			id_root += "_instance"sv;

			if(!vm->SetInstanceUniqeId2(instance, id_root.data())) {
				error("vmod: failed to generate unique id for '%s' syms singleton\n", name.data());
				return false;
			}
		}

		if(!vm->SetValue(main::instance().symbols_table(), name.data(), instance)) {
			error("vmod: failed to set syms '%s' table value\n"sv, name.data());
			return false;
		}

		return true;
	}

	singleton::~singleton() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}

		gsdk::HSCRIPT symbols_table{main::instance().symbols_table()};
		if(vm->ValueExists(symbols_table, name.data())) {
			vm->ClearValue(symbols_table, name.data());
		}
	}

	const symbol_cache &sv::cache() const noexcept
	{ return main::instance().sv_syms(); }
}

namespace vmod
{
	bool main::create_script_symbols() noexcept
	{
		if(!bindings::syms::sv_.initialize()) {
			return false;
		}

		return true;
	}
}
