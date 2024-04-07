#include "../../main.hpp"
#include "../../plugin.hpp"
#include "../../mod.hpp"
#include "../../filesystem.hpp"
#include "../docs.hpp"

#include "../cvar/bindings.hpp"
#include "../mem/bindings.hpp"
#include "../fs/bindings.hpp"
#include "../strtables/bindings.hpp"
#include "../syms/bindings.hpp"
#include "../ffi/bindings.hpp"
#include "../ent/bindings.hpp"

namespace vmod
{
	vscript::singleton_class_desc<main> main::desc{"vmod"};

	bool main::binding_mods() noexcept
	{
		using namespace std::literals::string_literals;

		unsigned char tmp{0};

		auto it{sv_script_class_descs.find("CBaseEntity"s)};
		for(auto &func : it->second->m_FunctionBindings) {
			if(std::strcmp(func.m_desc.m_pszScriptName, "GetEntityHandle") == 0) {
				func.m_desc.m_ReturnType = gsdk::FIELD_EHANDLE;
			#if GSDK_ENGINE == GSDK_ENGINE_TF2
				tmp |= (1 << 0);
				if(tmp == 31) {
					break;
				}
			#else
				break;
			#endif
			}
		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			else if(std::strcmp(func.m_desc.m_pszScriptName, "SetOwner") == 0 && (!func.m_desc.m_pszDescription || func.m_desc.m_pszDescription[0] == '\0')) {
				func.m_desc.m_pszDescription = "!!!DUPLICATE!!! DO NOT USE ME.";
				func.m_desc.m_pszScriptName = "__SetOwner";
				tmp |= (1 << 1);
				if(tmp == 31) {
					break;
				}
			} else if(std::strcmp(func.m_desc.m_pszScriptName, "SetGravity") == 0 && (!func.m_desc.m_pszDescription || func.m_desc.m_pszDescription[0] == '\0')) {
				func.m_desc.m_pszDescription = "!!!DUPLICATE!!! DO NOT USE ME.";
				func.m_desc.m_pszScriptName = "__SetGravity";
				tmp |= (1 << 2);
				if(tmp == 31) {
					break;
				}
			} else if(std::strcmp(func.m_desc.m_pszScriptName, "GetFriction") == 0 && (!func.m_desc.m_pszDescription || func.m_desc.m_pszDescription[0] == '\0')) {
				func.m_desc.m_pszDescription = "!!!DUPLICATE!!! DO NOT USE ME.";
				func.m_desc.m_pszScriptName = "__GetFriction";
				tmp |= (1 << 3);
				if(tmp == 31) {
					break;
				}
			} else if(std::strcmp(func.m_desc.m_pszScriptName, "SetFriction") == 0 && (!func.m_desc.m_pszDescription || func.m_desc.m_pszDescription[0] == '\0')) {
				func.m_desc.m_pszDescription = "!!!DUPLICATE!!! DO NOT USE ME.";
				func.m_desc.m_pszScriptName = "__SetFriction";
				tmp |= (1 << 4);
				if(tmp == 31) {
					break;
				}
			}
		#endif
		}

	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		it = sv_script_class_descs.find("TerrorNavArea"s);
		for(auto &func : it->second->m_FunctionBindings) {
			if(std::strcmp(func.m_desc.m_pszScriptName, "GetCorner") == 0) {
				func.m_desc.m_ReturnType = gsdk::FIELD_VECTOR;
				break;
			}
		}

		it = sv_script_class_descs.find("CTerrorPlayer"s);
		for(auto &func : it->second->m_FunctionBindings) {
			if(std::strcmp(func.m_desc.m_pszScriptName, "GetZombieType") == 0) {
				func.m_desc.m_ReturnType = gsdk::FIELD_INTEGER;
				break;
			}
		}
	#elif GSDK_ENGINE == GSDK_ENGINE_TF2
		//TODO!!!! fix MaxClients

		tmp = 0;

		it = sv_script_class_descs.find("CBasePlayer"s);
		for(auto &func : it->second->m_FunctionBindings) {
			if(std::strcmp(func.m_desc.m_pszScriptName, "GetPlayerMins") == 0) {
				func.m_desc.m_ReturnType = gsdk::FIELD_VECTOR;
				tmp |= (1 << 0);
				if(tmp == 3) {
					break;
				}
			} else if(std::strcmp(func.m_desc.m_pszScriptName, "GetPlayerMaxs") == 0) {
				func.m_desc.m_ReturnType = gsdk::FIELD_VECTOR;
				tmp |= (1 << 1);
				if(tmp == 3) {
					break;
				}
			}
		}
	#endif

		return true;
	}

