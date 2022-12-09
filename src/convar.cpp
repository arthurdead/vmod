#include "convar.hpp"
#include "vmod.hpp"

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
	{ return vmod.cvar_dll_id(); }

	void ConCommand::Init()
	{ cvar->RegisterConCommand(static_cast<gsdk::ConCommandBase *>(this)); }

	ConVar::~ConVar() noexcept
	{ unregister(); }

	void ConVar::Init()
	{ cvar->RegisterConCommand(static_cast<gsdk::ConCommandBase *>(this)); }

	gsdk::CVarDLLIdentifier_t ConVar::GetDLLIdentifier() const
	{ return vmod.cvar_dll_id(); }
}
