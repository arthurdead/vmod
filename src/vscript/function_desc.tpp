#include "../hacking.hpp"
#include "variant.hpp"
#include "../type_traits.hpp"
#include "../gsdk.hpp"

namespace vmod::vscript
{
	template <typename R, typename ...Args>
	void function_desc::initialize_shared(std::string_view name, std::string_view script_name, bool va)
	{
		constexpr std::size_t num_args{sizeof...(Args)};

		m_desc.m_pszDescription = "@";

		m_desc.m_pszFunction = name.data();
		m_desc.m_pszScriptName = script_name.data();

		m_desc.m_ReturnType = gsdk::IScriptVM::fixup_var_field(type_to_field<std::decay_t<R>>());
		(m_desc.m_Parameters.emplace_back(gsdk::IScriptVM::fixup_var_field(type_to_field<std::decay_t<Args>>())), ...);

		if constexpr(num_args > 0) {
			using LA = std::tuple_element_t<num_args-1, std::tuple<Args...>>;
			if constexpr(is_optional<LA>::value) {
				m_flags |= SF_OPT_FUNC;
			}
		}

		if(va) {
			m_flags |= SF_VA_FUNC;

			if(num_args >= 2) {
				m_desc.m_Parameters.erase(num_args);
				m_desc.m_Parameters.erase(num_args);
			}
		}
	}

