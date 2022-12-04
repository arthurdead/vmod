#include "convar.hpp"
#include "vmod.hpp"

namespace vmod
{
	void ConCommand::Dispatch(const gsdk::CCommand &args)
	{ func(args); }

	gsdk::CVarDLLIdentifier_t ConCommand::GetDLLIdentifier() const
	{ return vmod.cvar_dll_id(); }

	void ConCommand::Init()
	{ cvar->RegisterConCommand(static_cast<gsdk::ConCommandBase *>(this)); }
}
