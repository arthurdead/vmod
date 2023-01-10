#include "../../main.hpp"
#include "../../plugin.hpp"
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
		return true;
	}

	bool main::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&main::script_find_plugin, "script_find_plugin"sv, "find_plugin"sv)
		.desc("[plugin](path)"sv);

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

		if(!vm_->SetValue(scope, "root_dir", root_dir_.c_str())) {
			error("vmod: failed to set root_dir value\n"sv);
			return false;
		}

		return_flags_table = vm_->CreateTable();
		if(!return_flags_table || return_flags_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create return flags table\n"sv);
			return false;
		}

		{
			if(!vm_->SetValue(return_flags_table, "ignored", vscript::variant{plugin::callable::return_flags::ignored})) {
				error("vmod: failed to set return flags continue value\n"sv);
				return false;
			}

			if(!vm_->SetValue(return_flags_table, "halt", vscript::variant{plugin::callable::return_flags::halt})) {
				error("vmod: failed to set return flags halt value\n"sv);
				return false;
			}

			if(!vm_->SetValue(return_flags_table, "handled", vscript::variant{plugin::callable::return_flags::handled})) {
				error("vmod: failed to set return flags handled value\n"sv);
				return false;
			}

			if(!vm_->SetValue(return_flags_table, "handled_halt", vscript::variant{plugin::callable::return_flags::handled|plugin::callable::return_flags::halt})) {
				error("vmod: failed to set return flags handled_halt value\n"sv);
				return false;
			}
		}

		if(!vm_->SetValue(scope, "ret", return_flags_table)) {
			error("vmod: failed to set return flags table value\n"sv);
			return false;
		}

		if(!plugin::bindings()) {
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
		if(!stringtable_table || stringtable_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create strtables table\n"sv);
			return false;
		}

		if(!create_script_stringtables()) {
			return false;
		}

		if(!vm_->SetValue(scope, "strtables", stringtable_table)) {
			error("vmod: failed to set strtables table value\n"sv);
			return false;
		}

	#ifndef GSDK_NO_SYMBOLS
		if(symbols_available) {
			if(!bindings::syms::bindings()) {
				return false;
			}

			symbols_table_ = vm_->CreateTable();
			if(!symbols_table_ || symbols_table_ == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to create syms table\n"sv);
				return false;
			}

			if(!create_script_symbols()) {
				return false;
			}

			if(!vm_->SetValue(scope, "syms", symbols_table_)) {
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

		bindings::docs::write(&plugin::callback_instance::desc, true, 1, file, false);
		file += "\n\n"sv;

		bindings::docs::write(&plugin::desc, true, 1, file, false);
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

		if(stringtable_table && stringtable_table != gsdk::INVALID_HSCRIPT) {
			vm_->ReleaseTable(stringtable_table);
		}

		if(vm_->ValueExists(scope, "strtables")) {
			vm_->ClearValue(scope, "strtables");
		}

		bindings::strtables::unbindings();

	#ifndef GSDK_NO_SYMBOLS
		if(symbols_available) {
			if(symbols_table_ && symbols_table_ != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseTable(symbols_table_);
			}

			if(vm_->ValueExists(scope, "syms")) {
				vm_->ClearValue(scope, "syms");
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

		if(return_flags_table && return_flags_table != gsdk::INVALID_HSCRIPT) {
			vm_->ReleaseTable(return_flags_table);
		}

		if(vm_->ValueExists(scope, "ret")) {
			vm_->ClearValue(scope, "ret");
		}

		if(vm_->ValueExists(scope, "root_dir")) {
			vm_->ClearValue(scope, "root_dir");
		}

		singleton_base::unbindings();
	}

	gsdk::HSCRIPT main::script_find_plugin(std::string_view plname) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(plname.empty()) {
			vm_->RaiseException("vmod: invalid path: '%s'", plname.data());
			return nullptr;
		}

		std::filesystem::path path{build_plugin_path(plname)};
		{
			std::filesystem::path ext{path.extension()};
			if(ext != scripts_extension) {
				vm_->RaiseException("vmod: invalid extension expected '%s' got '%s'", scripts_extension.data(), ext.c_str());
				return nullptr;
			}
		}

		auto it{plugins.find(path)};
		if(it != plugins.end()) {
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