	template <typename R, typename C, typename ...Args, std::size_t ...I>
	R function_desc::call_member_impl(R(C::*func)(Args...), C *obj, const gsdk::ScriptVariant_t *args, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 0) {
			return (obj->*func)();
		} else {
			return (obj->*func)(to_value<std::decay_t<Args>>(args[I])...);
		}
	}

	template <typename R, typename ...Args, std::size_t ...I>
	R function_desc::call_impl(R(*func)(Args...), const gsdk::ScriptVariant_t *args, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 0) {
			return func();
		} else {
			return func(to_value<std::decay_t<Args>>(args[I])...);
		}
	}

	template <typename R, typename C, typename ...Args>
	bool function_desc::binding_member_singleton(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		if(!obj_ptr) {
			if constexpr(class_is_singleton_v<C>) {
				obj_ptr = &C::instance();
			} else {
				detail::raise_exception("vmod: missing object");
				return false;
			}
		}

		return binding_member<R, C, Args...>(func_storage, obj_ptr, args, num_args, ret);
	}

	template <typename R, typename C, typename ...Args>
	bool function_desc::binding_member(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args)};

		C *obj{static_cast<C *>(obj_ptr)};

		if(!obj) {
			detail::raise_exception("vmod: missing object");
			return false;
		}

		if(num_required_args > 0) {
			if(!args || num_args != static_cast<int>(num_required_args)) {
				detail::raise_exception("vmod: wrong number of parameters expected %zu got %i", num_required_args, num_args);
				return false;
			}
		} else {
			if(/*args ||*/ num_args != 0) {
				detail::raise_exception("vmod: wrong number of parameters expected %zu got %i", num_required_args, num_args);
				return false;
			}
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		R(C::*func)(Args...){reinterpret_cast<R(C::*)(Args...)>(func_storage.mfp)};
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		R(C::*func)(Args...){mfp_from_func(reinterpret_cast<R(__attribute__((__thiscall__)) *)(C *, Args...)>(func_storage.plain))};
	#else
		#error
	#endif
		#pragma GCC diagnostic pop

		if constexpr(std::is_void_v<R>) {
			if(ret) {
				detail::raise_exception("vmod: function is void");
				return false;
			}

			call_member<R, C, Args...>(func, obj, args);
		} else {
			if(!ret) {
				call_member<R, C, Args...>(func, obj, args);
			} else {
				if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<R>>) {
					*ret = call_member<R, C, Args...>(func, obj, args);
				} else {
					R ret_val{call_member<R, C, Args...>(func, obj, args)};
					to_variant<R>(*ret, std::forward<R>(ret_val));
				}
				gsdk::IScriptVM::fixup_var(*ret);
			}
		}

		return true;
	}

	template <typename R, typename ...Args>
	bool function_desc::binding(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args)};

		if(obj_ptr) {
			detail::raise_exception("vmod: not a member function");
			return false;
		}

		if(num_required_args > 0) {
			if(!args || num_args != num_required_args) {
				detail::raise_exception("vmod: wrong number of parameters expected %zu got %i", num_required_args, num_args);
				return false;
			}
		} else {
			if(/*args ||*/ num_args != 0) {
				detail::raise_exception("vmod: wrong number of parameters expected %zu got %i", num_required_args, num_args);
				return false;
			}
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		R(*func)(Args...){reinterpret_cast<R(*)(Args...)>(func_storage.func)};
		#pragma GCC diagnostic pop

		if constexpr(std::is_void_v<R>) {
			if(ret) {
				detail::raise_exception("vmod: return is void");
				return false;
			}

			call<R, Args...>(func, args);
		} else {
			if(!ret) {
				call<R, Args...>(func, args);
			} else {
				if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<R>>) {
					*ret = call<R, Args...>(func, args);
				} else {
					R ret_val{call<R, Args...>(func, args)};
					to_variant<R>(*ret, std::forward<R>(ret_val));
				}
				gsdk::IScriptVM::fixup_var(*ret);
			}
		}

		return true;
	}

	template <typename R, typename C, typename ...Args, std::size_t ...I>
	R function_desc::call_member_va_impl(R(C::*func)(Args..., ...), C *obj, const gsdk::ScriptVariant_t *args, const gsdk::ScriptVariant_t *args_va, std::size_t num_va, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 2) {
			return (obj->*func)(static_cast<const variant *>(args_va), num_va);
		} else {
			return (obj->*func)(to_value<std::decay_t<Args>>(args[I])..., static_cast<const variant *>(args_va), num_va);
		}
	}

	template <typename R, typename ...Args, std::size_t ...I>
	R function_desc::call_va_impl(R(*func)(Args..., ...), const gsdk::ScriptVariant_t *args, const gsdk::ScriptVariant_t *args_va, std::size_t num_va, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 2) {
			return func(static_cast<const variant *>(args_va), num_va);
		} else {
			return func(to_value<std::decay_t<Args>>(args[I])..., static_cast<const variant *>(args_va), num_va);
		}
	}

	template <typename R, typename C, typename ...Args>
	bool function_desc::binding_member_singleton_va(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		if(!obj_ptr) {
			if constexpr(class_is_singleton_v<C>) {
				obj_ptr = &C::instance();
			} else {
				detail::raise_exception("vmod: missing object");
				return false;
			}
		}

		return binding_member_va<R, C, Args...>(func_storage, obj_ptr, args, num_args, ret);
	}

	template <typename R, typename C, typename ...Args>
	bool function_desc::binding_member_va(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args) - 2};

		C *obj{static_cast<C *>(obj_ptr)};

		if(!obj) {
			detail::raise_exception("vmod: missing object");
			return false;
		}

		const gsdk::ScriptVariant_t *args_va{args + num_required_args};

		std::size_t num_va{0};
		for(int i{num_required_args}; i < num_args-1; ++i) {
			if(args[i].m_type == gsdk::FIELD_HSCRIPT &&
				args[i+1].m_type == gsdk::FIELD_VOID) {
				break;
			}

			++num_va;
		}

		if(num_required_args > 0) {
			if(!args || num_args < static_cast<int>(num_required_args)) {
				detail::raise_exception("vmod: wrong number of parameters expected %zu got %i", num_required_args, num_args);
				return false;
			}
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		R(C::*func)(Args..., ...){reinterpret_cast<R(C::*)(Args..., ...)>(func_storage.mfp)};
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		R(C::*func)(Args..., ...){mfp_from_func(reinterpret_cast<R(*)(C *, Args..., ...)>(func_storage.plain))};
	#else
		#error
	#endif
		#pragma GCC diagnostic pop

		if constexpr(std::is_void_v<R>) {
			if(ret) {
				detail::raise_exception("vmod: return is void");
				return false;
			}

			call_member_va<R, C, Args...>(func, obj, args, args_va, num_va);
		} else {
			if(!ret) {
				call_member_va<R, C, Args...>(func, obj, args, args_va, num_va);
			} else {
				if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<R>>) {
					*ret = call_member_va<R, C, Args...>(func, obj, args, args_va, num_va);
				} else {
					R ret_val{call_member_va<R, C, Args...>(func, obj, args, args_va, num_va)};
					to_variant<R>(*ret, std::forward<R>(ret_val));
				}
				gsdk::IScriptVM::fixup_var(*ret);
			}
		}

		return true;
	}

	template <typename R, typename ...Args>
	bool function_desc::binding_va(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		constexpr std::size_t num_required_args{sizeof...(Args) - 2};

		if(obj_ptr) {
			detail::raise_exception("vmod: not a member function");
			return false;
		}

		const gsdk::ScriptVariant_t *args_va{args + num_required_args};

		std::size_t num_va{0};
		for(int i{num_required_args}; i < num_args-1; ++i) {
			if(args[i].m_type == gsdk::FIELD_HSCRIPT &&
				args[i+1].m_type == gsdk::FIELD_VOID) {
				break;
			}

			++num_va;
		}

		if(num_required_args > 0) {
			if(!args || num_args < static_cast<int>(num_required_args)) {
				detail::raise_exception("vmod: wrong number of parameters expected %zu got %i", num_required_args, num_args);
				return false;
			}
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		R(*func)(Args..., ...){reinterpret_cast<R(*)(Args..., ...)>(func_storage.func)};
		#pragma GCC diagnostic pop

		if constexpr(std::is_void_v<R>) {
			if(ret) {
				detail::raise_exception("vmod: return is void");
				return false;
			}

			call_va<R, Args...>(func, args, args_va, num_va);
		} else {
			if(!ret) {
				call_va<R, Args...>(func, args, args_va, num_va);
			} else {
				if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<R>>) {
					*ret = call_va<R, Args...>(func, args, args_va, num_va);
				} else {
					R ret_val{call_va<R, Args...>(func, args, args_va, num_va)};
					to_variant<R>(*ret, std::forward<R>(ret_val));
				}
				gsdk::IScriptVM::fixup_var(*ret);
			}
		}

		return true;
	}

	template <typename R, typename C, typename ...Args>
	void function_desc::initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view script_name)
	{
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		m_pFunction.mfp = reinterpret_cast<generic_mfp_t>(func);
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		auto internal{mfp_to_func(func)};
		m_pFunction.plain = reinterpret_cast<generic_plain_mfp_t>(internal.first);
		if(internal.second != 0) {
			using namespace std::literals::string_view_literals;
			error("vmod: member function '%s::%s' has base adjustment\n"sv, demangle<C>().c_str(), name.data());
		}
	#else
		#error
	#endif
		#pragma GCC diagnostic pop
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_member<R, C, Args...>);
		m_flags = gsdk::SF_MEMBER_FUNC;
		initialize_shared<R, Args...>(name, script_name, false);
	}

	template <typename R, typename C, typename ...Args>
	void function_desc::initialize_member(R(C::*func)(Args..., ...), std::string_view name, std::string_view script_name)
	{
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		m_pFunction.mfp = reinterpret_cast<generic_mfp_t>(func);
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		auto internal{mfp_to_func(func)};
		m_pFunction.plain = reinterpret_cast<generic_plain_mfp_t>(internal.first);
		if(internal.second != 0) {
			using namespace std::literals::string_view_literals;
			error("vmod: member function '%s::%s' has base adjustment\n"sv, demangle<C>().c_str(), name.data());
		}
	#else
		#error
	#endif
		#pragma GCC diagnostic pop
		m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(binding_member_va<R, C, Args...>);
		m_flags = gsdk::SF_MEMBER_FUNC;
		initialize_shared<R, Args...>(name, script_name, true);
	}

	template <typename R, typename ...Args>
	void function_desc::initialize_static(R(*func)(Args...), std::string_view name, std::string_view script_name)
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
	void function_desc::initialize_static(R(*func)(Args..., ...), std::string_view name, std::string_view script_name)
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
