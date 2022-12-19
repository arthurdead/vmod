namespace vmod
{
	extern void __vmod_raiseexception(const char *str) noexcept;

	template <typename T>
	base_class_desc_t<T>::base_class_desc_t(std::string_view name) noexcept
		: gsdk::ScriptClassDesc_t{}
	{
		m_pfnConstruct = nullptr;
		m_pfnDestruct = nullptr;
		pHelper = nullptr;
		m_pNextDesc = reinterpret_cast<ScriptClassDesc_t *>(0xbebebebe);
		m_pBaseDesc = nullptr;
		m_pszClassname = demangle<T>().c_str();
		m_pszScriptName = name.data();
		m_pszDescription = "@";
	}

	template <typename T>
	base_class_desc_t<T> &base_class_desc_t<T>::operator=(base_class_desc_t &&other) noexcept
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
		m_desc.m_pszDescription = "@";

		m_desc.m_pszFunction = name.data();
		m_desc.m_pszScriptName = script_name.data();

		m_desc.m_ReturnType = __type_to_field_impl<std::decay_t<R>>();
		(m_desc.m_Parameters.emplace_back(gsdk::IScriptVM::fixup_var_field(__type_to_field_impl<std::decay_t<Args>>())), ...);

		constexpr std::size_t num_args{sizeof...(Args)};
		if constexpr(num_args > 0) {
			using LA = std::tuple_element_t<num_args-1, std::tuple<Args...>>;
			if constexpr(is_optional<LA>::value) {
				m_flags |= SF_OPT_FUNC;
			}
		}

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
	R func_desc_t::call_member_impl(R(C::*func)(Args...), void *obj, const gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 0) {
			return (static_cast<C *>(obj)->*func)();
		} else {
			return (static_cast<C *>(obj)->*func)(__variant_to_value_impl<std::decay_t<Args>>(args_var[I])...);
		}
	}

	template <typename R, typename ...Args, std::size_t ...I>
	R func_desc_t::call_impl(R(*func)(Args...), const gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 0) {
			return func();
		} else {
			return func(__variant_to_value_impl<std::decay_t<Args>>(args_var[I])...);
		}
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding_member_singleton(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		if(!obj) {
			if constexpr(singleton_instance_helper<C>::instance_func_available_v) {
				obj = &singleton_instance_helper<C>::instance();
			} else {
				__vmod_raiseexception("vmod: missing this");
				return false;
			}
		}

		return binding_member<R, C, Args...>(binding_func, obj, args_var, num_args, ret_var);
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding_member(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args)};

		if(!obj) {
			__vmod_raiseexception("vmod: missing this");
			return false;
		}

		if(num_required_args > 0) {
			if(!args_var || num_args != static_cast<int>(num_required_args)) {
				__vmod_raiseexception("wrong number of parameters");
				return false;
			}
		} else {
			if(/*args_var ||*/ num_args != 0) {
				__vmod_raiseexception("wrong number of parameters");
				return false;
			}
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		R(C::*func)(Args...){reinterpret_cast<R(C::*)(Args...)>(binding_func.mfp)};
		#pragma GCC diagnostic pop

		if constexpr(std::is_void_v<R>) {
			if(ret_var) {
				__vmod_raiseexception("vmod: function is void");
				return false;
			}

			call_member<R, C, Args...>(func, obj, args_var);
		} else {
			if(!ret_var) {
				call_member<R, C, Args...>(func, obj, args_var);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call_member<R, C, Args...>(func, obj, args_var);
				} else {
					R ret_val{call_member<R, C, Args...>(func, obj, args_var)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
				gsdk::IScriptVM::fixup_var(*ret_var);
			}
		}

		return true;
	}

	template <typename R, typename ...Args>
	bool func_desc_t::binding(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args)};

		if(obj) {
			__vmod_raiseexception("vmod: static function");
			return false;
		}

		if(num_required_args > 0) {
			if(!args_var || num_args != num_required_args) {
				__vmod_raiseexception("wrong number of parameters");
				return false;
			}
		} else {
			if(/*args_var ||*/ num_args != 0) {
				__vmod_raiseexception("wrong number of parameters");
				return false;
			}
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		R(*func)(Args...){reinterpret_cast<R(*)(Args...)>(binding_func.func)};
		#pragma GCC diagnostic pop

		if constexpr(std::is_void_v<R>) {
			if(ret_var) {
				__vmod_raiseexception("vmod: function is void");
				return false;
			}

			call<R, Args...>(func, args_var);
		} else {
			if(!ret_var) {
				call<R, Args...>(func, args_var);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call<R, Args...>(func, args_var);
				} else {
					R ret_val{call<R, Args...>(func, args_var)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
				gsdk::IScriptVM::fixup_var(*ret_var);
			}
		}

		return true;
	}

	template <typename R, typename C, typename ...Args, std::size_t ...I>
	R func_desc_t::call_member_va_impl(R(C::*func)(Args..., ...), void *obj, const gsdk::ScriptVariant_t *args_var, const gsdk::ScriptVariant_t *args_var_va, std::size_t num_va, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 2) {
			return (static_cast<C *>(obj)->*func)(static_cast<const script_variant_t *>(args_var_va), num_va);
		} else {
			return (static_cast<C *>(obj)->*func)(__variant_to_value_impl<std::decay_t<Args>>(args_var[I])..., static_cast<const script_variant_t *>(args_var_va), num_va);
		}
	}

	template <typename R, typename ...Args, std::size_t ...I>
	R func_desc_t::call_va_impl(R(*func)(Args..., ...), const gsdk::ScriptVariant_t *args_var, const gsdk::ScriptVariant_t *args_var_va, std::size_t num_va, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 2) {
			return func(static_cast<const script_variant_t *>(args_var_va), num_va);
		} else {
			return func(__variant_to_value_impl<std::decay_t<Args>>(args_var[I])..., static_cast<const script_variant_t *>(args_var_va), num_va);
		}
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding_member_singleton_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		if(!obj) {
			if constexpr(singleton_instance_helper<C>::instance_func_available_v) {
				obj = &singleton_instance_helper<C>::instance();
			} else {
				__vmod_raiseexception("vmod: missing this");
				return false;
			}
		}

		return binding_member_va<R, C, Args...>(binding_func, obj, args_var, num_args, ret_var);
	}

	template <typename R, typename C, typename ...Args>
	bool func_desc_t::binding_member_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args) - 2};

		if(!obj) {
			__vmod_raiseexception("vmod: missing this");
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
				__vmod_raiseexception("wrong number of parameters");
				return false;
			}
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		R(C::*func)(Args..., ...){reinterpret_cast<R(C::*)(Args..., ...)>(binding_func.mfp)};
		#pragma GCC diagnostic pop

		if constexpr(std::is_void_v<R>) {
			if(ret_var) {
				__vmod_raiseexception("vmod: function is void");
				return false;
			}

			call_member_va<R, C, Args...>(func, obj, args_var, args_var_va, num_va);
		} else {
			if(!ret_var) {
				call_member_va<R, C, Args...>(func, obj, args_var, args_var_va, num_va);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call_member_va<R, C, Args...>(func, obj, args_var, args_var_va, num_va);
				} else {
					R ret_val{call_member_va<R, C, Args...>(func, obj, args_var, args_var_va, num_va)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
				gsdk::IScriptVM::fixup_var(*ret_var);
			}
		}

		return true;
	}

	template <typename R, typename ...Args>
	bool func_desc_t::binding_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args) - 2};

		if(obj) {
			__vmod_raiseexception("vmod: static function");
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
				__vmod_raiseexception("wrong number of parameters");
				return false;
			}
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		R(*func)(Args..., ...){reinterpret_cast<R(*)(Args..., ...)>(binding_func.func)};
		#pragma GCC diagnostic pop

		if constexpr(std::is_void_v<R>) {
			if(ret_var) {
				__vmod_raiseexception("vmod: function is void");
				return false;
			}

			call_va<R, Args...>(func, args_var, args_var_va, num_va);
		} else {
			if(!ret_var) {
				call_va<R, Args...>(func, args_var, args_var_va, num_va);
			} else {
				if constexpr(std::is_same_v<R, script_variant_t>) {
					*ret_var = call_va<R, Args...>(func, args_var, args_var_va, num_va);
				} else {
					R ret_val{call_va<R, Args...>(func, args_var, args_var_va, num_va)};
					value_to_variant<R>(*ret_var, std::forward<R>(ret_val));
				}
				gsdk::IScriptVM::fixup_var(*ret_var);
			}
		}

		return true;
	}

	template <typename R, typename C, typename ...Args>
	void func_desc_t::initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view script_name)
	{
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		m_pFunction.mfp = reinterpret_cast<generic_mfp_t>(func);
		#pragma GCC diagnostic pop
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_member<R, C, Args...>);
		m_flags = gsdk::SF_MEMBER_FUNC;
		initialize_shared<R, Args...>(name, script_name, false);
	}

	template <typename R, typename C, typename ...Args>
	void func_desc_t::initialize_member(R(C::*func)(Args..., ...), std::string_view name, std::string_view script_name)
	{
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		m_pFunction.mfp = reinterpret_cast<generic_mfp_t>(func);
		#pragma GCC diagnostic pop
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_member_va<R, C, Args...>);
		m_flags = gsdk::SF_MEMBER_FUNC;
		initialize_shared<R, Args...>(name, script_name, true);
	}

	template <typename R, typename ...Args>
	void func_desc_t::initialize_static(R(*func)(Args...), std::string_view name, std::string_view script_name)
	{
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		m_pFunction.func = reinterpret_cast<generic_func_t>(func);
		#pragma GCC diagnostic pop
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding<R, Args...>);
		m_flags = 0;
		initialize_shared<R, Args...>(name, script_name, false);
	}

	template <typename R, typename ...Args>
	void func_desc_t::initialize_static(R(*func)(Args..., ...), std::string_view name, std::string_view script_name)
	{
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		m_pFunction.func = reinterpret_cast<generic_func_t>(func);
		#pragma GCC diagnostic pop
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_va<R, Args...>);
		m_flags = 0;
		initialize_shared<R, Args...>(name, script_name, true);
	}
}
