namespace vmod
{
	template <typename T>
	class_desc_t<T>::class_desc_t(std::string_view name) noexcept
	{
		m_pfnConstruct = nullptr;
		m_pfnDestruct = nullptr;
		pHelper = &instance_helper::singleton();
		m_pNextDesc = nullptr;
		m_pBaseDesc = nullptr;
		m_pszClassname = demangle<T>().c_str();
		m_pszScriptName = name.data();
		m_pszDescription = m_pszClassname;
	}

	template <typename R, typename ...Args>
	void func_desc_t::initialize_shared(std::string_view name, std::string_view rename)
	{
		m_desc.m_pszFunction = name.data();
		m_desc.m_pszScriptName = rename.empty() ? name.data() : rename.data();

		m_desc.m_ReturnType = type_to_field<std::decay_t<R>>();
		(m_desc.m_Parameters.emplace_back(type_to_field<std::decay_t<Args>>()), ...);
	}

	template <typename R, typename C, typename ...Args, std::size_t ...I>
	R func_desc_t::call_impl(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *ctx, gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept
	{
		if constexpr(std::is_void_v<C>) {
			if constexpr(sizeof...(Args) == 0) {
				return (reinterpret_cast<R(*)(Args...)>(binding_func))();
			} else {
				return (reinterpret_cast<R(*)(Args...)>(binding_func))(variant_to_value<std::decay_t<Args>>(args_var[I])...);
			}
		} else {
			if constexpr(sizeof...(Args) == 0) {
				return (static_cast<C *>(ctx)->*mfp_from_func<R, C, Args...>(reinterpret_cast<generic_func_t>(binding_func)))();
			} else {
				return (static_cast<C *>(ctx)->*mfp_from_func<R, C, Args...>(reinterpret_cast<generic_func_t>(binding_func)))(variant_to_value<std::decay_t<Args>>(args_var[I])...);
			}
		}
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding(gsdk::ScriptFunctionBindingStorageType_t binding_func, [[maybe_unused]] char unkarg1[sizeof(int)], void *obj, gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args)};

		if constexpr(!std::is_void_v<C>) {
			if(!obj) {
				return false;
			}
		}

		if(num_required_args > 0) {
			if(!args_var || num_args != num_required_args) {
				return false;
			}
		} else {
			if(/*args_var ||*/ num_args != 0) {
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

	template <typename R, typename ...Args>
	void func_desc_t::initialize_static(R(*func)(Args...), std::string_view name, std::string_view rename)
	{
		initialize_shared<R, Args...>(name, rename);
		m_desc.m_pszDescription = demangle<R(*)(Args...)>().c_str();
		m_pFunction = reinterpret_cast<void *>(func);
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding<R, void, Args...>);
		m_flags = 0;
	}
}