	bool main::binding_mods_late() noexcept
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		vscript::variant const_table_var;
		if(vm_->GetValue(nullptr, "Constants", &const_table_var)) {
			vscript::handle_ref const_table_ref{const_table_var};
			vscript::variant taunt_table_var;
			if(vm_->GetValue(*const_table_ref, "FTaunts", &taunt_table_var)) {
				vscript::handle_wrapper taunt_table_hndl{std::move(taunt_table_var)};

				auto new_taunt_table{vm_->CreateTable()};
				if(new_taunt_table.object && new_taunt_table.object != gsdk::INVALID_HSCRIPT) {
					int num2{vm_->GetNumTableEntries(*taunt_table_hndl)};
					for(int j{0}, it2{0}; it2 != -1 && j < num2; ++j) {
						vscript::variant key2;
						vscript::variant value2;
						it2 = vm_->GetKeyValue(*taunt_table_hndl, it2, &key2, &value2);

						std::string_view value_name{key2.get<std::string_view>()};

						vm_->SetValue(new_taunt_table.object, value_name.data(), value2);
					}

					if(vm_->SetValue(*const_table_ref, "ETaunts", new_taunt_table.object)) {
						new_taunt_table.should_free_ = false;
						vm_->ClearValue(*const_table_ref, "FTaunts");
						taunt_table_hndl.free();
						//vm_->SetValue(*const_table_ref, "FTaunts", new_taunt_table.object);
					}
				}
			}
		}
	#endif

