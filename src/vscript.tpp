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
	constexpr inline gsdk::ScriptDataType_t type_to_field<script_variant_t>() noexcept
	{ return gsdk::FIELD_VARIANT; }

	template <>
	inline void initialize_variant_value<std::string_view>(gsdk::ScriptVariant_t &var, std::string_view &&value) noexcept
	{ var.m_pszString = value.data(); }

	template <>
	inline void initialize_variant_value<gsdk::HSCRIPT>(gsdk::ScriptVariant_t &var, gsdk::HSCRIPT &&value) noexcept
	{ var.m_hScript = value; }

	template <>
	inline std::string_view variant_to_value(const gsdk::ScriptVariant_t &var) noexcept
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

	template <typename T>
	class_desc_t<T>::class_desc_t(std::string_view name) noexcept
	{
		m_pfnConstruct = nullptr;
		m_pfnDestruct = nullptr;
		pHelper = &script_instance_helper<T>::singleton();
		m_pNextDesc = nullptr;
		m_pBaseDesc = nullptr;
		m_pszClassname = demangle<T>().c_str();
		m_pszScriptName = name.empty() ? m_pszClassname : name.data();
		m_pszDescription = m_pszClassname;
	}

	template <typename R, typename ...Args>
	void func_desc_t::initialize_shared(std::string_view name, std::string_view rename)
	{
		m_desc.m_pszFunction = name.data();
		m_desc.m_pszScriptName = rename.empty() ? name.data() : rename.data();

		m_desc.m_ReturnType = type_to_field<R>();
		(m_desc.m_Parameters.emplace_back(type_to_field<Args>()), ...);
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding(gsdk::ScriptFunctionBindingStorageType_t binding_func, [[maybe_unused]] char unkarg1[sizeof(int)], void *obj, gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args)};

		if(!obj) {
			return false;
		}

		if(num_required_args > 0) {
			if(!args_var || num_args != num_required_args) {
				return false;
			}
		} else {
			if(args_var || num_args != 0) {
				return false;
			}
		}

		if constexpr(std::is_void_v<R>) {
			if(ret_var) {
				return false;
			}

			call<R, C, Args...>(binding_func, obj, args_var);
		} else {
			if(!ret_var) {
				call<R, C, Args...>(binding_func, obj, args_var);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call<R, C, Args...>(binding_func, obj, args_var);
				} else {
					R ret_val{call<R, C, Args...>(binding_func, obj, args_var)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
			}
		}

		return true;
	}

	template <typename R, typename C, typename ...Args>
	void func_desc_t::initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view rename)
	{
		initialize_shared<R, Args...>(name, rename);
		m_desc.m_pszDescription = demangle<R(C::*)(Args...)>().c_str();
		m_pFunction = reinterpret_cast<void *>(mfp_to_func<R, C, Args...>(func));
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding<R, C, Args...>);
		m_flags = gsdk::SF_MEMBER_FUNC;
	}
}
