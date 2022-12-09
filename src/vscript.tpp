namespace vmod
{
	template <typename T>
	class_desc_t<T>::class_desc_t(std::string_view name) noexcept
	{
		m_pfnConstruct = nullptr;
		m_pfnDestruct = nullptr;
		pHelper = &instance_helper<T>::singleton();
		m_pNextDesc = nullptr;
		m_pBaseDesc = nullptr;
		m_pszClassname = demangle<T>().c_str();
		m_pszScriptName = name.data();
		m_pszDescription = m_pszClassname;
	}

	template <typename T>
	class_desc_t<T> &class_desc_t<T>::operator=(class_desc_t &&other) noexcept
	{
		m_pszScriptName = other.m_pszScriptName;
		other.m_pszScriptName = nullptr;
		m_pszClassname = other.m_pszClassname;
		other.m_pszClassname = nullptr;
		m_pszDescription = other.m_pszDescription;
		other.m_pszDescription = nullptr;
		m_pBaseDesc = other.m_pBaseDesc;
		other.m_pBaseDesc = nullptr;
		m_FunctionBindings = std::move(other.m_FunctionBindings);
		m_pfnConstruct = other.m_pfnConstruct;
		other.m_pfnConstruct = nullptr;
		m_pfnDestruct = other.m_pfnDestruct;
		other.m_pfnDestruct = nullptr;
		pHelper = other.pHelper;
		other.pHelper = nullptr;
		m_pNextDesc = nullptr;
		return *this;
	}

	template <typename R, typename ...Args>
	void func_desc_t::initialize_shared(std::string_view name, std::string_view script_name, bool va)
	{
		m_desc.m_pszFunction = name.data();
		m_desc.m_pszScriptName = script_name.data();

		m_desc.m_ReturnType = __type_to_field_impl<std::decay_t<R>>();
		(m_desc.m_Parameters.emplace_back(__type_to_field_impl<std::decay_t<Args>>()), ...);

		if(va) {
			m_flags |= SF_VA_FUNC;

			constexpr std::size_t va_start{sizeof...(Args)};
			if(va_start >= 2) {
				m_desc.m_Parameters.erase(va_start);
				m_desc.m_Parameters.erase(va_start);
			}
		}
	}

	template <typename R, typename C, typename ...Args, std::size_t ...I>
	R func_desc_t::call_member_impl(R(__attribute__((__thiscall__)) *binding_func)(C *, Args...), std::size_t adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 0) {
			return (static_cast<C *>(obj)->*mfp_from_func<R, C, Args...>(binding_func, adjustor))();
		} else {
			return (static_cast<C *>(obj)->*mfp_from_func<R, C, Args...>(binding_func, adjustor))(variant_to_value<std::decay_t<Args>>(args_var[I])...);
		}
	}

	template <typename R, typename ...Args, std::size_t ...I>
	R func_desc_t::call_impl(R(*binding_func)(Args...), const gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 0) {
			return binding_func();
		} else {
			return binding_func(variant_to_value<std::decay_t<Args>>(args_var[I])...);
		}
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding_member_singleton(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		if(!obj) {
			obj = &C::instance();
		}

		return binding_member<R, C, Args...>(binding_func, adjustor, obj, args_var, num_args, ret_var);
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding_member(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args)};

		if(!obj) {
			return false;
		}

		if(num_required_args > 0) {
			if(!args_var || num_args != static_cast<int>(num_required_args)) {
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

			call_member<R, C, Args...>(reinterpret_cast<R(__attribute__((__thiscall__)) *)(C *, Args...)>(binding_func), static_cast<std::size_t>(adjustor), obj, args_var);
		} else {
			if(!ret_var) {
				call_member<R, C, Args...>(reinterpret_cast<R(__attribute__((__thiscall__)) *)(C *, Args...)>(binding_func), static_cast<std::size_t>(adjustor), obj, args_var);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call_member<R, C, Args...>(reinterpret_cast<R(__attribute__((__thiscall__)) *)(C *, Args...)>(binding_func), static_cast<std::size_t>(adjustor), obj, args_var);
				} else {
					R ret_val{call_member<R, C, Args...>(reinterpret_cast<R(__attribute__((__thiscall__)) *)(C *, Args...)>(binding_func), static_cast<std::size_t>(adjustor), obj, args_var)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
			}
		}

		return true;
	}

	template <typename R, typename ...Args>
	bool func_desc_t::binding(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args)};

		if(obj || adjustor != 0) {
			return false;
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

			call<R, Args...>(reinterpret_cast<R(*)(Args...)>(binding_func), args_var);
		} else {
			if(!ret_var) {
				call<R, Args...>(reinterpret_cast<R(*)(Args...)>(binding_func), args_var);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call<R, Args...>(reinterpret_cast<R(*)(Args...)>(binding_func), args_var);
				} else {
					R ret_val{call<R, Args...>(reinterpret_cast<R(*)(Args...)>(binding_func), args_var)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
			}
		}

		return true;
	}

	template <typename R, typename C, typename ...Args, std::size_t ...I>
	R func_desc_t::call_member_va_impl(R(*binding_func)(C *, Args..., ...), std::size_t adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, const gsdk::ScriptVariant_t *args_var_va, std::size_t num_va, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 2) {
			return (static_cast<C *>(obj)->*mfp_from_func<R, C, Args...>(binding_func, adjustor))(reinterpret_cast<const script_variant_t *>(args_var_va), num_va);
		} else {
			return (static_cast<C *>(obj)->*mfp_from_func<R, C, Args...>(binding_func, adjustor))(variant_to_value<std::decay_t<Args>>(args_var[I])..., reinterpret_cast<const script_variant_t *>(args_var_va), num_va);
		}
	}

	template <typename R, typename ...Args, std::size_t ...I>
	R func_desc_t::call_va_impl(R(*binding_func)(Args..., ...), const gsdk::ScriptVariant_t *args_var, const gsdk::ScriptVariant_t *args_var_va, std::size_t num_va, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 2) {
			return binding_func(reinterpret_cast<const script_variant_t *>(args_var_va), num_va);
		} else {
			return binding_func(variant_to_value<std::decay_t<Args>>(args_var[I])..., reinterpret_cast<const script_variant_t *>(args_var_va), num_va);
		}
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding_member_singleton_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		if(!obj) {
			obj = &C::instance();
		}

		return binding_member_va<R, C, Args...>(binding_func, adjustor, obj, args_var, num_args, ret_var);
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding_member_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args) - 2};

		if(!obj) {
			return false;
		}

		const gsdk::ScriptVariant_t *args_var_va{args_var + num_required_args};

		std::size_t num_va{0};
		for(int i{num_required_args}; i < num_args-1; ++i) {
			if(args_var[i].m_type == gsdk::FIELD_HSCRIPT &&
				args_var[i+1].m_type == gsdk::FIELD_VOID) {
				break;
			}

			++num_va;
		}

		if(num_required_args > 0) {
			if(!args_var || num_args < static_cast<int>(num_required_args)) {
				return false;
			}
		}

		if constexpr(std::is_void_v<R>) {
			if(ret_var) {
				return false;
			}

			call_member_va<R, C, Args...>(reinterpret_cast<R(*)(C *, Args..., ...)>(binding_func), static_cast<std::size_t>(adjustor), obj, args_var, args_var_va, num_va);
		} else {
			if(!ret_var) {
				call_member_va<R, C, Args...>(reinterpret_cast<R(*)(C *, Args..., ...)>(binding_func), static_cast<std::size_t>(adjustor), obj, args_var, args_var_va, num_va);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call_member_va<R, C, Args...>(reinterpret_cast<R(*)(C *, Args..., ...)>(binding_func), static_cast<std::size_t>(adjustor), obj, args_var, args_var_va, num_va);
				} else {
					R ret_val{call_member_va<R, C, Args...>(reinterpret_cast<R(*)(C *, Args..., ...)>(binding_func), static_cast<std::size_t>(adjustor), obj, args_var, args_var_va, num_va)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
			}
		}

		return true;
	}

	template <typename R, typename ...Args>
	bool func_desc_t::binding_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args) - 2};

		if(obj || adjustor != 0) {
			return false;
		}

		const gsdk::ScriptVariant_t *args_var_va{args_var + num_required_args};

		std::size_t num_va{0};
		for(int i{num_required_args}; i < num_args-1; ++i) {
			if(args_var[i].m_type == gsdk::FIELD_HSCRIPT &&
				args_var[i+1].m_type == gsdk::FIELD_VOID) {
				break;
			}

			++num_va;
		}

		if(num_required_args > 0) {
			if(!args_var || num_args < static_cast<int>(num_required_args)) {
				return false;
			}
		}

		if constexpr(std::is_void_v<R>) {
			if(ret_var) {
				return false;
			}

			call_va<R, Args...>(reinterpret_cast<R(*)(Args..., ...)>(binding_func), args_var, args_var_va, num_va);
		} else {
			if(!ret_var) {
				call_va<R, Args...>(reinterpret_cast<R(*)(Args..., ...)>(binding_func), args_var, args_var_va, num_va);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call_va<R, Args...>(reinterpret_cast<R(*)(Args..., ...)>(binding_func), args_var, args_var_va, num_va);
				} else {
					R ret_val{call_va<R, Args...>(reinterpret_cast<R(*)(Args..., ...)>(binding_func), args_var, args_var_va, num_va)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
			}
		}

		return true;
	}

	template <typename R, typename C, typename ...Args>
	void func_desc_t::initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view script_name)
	{
		m_desc.m_pszDescription = demangle<R(C::*)(Args...)>().c_str();
		auto mfp{mfp_to_func<R, C, Args...>(func)};
		m_pFunction = reinterpret_cast<void *>(mfp.first);
		m_adjustor = static_cast<int>(mfp.second);
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_member<R, C, Args...>);
		m_flags = gsdk::SF_MEMBER_FUNC;
		initialize_shared<R, Args...>(name, script_name, false);
	}

	template <typename R, typename C, typename ...Args>
	void func_desc_t::initialize_member(R(C::*func)(Args..., ...), std::string_view name, std::string_view script_name)
	{
		m_desc.m_pszDescription = demangle<R(C::*)(Args..., ...)>().c_str();
		auto mfp{mfp_to_func<R, C, Args...>(func)};
		m_pFunction = reinterpret_cast<void *>(mfp.first);
		m_adjustor = static_cast<int>(mfp.second);
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_member_va<R, C, Args...>);
		m_flags = gsdk::SF_MEMBER_FUNC;
		initialize_shared<R, Args...>(name, script_name, true);
	}

	template <typename R, typename ...Args>
	void func_desc_t::initialize_static(R(*func)(Args...), std::string_view name, std::string_view script_name)
	{
		m_desc.m_pszDescription = demangle<R(*)(Args...)>().c_str();
		m_pFunction = reinterpret_cast<void *>(func);
		m_adjustor = 0;
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding<R, Args...>);
		m_flags = 0;
		initialize_shared<R, Args...>(name, script_name, false);
	}

	template <typename R, typename ...Args>
	void func_desc_t::initialize_static(R(*func)(Args..., ...), std::string_view name, std::string_view script_name)
	{
		m_desc.m_pszDescription = demangle<R(*)(Args..., ...)>().c_str();
		m_pFunction = reinterpret_cast<void *>(func);
		m_adjustor = 0;
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_va<R, Args...>);
		m_flags = 0;
		initialize_shared<R, Args...>(name, script_name, true);
	}
}
