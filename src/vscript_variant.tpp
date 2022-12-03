#include <filesystem>
#include "vmod.hpp"

namespace vmod
{
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<void>() noexcept
	{ return gsdk::FIELD_VOID; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::string_view>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<gsdk::HSCRIPT>() noexcept
	{ return gsdk::FIELD_HSCRIPT; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned int>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<int>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::filesystem::path>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<bool>() noexcept
	{ return gsdk::FIELD_BOOLEAN; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<short>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<float>() noexcept
	{ return gsdk::FIELD_FLOAT; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<double>() noexcept
	{ return gsdk::FIELD_FLOAT; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long double>() noexcept
	{ return gsdk::FIELD_FLOAT; }

	template <>
	inline void initialize_variant_value<std::string_view>(gsdk::ScriptVariant_t &var, std::string_view value) noexcept
	{ var.m_pszString = value.data(); }

	template <>
	inline void initialize_variant_value<gsdk::HSCRIPT>(gsdk::ScriptVariant_t &var, gsdk::HSCRIPT value) noexcept
	{ var.m_hScript = value; }

	template <>
	inline void initialize_variant_value<unsigned int>(gsdk::ScriptVariant_t &var, unsigned int value) noexcept
	{ var.m_int = static_cast<int>(value); }

	template <>
	inline void initialize_variant_value<unsigned long>(gsdk::ScriptVariant_t &var, unsigned long value) noexcept
	{ var.m_int = static_cast<int>(value); }

	template <>
	inline void initialize_variant_value<int>(gsdk::ScriptVariant_t &var, int value) noexcept
	{ var.m_int = value; }

	template <>
	inline void initialize_variant_value<short>(gsdk::ScriptVariant_t &var, short value) noexcept
	{ var.m_int = static_cast<int>(value); }

	template <>
	inline void initialize_variant_value<long long>(gsdk::ScriptVariant_t &var, long long value) noexcept
	{ var.m_int = static_cast<int>(value); }

	template <>
	inline void initialize_variant_value<float>(gsdk::ScriptVariant_t &var, float value) noexcept
	{ var.m_float = value; }

	template <>
	inline void initialize_variant_value<double>(gsdk::ScriptVariant_t &var, double value) noexcept
	{ var.m_float = static_cast<float>(value); }

	template <>
	inline void initialize_variant_value<long double>(gsdk::ScriptVariant_t &var, long double value) noexcept
	{ var.m_float = static_cast<float>(value); }

	template <>
	inline void initialize_variant_value<bool>(gsdk::ScriptVariant_t &var, bool value) noexcept
	{ var.m_bool = value; }

	template <>
	inline bool variant_to_value<bool>(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT: {
				return var.m_float > 0.0f;
			}
			case gsdk::FIELD_CSTRING: {
				return std::atoll(var.m_pszString) > 0;
			}
			case gsdk::FIELD_VECTOR: {
				return {};
			}
			case gsdk::FIELD_INTEGER: {
				return var.m_int > 0;
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool;
			}
			case gsdk::FIELD_HSCRIPT: {
				//return vmod.to_boolean(var.m_hScript);
				return {};
			}
		}

		return {};
	}

	template <>
	inline std::size_t variant_to_value<std::size_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT: {
				return static_cast<std::size_t>(var.m_float);
			}
			case gsdk::FIELD_CSTRING: {
				return static_cast<std::size_t>(std::atoll(var.m_pszString));
			}
			case gsdk::FIELD_VECTOR: {
				return {};
			}
			case gsdk::FIELD_INTEGER: {
				return static_cast<std::size_t>(var.m_int);
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool ? 1 : 0;
			}
			case gsdk::FIELD_HSCRIPT: {
				//return vmod.to_integer(var.m_hScript);
				return {};
			}
		}

		return {};
	}

	template <>
	inline std::string_view variant_to_value<std::string_view>(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		static std::string temp_buffer;

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT: {
				temp_buffer = std::to_string(var.m_float);
				return temp_buffer;
			}
			case gsdk::FIELD_CSTRING: {
				return var.m_pszString;
			}
			case gsdk::FIELD_VECTOR: {
				temp_buffer.clear();
				temp_buffer += "(vector : ("sv;
				temp_buffer += std::to_string(var.m_pVector->x);
				temp_buffer += ", "sv;
				temp_buffer += std::to_string(var.m_pVector->y);
				temp_buffer += ", "sv;
				temp_buffer += std::to_string(var.m_pVector->z);
				temp_buffer += "))"sv;
				return temp_buffer;
			}
			case gsdk::FIELD_INTEGER: {
				temp_buffer = std::to_string(var.m_int);
				return temp_buffer;
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool ? "true"sv : "false"sv;
			}
			case gsdk::FIELD_HSCRIPT: {
				return vmod.to_string(var.m_hScript);
			}
		}

		return {};
	}

	template <>
	inline std::filesystem::path variant_to_value<std::filesystem::path>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_CSTRING: {
				return var.m_pszString;
			}
		}

		return {};
	}
}
