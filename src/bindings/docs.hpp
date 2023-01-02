#pragma once

#include "../vscript/vscript.hpp"
#include <cstddef>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

namespace vmod::bindings::docs
{
	inline void ident(std::string &file, std::size_t num) noexcept
	{ file.insert(file.end(), num, '\t'); }

	extern void gen_date(std::string &file) noexcept;

	extern std::string_view type_name(gsdk::ScriptDataType_t type) noexcept;
	extern std::string_view type_name(gsdk::ScriptDataType_t type, std::size_t size) noexcept;

	extern std::string_view get_class_desc_name(const gsdk::ScriptClassDesc_t *desc) noexcept;

	struct value final
	{
		enum class type : unsigned char
		{
			variant,
			desc
		};

		type type;

		inline value(gsdk::ScriptVariant_t &&var_) noexcept
			: type{type::variant}, var{std::move(var_)}
		{
		}

		inline value(const gsdk::ScriptVariant_t &var_) noexcept
			: type{type::variant}, var{var_}
		{
		}

		inline value(const gsdk::ScriptClassDesc_t *desc_) noexcept
			: type{type::desc}, desc{std::move(desc_)}
		{
		}

		inline ~value() noexcept
		{
			if(type == type::variant) {
				var.~CVariantBase();
			}
		}

		inline value(value &&other) noexcept
		{ operator=(std::move(other)); }

		value &operator=(value &&other) noexcept;

		union {
			gsdk::ScriptVariant_t var;
			const gsdk::ScriptClassDesc_t *desc;
		};

	private:
		value() = delete;
		value(const value &) = delete;
		value &operator=(const value &) = delete;
	};

	extern bool write(const gsdk::ScriptFunctionBinding_t *func, bool global, std::size_t ident, std::string &file, bool respect_hide) noexcept;
	extern bool write(const gsdk::ScriptClassDesc_t *desc, bool global, std::size_t ident, std::string &file, bool respect_hide) noexcept;
	extern void write(const std::filesystem::path &dir, const std::vector<const gsdk::ScriptClassDesc_t *> &vec, bool respect_hide) noexcept;
	extern void write(const std::filesystem::path &dir, const std::vector<const gsdk::ScriptFunctionBinding_t *> &vec, bool respect_hide) noexcept;
	extern void write(const std::filesystem::path &dir, const std::unordered_map<std::string, value> &map) noexcept;

	enum class write_enum_how : unsigned char
	{
		flags,
		name,
		normal
	};
	extern void write(std::string &file, std::size_t depth, gsdk::HSCRIPT enum_table, write_enum_how how) noexcept;
}
