#include "singleton.hpp"
#include "../../gsdk.hpp"
#include "../../convar.hpp"
#include "convar.hpp"
#include "concommand.hpp"
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

		gsdk::IScriptVM *vm{vscript::vm()};

		desc.func(&singleton::script_create_cvar, "script_create_cvar"sv, "create_var"sv)
		.desc("[convar_impl](name, value)"sv);

		desc.func(&singleton::script_create_concmd, "script_create_concmd"sv, "create_cmd"sv)
		.desc("[concommand_impl](name, concommand_callback|callback)"sv);

		desc.func(&singleton::script_find_cvar, "script_find_cvar"sv, "find_var"sv)
		.desc("[convar_ref](name)"sv);

		desc.func(&singleton::script_find_concmd, "script_find_concmd"sv, "find_cmd"sv)
		.desc("[concommand_ref](name)"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		flags_table = vm->CreateTable();
		if(!flags_table) {
			error("vmod: failed to create cvar flags table\n"sv);
			return false;
		}

		{
			if(!vm->SetValue(*flags_table, "none", vscript::variant{gsdk::FCVAR_NONE})) {
				error("vmod: failed to set cvar none flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "development", vscript::variant{gsdk::FCVAR_DEVELOPMENTONLY})) {
				error("vmod: failed to set cvar development flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "hidden", vscript::variant{gsdk::FCVAR_HIDDEN})) {
				error("vmod: failed to set cvar hidden flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "protected", vscript::variant{gsdk::FCVAR_PROTECTED})) {
				error("vmod: failed to set cvar protected flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "singleplayer", vscript::variant{gsdk::FCVAR_SPONLY})) {
				error("vmod: failed to set cvar singleplayer flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "printable_only", vscript::variant{gsdk::FCVAR_PRINTABLEONLY})) {
				error("vmod: failed to set cvar printable_only flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "never_string", vscript::variant{gsdk::FCVAR_NEVER_AS_STRING})) {
				error("vmod: failed to set cvar never_string flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "server", vscript::variant{gsdk::FCVAR_GAMEDLL})) {
				error("vmod: failed to set cvar server flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "client", vscript::variant{gsdk::FCVAR_CLIENTDLL})) {
				error("vmod: failed to set cvar client flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "archive", vscript::variant{gsdk::FCVAR_ARCHIVE})) {
				error("vmod: failed to set cvar archive flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "notify", vscript::variant{gsdk::FCVAR_NOTIFY})) {
				error("vmod: failed to set cvar notify flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "userinfo", vscript::variant{gsdk::FCVAR_USERINFO})) {
				error("vmod: failed to set cvar userinfo flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "cheat", vscript::variant{gsdk::FCVAR_CHEAT})) {
				error("vmod: failed to set cvar cheat flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "replicated", vscript::variant{gsdk::FCVAR_REPLICATED})) {
				error("vmod: failed to set cvar replicated flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "only_unconnected", vscript::variant{gsdk::FCVAR_NOT_CONNECTED})) {
				error("vmod: failed to set cvar only_unconnected flag value\n"sv);
				return false;
			}

		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			if(!vm->SetValue(*flags_table, "allowed_in_competitive", vscript::variant{gsdk::FCVAR_ALLOWED_IN_COMPETITIVE})) {
				error("vmod: failed to set cvar allowed_in_competitive flag value\n"sv);
				return false;
			}
		#endif

		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			if(!vm->SetValue(*flags_table, "internal", vscript::variant{gsdk::FCVAR_INTERNAL_USE})) {
				error("vmod: failed to set cvar internal flag value\n"sv);
				return false;
			}
		#endif

			if(!vm->SetValue(*flags_table, "server_can_exec", vscript::variant{gsdk::FCVAR_SERVER_CAN_EXECUTE})) {
				error("vmod: failed to set cvar server_can_exec flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "server_cant_query", vscript::variant{gsdk::FCVAR_SERVER_CANNOT_QUERY})) {
				error("vmod: failed to set cvar server_cant_query flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(*flags_table, "client_can_exec", vscript::variant{gsdk::FCVAR_CLIENTCMD_CAN_EXECUTE})) {
				error("vmod: failed to set cvar client_can_exec flag value\n"sv);
				return false;
			}

		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			if(!vm->SetValue(*flags_table, "exec_in_default", vscript::variant{gsdk::FCVAR_EXEC_DESPITE_DEFAULT})) {
				error("vmod: failed to set cvar exec_in_default flag value\n"sv);
				return false;
			}
		#endif
		}

		if(!vm->SetValue(*scope, "flags", *flags_table)) {
			error("vmod: failed to set cvar flags table value\n"sv);
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		flags_table.free();

		if(scope) {
			if(vm->ValueExists(*scope, "flags")) {
				vm->ClearValue(*scope, "flags");
			}
		}

		singleton_base::unbindings();
	}

	vscript::handle_ref singleton::script_create_cvar(std::string_view varname, std::string_view value) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(varname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", varname.data());
			return nullptr;
		}

		if(vmod::cvar->FindCommandBase(varname.data()) != nullptr) {
			vm->RaiseException("vmod: name already in use: '%s'", varname.data());
			return nullptr;
		}

		ConVar *var{new ConVar};

		convar *svar{new convar{var}};
		if(!svar->initialize()) {
			delete svar;
			return nullptr;
		}

		var->initialize(varname, value);

		return svar->instance_;
	}

	vscript::handle_ref singleton::script_create_concmd(std::string_view varname, vscript::handle_wrapper callback) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(varname.empty()) {
			vm->RaiseException("vmod: empty name");
			return nullptr;
		}

		if(vmod::cvar->FindCommandBase(varname.data()) != nullptr) {
			vm->RaiseException("vmod: name already in use: '%s'", varname.data());
			return nullptr;
		}

		if(!callback) {
			vm->RaiseException("vmod: invalid callback");
			return nullptr;
		}

		callback = vm->ReferenceFunction(*callback);
		if(!callback) {
			vm->RaiseException("vmod: failed to get callback reference");
			return nullptr;
		}

		ConCommand *var{new ConCommand};

		concommand *svar{new concommand{var, std::move(callback)}};
		if(!svar->initialize()) {
			delete svar;
			return nullptr;
		}

		plugin *pl{plugin::assumed_currently_running()};
		vscript::handle_ref scope{pl ? pl->private_scope() : nullptr};

		vscript::handle_ref callback_ref{callback};

		var->initialize(varname, [var,vm,callback_ref,scope](const gsdk::CCommand &cmdargs) noexcept -> void {
			vscript::handle_wrapper arr{vm->CreateArray()};

			for(int i{0}; i < cmdargs.m_nArgc; ++i) {
				vscript::variant tmp{cmdargs.m_ppArgv[i]};
				vm->ArrayAddToTail(*arr, std::move(tmp));
			}

			vscript::variant scriptargs[]{
				std::move(arr)
			};
			if(vm->ExecuteFunction(*callback_ref, scriptargs, std::size(scriptargs), nullptr, *scope, true) == gsdk::SCRIPT_ERROR) {
				error("vmod: script cmd '%s' failed to execute\n", var->GetName());
			}
		});

		return svar->instance_;
	}

	vscript::handle_ref singleton::script_find_cvar(std::string &&varname) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(varname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", varname.c_str());
			return nullptr;
		}

		auto it{convars.find(varname)};
		if(it == convars.end()) {
			gsdk::ConVar *var{vmod::cvar->FindVar(varname.data())};
			if(!var) {
				return nullptr;
			}

			std::unique_ptr<convar_ref> svar{new convar_ref{var}};
			if(!svar->initialize()) {
				return nullptr;
			}

			it = convars.emplace(std::move(varname), std::move(svar)).first;
		}

		return it->second->instance_;
	}

	vscript::handle_ref singleton::script_find_concmd(std::string &&varname) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(varname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", varname.c_str());
			return nullptr;
		}

		auto it{concommands.find(varname)};
		if(it == concommands.end()) {
			gsdk::ConCommand *var{vmod::cvar->FindCommand(varname.data())};
			if(!var) {
				return nullptr;
			}

			std::unique_ptr<concommand_ref> svar{new concommand_ref{var}};
			if(!svar->initialize()) {
				return nullptr;
			}

			it = concommands.emplace(std::move(varname), std::move(svar)).first;
		}

		return it->second->instance_;
	}
}
