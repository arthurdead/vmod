#include "baseentity.hpp"

namespace gsdk
{
	IScriptVM *g_pScriptVM;

	ScriptClassDesc_t *CBaseEntity::g_pScriptDesc;
	HSCRIPT (CBaseEntity::*CBaseEntity::GetScriptInstance_ptr)();

	HSCRIPT CBaseEntity::GetScriptInstance() noexcept
	{
		HSCRIPT ret{(this->*GetScriptInstance_ptr)()};
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
		return ret;
	}

	CBaseEntity *CBaseEntity::from_instance(HSCRIPT instance) noexcept
	{
		if(!instance || instance == INVALID_HSCRIPT) {
			return nullptr;
		}

		return static_cast<CBaseEntity *>(g_pScriptVM->GetInstanceValue(instance, g_pScriptDesc));
	}
}
