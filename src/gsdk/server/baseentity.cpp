#define __VMOD_COMPILING_GSDK
#include "baseentity.hpp"

namespace gsdk
{
	ScriptClassDesc_t *CBaseEntity::g_pScriptDesc{nullptr};
	HSCRIPT (CBaseEntity::*CBaseEntity::GetScriptInstance_ptr)() {nullptr};
	std::size_t CBaseEntity::UpdateOnRemove_vindex{static_cast<std::size_t>(-1)};
	std::size_t CBaseEntity::GetDataDescMap_vindex{vmod::vfunc_index(&gsdk::CBaseEntity::GetDataDescMap)};
	std::size_t CBaseEntity::GetServerClass_vindex{vmod::vfunc_index(&gsdk::CBaseEntity::GetServerClass)};
	std::size_t IServerNetworkable::vtable_size{static_cast<std::size_t>(-1)};
	std::size_t IServerNetworkable::GetServerClass_vindex{vmod::vfunc_index(&gsdk::IServerNetworkable::GetServerClass)};

	void IEntityFactory::Destroy(IServerNetworkable *net)
	{
		if(net) {
			net->Release();
		}
	}

	HSCRIPT CBaseEntity::GetScriptInstance() noexcept
	{
		if(!GetScriptInstance_ptr) {
			return INVALID_HSCRIPT;
		}

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

		if(!g_pScriptDesc) {
			return nullptr;
		}

		return g_pScriptVM->GetInstanceValue<CBaseEntity>(instance, g_pScriptDesc);
	}
}
