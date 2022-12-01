#include "convar.hpp"
#include "vmod.hpp"

namespace vmod
{
	ConCommand::ConCommand(std::string_view name, int flags) noexcept
	{
		m_pszName = name.data();
		m_pszHelpString = "";
		m_nFlags = flags;
	}

	void ConCommand::unregister() noexcept
	{
		if(m_bRegistered) {
			cvar->UnregisterConCommand(static_cast<gsdk::ConCommandBase *>(this));
		}
	}

	void ConCommand::Dispatch(const gsdk::CCommand &args)
	{ func(args); }

	gsdk::CVarDLLIdentifier_t ConCommand::GetDLLIdentifier() const
	{ return cvar_dll_id; }

	void ConCommand::Init()
	{ cvar->RegisterConCommand(static_cast<gsdk::ConCommandBase *>(this)); }
}
