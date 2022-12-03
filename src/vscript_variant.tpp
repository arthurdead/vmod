#include <filesystem>
#include "vmod.hpp"

namespace vmod
{
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<void>() noexcept
	{ return gsdk::FIELD_VOID; }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<bool>() noexcept
	{ return gsdk::FIELD_BOOLEAN; }
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
				return std::atoi(var.m_pszString) > 0;
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

	template <typename T>
	inline T variant_to_int_value(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT: {
				return static_cast<T>(var.m_float);
			}
			case gsdk::FIELD_CSTRING: {
				if constexpr(std::is_same_v<std::make_signed_t<T>, long long>) {
					return static_cast<T>(std::atoll(var.m_pszString));
				} else if constexpr(std::is_same_v<std::make_signed_t<T>, long>) {
					return static_cast<T>(std::atol(var.m_pszString));
				} else {
					return static_cast<T>(std::atoi(var.m_pszString));
				}
			}
			case gsdk::FIELD_VECTOR: {
				return {};
			}
			case gsdk::FIELD_INTEGER: {
				return static_cast<T>(var.m_int);
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool ? T{1} : T{0};
			}
			case gsdk::FIELD_HSCRIPT: {
				//return vmod.to_integer(var.m_hScript);
				return {};
			}
		}

		return {};
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned int>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	inline void initialize_variant_value<unsigned int>(gsdk::ScriptVariant_t &var, unsigned int value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline unsigned int variant_to_value<unsigned int>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned int>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<int>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	inline void initialize_variant_value<int>(gsdk::ScriptVariant_t &var, int value) noexcept
	{ var.m_int = value; }
	template <>
	inline int variant_to_value<int>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<int>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned short>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	inline void initialize_variant_value<unsigned short>(gsdk::ScriptVariant_t &var, unsigned short value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline unsigned short variant_to_value<unsigned short>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned short>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<short>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	inline void initialize_variant_value<short>(gsdk::ScriptVariant_t &var, short value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline short variant_to_value<short>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<short>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	inline void initialize_variant_value<unsigned long>(gsdk::ScriptVariant_t &var, unsigned long value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline unsigned long variant_to_value<unsigned long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	inline void initialize_variant_value<long>(gsdk::ScriptVariant_t &var, long value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline long variant_to_value<long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned long long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	inline void initialize_variant_value<unsigned long long>(gsdk::ScriptVariant_t &var, unsigned long long value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline unsigned long long variant_to_value<unsigned long long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned long long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	template <>
	inline void initialize_variant_value<long long>(gsdk::ScriptVariant_t &var, long long value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline long long variant_to_value<long long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<long long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<float>() noexcept
	{ return gsdk::FIELD_FLOAT; }
	template <>
	inline void initialize_variant_value<float>(gsdk::ScriptVariant_t &var, float value) noexcept
	{ var.m_float = value; }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<double>() noexcept
	{ return gsdk::FIELD_FLOAT; }
	template <>
	inline void initialize_variant_value<double>(gsdk::ScriptVariant_t &var, double value) noexcept
	{ var.m_float = static_cast<float>(value); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long double>() noexcept
	{ return gsdk::FIELD_FLOAT; }
	template <>
	inline void initialize_variant_value<long double>(gsdk::ScriptVariant_t &var, long double value) noexcept
	{ var.m_float = static_cast<float>(value); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<gsdk::HSCRIPT>() noexcept
	{ return gsdk::FIELD_HSCRIPT; }
	template <>
	inline void initialize_variant_value<gsdk::HSCRIPT>(gsdk::ScriptVariant_t &var, gsdk::HSCRIPT value) noexcept
	{ var.m_hScript = value; }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::string_view>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	template <>
	inline void initialize_variant_value<std::string_view>(gsdk::ScriptVariant_t &var, std::string_view value) noexcept
	{ var.m_pszString = value.data(); }
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
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::filesystem::path>() noexcept
	{ return gsdk::FIELD_CSTRING; }
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