		return true;
	}

	bool main::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&main::script_find_mod, "script_find_mod"sv, "find_mod"sv)
		.desc("[mod](path)"sv);

		desc.func(&main::script_is_map_active, "script_is_map_active"sv, "is_map_active"sv);
		desc.func(&main::script_is_map_loaded, "script_is_map_loaded"sv, "is_map_loaded"sv);
		desc.func(&main::script_are_stringtables_created, "script_are_stringtables_created"sv, "are_stringtables_created"sv);

		desc.func(&main::script_success, "script_success"sv, "success"sv);
		desc.func(&main::script_print, "script_print"sv, "print"sv);
		desc.func(&main::script_info, "script_info"sv, "info"sv);
		desc.func(&main::script_remark, "script_remark"sv, "remark"sv);
		desc.func(&main::script_error, "script_error"sv, "error"sv);
		desc.func(&main::script_warning, "script_warning"sv, "warning"sv);

		desc.func(&main::script_successl, "script_successl"sv, "successl"sv);
		desc.func(&main::script_printl, "script_printl"sv, "printl"sv);
		desc.func(&main::script_infol, "script_infol"sv, "infol"sv);
		desc.func(&main::script_remarkl, "script_remarkl"sv, "remarkl"sv);
		desc.func(&main::script_errorl, "script_errorl"sv, "errorl"sv);
		desc.func(&main::script_warningl, "script_warningl"sv, "warningl"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		if(!vm_->SetValue(*scope, "root_dir", root_dir_.c_str())) {
			error("vmod: failed to set root_dir value\n"sv);
			return false;
		}

		return_flags_table = vm_->CreateTable();
		if(!return_flags_table) {
			error("vmod: failed to create return flags table\n"sv);
			return false;
		}

		{
			if(!vm_->SetValue(*return_flags_table, "ignored", vscript::variant{plugin::callable::return_flags::ignored})) {
				error("vmod: failed to set return flags continue value\n"sv);
				return false;
			}

			if(!vm_->SetValue(*return_flags_table, "halt", vscript::variant{plugin::callable::return_flags::halt})) {
				error("vmod: failed to set return flags halt value\n"sv);
				return false;
			}

			if(!vm_->SetValue(*return_flags_table, "handled", vscript::variant{plugin::callable::return_flags::handled})) {
				error("vmod: failed to set return flags handled value\n"sv);
				return false;
			}

			if(!vm_->SetValue(*return_flags_table, "handled_halt", vscript::variant{plugin::callable::return_flags::handled|plugin::callable::return_flags::halt})) {
				error("vmod: failed to set return flags handled_halt value\n"sv);
				return false;
			}
		}

		if(!vm_->SetValue(*scope, "ret", *return_flags_table)) {
			error("vmod: failed to set return flags table value\n"sv);
			return false;
		}

		if(!plugin::bindings()) {
			return false;
		}

		if(!mod::bindings()) {
			return false;
		}

		if(!bindings::cvar::bindings()) {
			return false;
		}

		if(!bindings::mem::bindings()) {
			return false;
		}

		if(!bindings::ffi::bindings()) {
			return false;
		}

		if(!bindings::fs::bindings()) {
			return false;
		}

		if(!bindings::ent::bindings()) {
			return false;
		}

		if(!bindings::strtables::bindings()) {
			return false;
		}

		stringtable_table = vm_->CreateTable();
		if(!stringtable_table) {
			error("vmod: failed to create strtables table\n"sv);
			return false;
		}

		if(!create_script_stringtables()) {
			return false;
		}

		if(!vm_->SetValue(*scope, "strtables", *stringtable_table)) {
			error("vmod: failed to set strtables table value\n"sv);
			return false;
		}

	#ifndef GSDK_NO_SYMBOLS
		if(symbols_available) {
			if(!bindings::syms::bindings()) {
				return false;
			}

			symbols_table_ = vm_->CreateTable();
			if(!symbols_table_) {
				error("vmod: failed to create syms table\n"sv);
				return false;
			}

			if(!create_script_symbols()) {
				return false;
			}

			if(!vm_->SetValue(*scope, "syms", *symbols_table_)) {
				error("vmod: failed to set syms table value\n"sv);
				return false;
			}
		}
	#endif

		if(!bindings::cvar::create_get()) {
			return false;
		}

		if(!bindings::mem::create_get()) {
			return false;
		}

		if(!bindings::ffi::create_get()) {
			return false;
		}

		if(!bindings::fs::create_get()) {
			return false;
		}

		if(!bindings::ent::create_get()) {
			return false;
		}

		if(!create_get()) {
			return false;
		}

		return true;
	}

	void main::write_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		bindings::docs::gen_date(file);

		file += "namespace vmod\n{\n"sv;
		bindings::docs::write(&desc, false, 1, file, false);
		file += '\n';

		bindings::docs::ident(file, 1);
		file += "fs::path root_dir;\n\n"sv;

		bindings::docs::write(&plugin::owned_instance::desc, true, 1, file, false);
		file += "\n\n"sv;

		bindings::docs::ident(file, 1);
		file += "enum class callback_return_flags\n"sv;
		bindings::docs::ident(file, 1);
		file += "{\n"sv;
		bindings::docs::ident(file, 2);
		file += "ignored,\n"sv;
		bindings::docs::ident(file, 2);
		file += "error,\n"sv;
		bindings::docs::ident(file, 2);
		file += "halt,\n"sv;
		bindings::docs::ident(file, 2);
		file += "handled\n"sv;
		bindings::docs::ident(file, 1);
		file += "};\n\n"sv;

		bindings::docs::write(&plugin::callback_instance::desc, true, 1, file, false);
		file += "\n\n"sv;

		bindings::docs::write(&plugin::desc, true, 1, file, false);
		file += "\n\n"sv;

		bindings::docs::write(&mod::desc, true, 1, file, false);
		file += "\n\n"sv;

		bindings::docs::ident(file, 1);
		file += "namespace fs;\n\n"sv;
		bindings::fs::write_docs(dir);

		bindings::docs::ident(file, 1);
		file += "namespace cvar;\n\n"sv;
		bindings::cvar::write_docs(dir);

		bindings::docs::ident(file, 1);
		file += "namespace mem;\n\n"sv;
		bindings::mem::write_docs(dir);

		bindings::docs::ident(file, 1);
		file += "namespace strtables;\n\n"sv;
		bindings::strtables::write_docs(dir);

	#ifndef GSDK_NO_SYMBOLS
		if(symbols_available) {
			bindings::docs::ident(file, 1);
			file += "namespace syms;\n\n"sv;
			bindings::syms::write_docs(dir);
		}
	#endif

		bindings::docs::ident(file, 1);
		file += "namespace ffi;\n\n"sv;
		bindings::ffi::write_docs(dir);

		bindings::docs::ident(file, 1);
		file += "namespace ent;\n"sv;
		bindings::ent::write_docs(dir);

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "vmod"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void main::unbindings() noexcept
	{
		script_stringtables.clear();

		stringtable_table.free();

		if(scope) {
			if(vm_->ValueExists(*scope, "strtables")) {
				vm_->ClearValue(*scope, "strtables");
			}
		}

		bindings::strtables::unbindings();

	#ifndef GSDK_NO_SYMBOLS
		if(symbols_available) {
			symbols_table_.free();

			if(scope) {
				if(vm_->ValueExists(*scope, "syms")) {
					vm_->ClearValue(*scope, "syms");
				}
			}
		}
	#endif

		bindings::ent::unbindings();

		bindings::ffi::unbindings();

	#ifndef GSDK_NO_SYMBOLS
		if(symbols_available) {
			bindings::syms::unbindings();
		}
	#endif

		bindings::fs::unbindings();

		bindings::mem::unbindings();

		bindings::cvar::unbindings();

		plugin::unbindings();

		mod::unbindings();

		return_flags_table.free();

		if(scope) {
			if(vm_->ValueExists(*scope, "ret")) {
				vm_->ClearValue(*scope, "ret");
			}

			if(vm_->ValueExists(*scope, "root_dir")) {
				vm_->ClearValue(*scope, "root_dir");
			}
		}

		singleton_base::unbindings();
	}

	vscript::handle_ref main::script_find_mod(std::string_view mdname) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(mdname.empty()) {
			vm_->RaiseException("vmod: invalid path: '%s'", mdname.data());
			return nullptr;
		}

		std::filesystem::path path{build_mod_path(mdname)};

		auto it{mods.find(path)};
		if(it != mods.end()) {
			return it->second->instance();
		}

		return nullptr;
	}

	void main::script_print(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		print("%s"sv, str.data());
	}
	void main::script_success(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		success("%s"sv, str.data());
	}
	void main::script_error(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		error("%s"sv, str.data());
	}
	void main::script_warning(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		warning("%s"sv, str.data());
	}
	void main::script_info(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		info("%s"sv, str.data());
	}
	void main::script_remark(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		remark("%s"sv, str.data());
	}

	void main::script_printl(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		print("%s\n"sv, str.data());
	}
	void main::script_successl(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		success("%s\n"sv, str.data());
	}
	void main::script_errorl(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		error("%s\n"sv, str.data());
	}
	void main::script_warningl(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		warning("%s\n"sv, str.data());
	}
	void main::script_infol(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		info("%s\n"sv, str.data());
	}
	void main::script_remarkl(const vscript::variant &var) noexcept
	{
		using namespace std::literals::string_view_literals;
		std::string_view str{var.get<std::string_view>()};
		remark("%s\n"sv, str.data());
	}

	bool main::script_is_map_loaded() const noexcept
	{ return is_map_loaded; }
	bool main::script_is_map_active() const noexcept
	{ return is_map_active; }
	bool main::script_are_stringtables_created() const noexcept
	{ return are_string_tables_created; }
}
