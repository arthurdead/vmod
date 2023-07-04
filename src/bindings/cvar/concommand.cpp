#include "concommand.hpp"
#include "../../main.hpp"
#include "../docs.hpp"
#include "singleton.hpp"

namespace vmod::bindings::cvar
{
	vscript::class_desc<concommand_base> concommand_base::desc{"cvar::concommand"};
	vscript::class_desc<concommand_ref> concommand_ref::desc{"cvar::concommand_ref"};
	vscript::class_desc<concommand> concommand::desc{"cvar::concommand_impl"};

	std::string concommand_base::argv_buffer[gsdk::CCommand::COMMAND_MAX_ARGC];

	concommand_base::~concommand_base() noexcept {}
	concommand_ref::~concommand_ref() noexcept {}

	bool concommand_base::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		desc.func(&concommand_base::script_exec, "script_exec"sv, "exec"sv);

		concommand::desc.base(desc);
		concommand::desc.dtor();

		concommand_ref::desc.base(desc);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register cvar concommand class\n"sv);
			return false;
		}

		if(!plugin::owned_instance::register_class(&concommand::desc)) {
			return false;
		}

		if(!vm->RegisterClass(&concommand_ref::desc)) {
			error("vmod: failed to register cvar concommand class\n"sv);
			return false;
		}

		return true;
	}

	void concommand_base::unbindings() noexcept
	{
		
	}

	void concommand_base::script_exec(const gsdk::ScriptVariant_t *args, std::size_t num_args, ...) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(num_args >= gsdk::CCommand::COMMAND_MAX_ARGC) {
			vm->RaiseException("vmod: number or arguments excedeed max %zu vs %i", num_args, gsdk::CCommand::COMMAND_MAX_ARGC);
			return;
		}

		gsdk::CCommand cmd_args{};

		cmd_args.m_nArgc = static_cast<int>(num_args);

		for(std::size_t i{0}; i < num_args; ++i) {
			argv_buffer[i] = vscript::to_value<std::string>(args[i]);
			cmd_args.m_ppArgv[i] = argv_buffer[i].c_str();
		}

		cmd->Dispatch(cmd_args);
	}

	concommand::concommand(gsdk::ConCommand *cmd_, gsdk::HSCRIPT callback_) noexcept
		: concommand_base{cmd_}, callback{callback_}
	{
	}

	concommand::~concommand() noexcept
	{
		singleton &cvar{singleton::instance()};

		auto it{cvar.concommands.find(cmd->ConCommand::GetName())};
		if(it != cvar.concommands.end()) {
			cvar.concommands.erase(it);
		}

		delete cmd;

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(callback) {
			vm->ReleaseFunction(callback);
		}
	}
}
