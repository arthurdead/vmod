#pragma once

#include "../../gsdk/engine/stringtable.hpp"

namespace vmod
{
	extern gsdk::INetworkStringTable *m_pDownloadableFileTable;
	extern gsdk::INetworkStringTable *m_pModelPrecacheTable;
	extern gsdk::INetworkStringTable *m_pGenericPrecacheTable;
	extern gsdk::INetworkStringTable *m_pSoundPrecacheTable;
	extern gsdk::INetworkStringTable *m_pDecalPrecacheTable;

	extern gsdk::INetworkStringTable *g_pStringTableParticleEffectNames;
	extern gsdk::INetworkStringTable *g_pStringTableEffectDispatch;
	extern gsdk::INetworkStringTable *g_pStringTableVguiScreen;
	extern gsdk::INetworkStringTable *g_pStringTableMaterials;
	extern gsdk::INetworkStringTable *g_pStringTableInfoPanel;
	extern gsdk::INetworkStringTable *g_pStringTableClientSideChoreoScenes;
}
