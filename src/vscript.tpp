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
	inline void initialize_variant_value<std::string_view>(gsdk::ScriptVariant_t &var, std::string_view value) noexcept
	{ var.m_pszString = value.data(); }

	template <typename T>
	singleton_class_desc_t<T>::singleton_class_desc_t(std::string_view classname, std::string_view name) noexcept
	{
		m_pBaseDesc = nullptr;
		m_pszClassname = classname.data();
		m_pfnConstruct = nullptr;
		m_pfnDestruct = nullptr;
		m_pszScriptName = name.empty() ? classname.data() : name.data();
		m_pszDescription = "";
		pHelper = nullptr;
		m_pNextDesc = nullptr;
	}

	template <typename R, typename ...Args>
	void func_desc_t::initialize_shared(std::string_view name, std::string_view rename)
	{
		m_desc.m_pszFunction = name.data();
		m_desc.m_pszScriptName = rename.empty() ? name.data() : rename.data();
		m_desc.m_pszDescription = "";

		m_desc.m_ReturnType = type_to_field<R>();
		(m_desc.m_Parameters.emplace_back(type_to_field<Args>()), ...);
	}

	template <typename R, typename C, typename ...Args>
	void func_desc_t::initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view rename)
	{
		initialize_shared<R, Args...>(name, rename);

		static constexpr auto binding_lambda = 
			[](gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept -> bool {
				debugbreak();
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
					call<R, C, Args...>(binding_func, obj, args_var);

					if(ret_var) {
						null_variant(*ret_var);
					}
				} else {
					if(!ret_var) {
						call<R, C, Args...>(binding_func, obj, args_var);
					} else {
						R ret_val{call<R, C, Args...>(binding_func, obj, args_var)};
						value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
					}
				}

				return true;
			};

		m_pFunction = reinterpret_cast<void *>(mfp_to_func<R, C, Args...>(func));
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_lambda);
		m_flags = gsdk::SF_MEMBER_FUNC;
	}
}
