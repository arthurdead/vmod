#include "singleton.hpp"
#include "../../main.hpp"
#include "../../gsdk.hpp"
#include "../../convar.hpp"
#include "convar.hpp"
#include <memory>

namespace vmod::bindings::cvar
{
	vscript::singleton_class_desc<singleton> singleton::desc{"cvar"};

	static singleton cvar_;

	singleton &singleton::instance() noexcept
	{ return cvar_; }

	singleton::~singleton() noexcept {}

	bool singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		desc.func(&singleton::script_create_cvar, "script_create_cvar"sv, "create_var"sv)
		.desc("[convar_impl](name, value)"sv);

		desc.func(&singleton::script_find_cvar, "script_find_cvar"sv, "find_var"sv)
		.desc("[convar_ref](name)"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		flags_table = vm->CreateTable();
		if(!flags_table || flags_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create cvar flags table\n"sv);
			return false;
		}

		{
			if(!vm->SetValue(flags_table, "none", vscript::variant{gsdk::FCVAR_NONE})) {
				error("vmod: failed to set cvar none flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "development", vscript::variant{gsdk::FCVAR_DEVELOPMENTONLY})) {
				error("vmod: failed to set cvar development flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "hidden", vscript::variant{gsdk::FCVAR_HIDDEN})) {
				error("vmod: failed to set cvar hidden flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "protected", vscript::variant{gsdk::FCVAR_PROTECTED})) {
				error("vmod: failed to set cvar protected flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "singleplayer", vscript::variant{gsdk::FCVAR_SPONLY})) {
				error("vmod: failed to set cvar singleplayer flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "printable_only", vscript::variant{gsdk::FCVAR_PRINTABLEONLY})) {
				error("vmod: failed to set cvar printable_only flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "never_string", vscript::variant{gsdk::FCVAR_NEVER_AS_STRING})) {
				error("vmod: failed to set cvar never_string flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "server", vscript::variant{gsdk::FCVAR_GAMEDLL})) {
				error("vmod: failed to set cvar server flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "client", vscript::variant{gsdk::FCVAR_CLIENTDLL})) {
				error("vmod: failed to set cvar client flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "archive", vscript::variant{gsdk::FCVAR_ARCHIVE})) {
				error("vmod: failed to set cvar archive flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "notify", vscript::variant{gsdk::FCVAR_NOTIFY})) {
				error("vmod: failed to set cvar notify flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "userinfo", vscript::variant{gsdk::FCVAR_USERINFO})) {
				error("vmod: failed to set cvar userinfo flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "cheat", vscript::variant{gsdk::FCVAR_CHEAT})) {
				error("vmod: failed to set cvar cheat flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "replicated", vscript::variant{gsdk::FCVAR_REPLICATED})) {
				error("vmod: failed to set cvar replicated flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "only_unconnected", vscript::variant{gsdk::FCVAR_NOT_CONNECTED})) {
				error("vmod: failed to set cvar only_unconnected flag value\n"sv);
				return false;
			}

		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			if(!vm->SetValue(flags_table, "allowed_in_competitive", vscript::variant{gsdk::FCVAR_ALLOWED_IN_COMPETITIVE})) {
				error("vmod: failed to set cvar allowed_in_competitive flag value\n"sv);
				return false;
			}
		#endif

		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			if(!vm->SetValue(flags_table, "internal", vscript::variant{gsdk::FCVAR_INTERNAL_USE})) {
				error("vmod: failed to set cvar internal flag value\n"sv);
				return false;
			}
		#endif

			if(!vm->SetValue(flags_table, "server_can_exec", vscript::variant{gsdk::FCVAR_SERVER_CAN_EXECUTE})) {
				error("vmod: failed to set cvar server_can_exec flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "server_cant_query", vscript::variant{gsdk::FCVAR_SERVER_CANNOT_QUERY})) {
				error("vmod: failed to set cvar server_cant_query flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "client_can_exec", vscript::variant{gsdk::FCVAR_CLIENTCMD_CAN_EXECUTE})) {
				error("vmod: failed to set cvar client_can_exec flag value\n"sv);
				return false;
			}

		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			if(!vm->SetValue(flags_table, "exec_in_default", vscript::variant{gsdk::FCVAR_EXEC_DESPITE_DEFAULT})) {
				error("vmod: failed to set cvar exec_in_default flag value\n"sv);
				return false;
			}
		#endif
		}

		if(!vm->SetValue(scope, "flags", flags_table)) {
			error("vmod: failed to set cvar flags table value\n"sv);
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(flags_table && flags_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(flags_table);
		}

		if(vm->ValueExists(scope, "flags")) {
			vm->ClearValue(scope, "flags");
		}

		singleton_base::unbindings();
	}

	gsdk::HSCRIPT singleton::script_create_cvar(std::string_view varname, std::string_view value) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(varname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", varname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		if(vmod::cvar->FindCommandBase(varname.data()) != nullptr) {
			vm->RaiseException("vmod: name already in use: '%s'", varname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		ConVar *var{new ConVar};

		convar *svar{new convar{var}};
		if(!svar->initialize()) {
			delete svar;
			return gsdk::INVALID_HSCRIPT;
		}

		var->initialize(varname, value);

		return svar->instance;
	}

	gsdk::HSCRIPT singleton::script_find_cvar(std::string &&varname) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(varname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", varname.c_str());
			return gsdk::INVALID_HSCRIPT;
		}

		auto it{convars.find(varname)};
		if(it == convars.end()) {
			gsdk::ConVar *var{vmod::cvar->FindVar(varname.data())};
			if(!var) {
				return gsdk::INVALID_HSCRIPT;
			}

			std::unique_ptr<convar_ref> svar{new convar_ref{var}};
			if(!svar->initialize()) {
				return gsdk::INVALID_HSCRIPT;
			}

			it = convars.emplace(std::move(varname), std::move(svar)).first;
		}

		return it->second->instance;
	}
}
