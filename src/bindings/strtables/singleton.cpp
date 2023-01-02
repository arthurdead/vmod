#include "singleton.hpp"
#include "../../main.hpp"
#include "string_table.hpp"
#include <string>

namespace vmod
{
	void main::clear_script_stringtables() noexcept
	{
		for(const auto &it : script_stringtables) {
			it.second->table = nullptr;
		}
	}

	void main::recreate_script_stringtables() noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		static auto set_table_value{
			[this](std::string &&tablename, gsdk::INetworkStringTable *value) noexcept -> void {
				auto it{script_stringtables.find(tablename)};
				if(it == script_stringtables.end()) {
					std::unique_ptr<bindings::strtables::string_table> ptr{new bindings::strtables::string_table{value}};
					if(!ptr->initialize()) {
						error("vmod: failed to register '%s' string table instance\n"sv, tablename.c_str());
						return;
					}

					if(!vm_->SetValue(stringtable_table, tablename.c_str(), ptr->instance)) {
						error("vmod: failed to set '%s' string table value\n"sv);
						return;
					}

					it = script_stringtables.emplace(std::move(tablename), std::move(ptr)).first;
				}

				it->second->table = value;
			}
		};

		set_table_value(gsdk::DOWNLOADABLE_FILE_TABLENAME, m_pDownloadableFileTable);
		set_table_value(gsdk::MODEL_PRECACHE_TABLENAME, m_pModelPrecacheTable);
		set_table_value(gsdk::GENERIC_PRECACHE_TABLENAME, m_pGenericPrecacheTable);
		set_table_value(gsdk::SOUND_PRECACHE_TABLENAME, m_pSoundPrecacheTable);
		set_table_value(gsdk::DECAL_PRECACHE_TABLENAME, m_pDecalPrecacheTable);

		set_table_value("ParticleEffectNames"s, g_pStringTableParticleEffectNames);
		set_table_value("EffectDispatch"s, g_pStringTableEffectDispatch);
		set_table_value("VguiScreen"s, g_pStringTableVguiScreen);
		set_table_value("Materials"s, g_pStringTableMaterials);
		set_table_value("InfoPanel"s, g_pStringTableInfoPanel);
		set_table_value("Scenes"s, g_pStringTableClientSideChoreoScenes);
	}

	bool main::create_script_stringtable(std::string &&tablename) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::unique_ptr<bindings::strtables::string_table> ptr{new bindings::strtables::string_table};

		if(!ptr->initialize()) {
			error("vmod: failed to register '%s' string table instance\n"sv, tablename.c_str());
			return false;
		}

		if(!vm_->SetValue(stringtable_table, tablename.c_str(), ptr->instance)) {
			error("vmod: failed to set '%s' string table value\n"sv, tablename.c_str());
			return false;
		}

		script_stringtables.emplace(std::move(tablename), std::move(ptr));

		return true;
	}

	bool main::create_script_stringtables() noexcept
	{
		using namespace std::literals::string_literals;

		if(!create_script_stringtable(gsdk::DOWNLOADABLE_FILE_TABLENAME)) {
			return false;
		}

		if(!create_script_stringtable(gsdk::MODEL_PRECACHE_TABLENAME)) {
			return false;
		}

		if(!create_script_stringtable(gsdk::GENERIC_PRECACHE_TABLENAME)) {
			return false;
		}

		if(!create_script_stringtable(gsdk::SOUND_PRECACHE_TABLENAME)) {
			return false;
		}

		if(!create_script_stringtable(gsdk::DECAL_PRECACHE_TABLENAME)) {
			return false;
		}

		if(!create_script_stringtable("ParticleEffectNames"s)) {
			return false;
		}

		if(!create_script_stringtable("EffectDispatch"s)) {
			return false;
		}

		if(!create_script_stringtable("VguiScreen"s)) {
			return false;
		}

		if(!create_script_stringtable("Materials"s)) {
			return false;
		}

		if(!create_script_stringtable("InfoPanel"s)) {
			return false;
		}

		if(!create_script_stringtable("Scenes"s)) {
			return false;
		}

		return true;
	}
}
