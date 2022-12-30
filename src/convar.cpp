#include "convar.hpp"
#include "main.hpp"

namespace vmod
{
	void ConCommand::Dispatch(const gsdk::CCommand &args)
	{
		if(func) {
			func(args);
		}
	}

	ConCommand::~ConCommand() noexcept
	{ unregister(); }

	gsdk::CVarDLLIdentifier_t ConCommand::GetDLLIdentifier() const
	{ return main::instance().cvar_dll_id(); }

	void ConCommand::Init()
	{ cvar->RegisterConCommand(this); }

	ConVar::~ConVar() noexcept
	{ unregister(); }

	void ConVar::Init()
	{ cvar->RegisterConCommand(this); }

	gsdk::CVarDLLIdentifier_t ConVar::GetDLLIdentifier() const
	{ return main::instance().cvar_dll_id(); }
}
