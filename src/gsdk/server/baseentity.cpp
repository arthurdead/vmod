#define __VMOD_COMPILING_GSDK
#include "baseentity.hpp"

namespace gsdk
{
	ScriptClassDesc_t *CBaseEntity::g_pScriptDesc{nullptr};
	HSCRIPT (CBaseEntity::*CBaseEntity::GetScriptInstance_ptr)() {nullptr};

	void IEntityFactory::Destroy(IServerNetworkable *net)
	{
		if(net) {
			net->Release();
		}
	}

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

		return g_pScriptVM->GetInstanceValue<CBaseEntity>(instance, g_pScriptDesc);
	}
}
