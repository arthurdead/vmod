#include "singleton.hpp"
#include "../gsdk.hpp"
#include "../main.hpp"
#include "../gsdk/tier1/utlstring.hpp"

namespace vmod::bindings
{
	bool singleton_base::Get(const gsdk::CUtlConstString &getname, gsdk::ScriptVariant_t &value)
	{
		if(vscript::vm()->GetValue(*instance, getname.data(), &value)) {
			return true;
		}

		error("vmod: member '%s' not found in '%s'\n", getname.data(), name.data());

		return false;
	}

	bool singleton_base::bindings(gsdk::ScriptClassDesc_t *desc) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		instance = vm->RegisterInstance(desc, this);
		if(!instance) {
			error("vmod: failed to register '%s' singleton instance\n"sv, name.data());
			return false;
		}

		std::string_view obfuscated_name;

		if(desc->m_pNextDesc == reinterpret_cast<const gsdk::ScriptClassDesc_t *>(uninitialized_memory)) {
			const vscript::extra_class_desc &extra{static_cast<const vscript::detail::base_class_desc<generic_class> *>(desc)->extra()};
			obfuscated_name = extra.obfuscated_name();
		} else {
			error("vmod: invalid class desc for '%s' singleton\n", name.data());
			return false;
		}

		{
			std::string id_root{obfuscated_name};
			id_root += "_instance"sv;

			if(!vm->SetInstanceUniqeId2(*instance, id_root.data())) {
				error("vmod: failed to generate unique id for '%s' singleton\n", name.data());
				return false;
			}
		}

		{
			std::string scope_name{obfuscated_name};
			scope_name += "_scope__"sv;

			scope = vm->CreateScope(scope_name.c_str(), nullptr);
			if(!scope) {
				error("vmod: failed to create '%s' scope\n"sv, name.data());
				return false;
			}
		}

		vscript::scope_handle_ref target_scope{root ? nullptr : *main::instance().scope};

		if(!vm->SetValue(*target_scope, name.data(), *scope)) {
			error("vmod: failed to set '%s' scope value\n"sv, name.data());
			return false;
		}

		return true;
	}

	bool singleton_base::create_get() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		vscript::scope_handle_ref target_scope{root ? nullptr : *main::instance().scope};

	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		get_impl = vm->MakeSquirrelMetamethod_Get(*target_scope, name.data(), this, false);
		if(!get_impl) {
			warning("vmod: failed to create '%s' _get metamethod\n"sv, name.data());
		}
	#endif

		return true;
	}

	void singleton_base::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		vscript::scope_handle_ref target_scope{root ? nullptr : *main::instance().scope};

		if(target_scope) {
			if(vm->ValueExists(*target_scope, name.data())) {
				vm->ClearValue(*target_scope, name.data());
			}
		}

		scope.free();

	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(get_impl) {
			vm->DestroySquirrelMetamethod_Get(get_impl);
		}
	#endif

		instance.free();
	}
}
