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

	#ifndef __VMOD_USING_CUSTOM_VM
		m_desc.m_ReturnType = gsdk::IScriptVM::fixup_var_field(type_to_field<std::decay_t<R>>());
		(m_desc.m_Parameters.emplace_back(gsdk::IScriptVM::fixup_var_field(type_to_field<std::decay_t<Args>>())), ...);
	#else
		m_desc.m_ReturnType = type_to_field_and_flags<std::decay_t<R>>();
		(m_desc.m_Parameters.emplace_back(type_to_field_and_flags<std::decay_t<Args>>()), ...);
	#endif

		if(va) {
			m_flags |= gsdk::SF_VA_FUNC;

			if(num_args >= 2) {
				m_desc.m_Parameters.erase(num_args);
				m_desc.m_Parameters.erase(num_args);
			}
		}
	}

	namespace detail
	{
		template <std::size_t I, typename T>
		static std::remove_reference_t<T> to_value(std::size_t num_args, gsdk::ScriptVariant_t *args) noexcept
		{
			if(I < num_args) {
				return vscript::to_value<T>(args[I]);
			} else {
				return missing_value<T>();
			}
		}
	}

	template <typename R, typename C, typename ...Args, std::size_t ...I>
	R function_desc::call_member_impl(R(C::*func)(Args...), C *obj, std::size_t num_args, gsdk::ScriptVariant_t *args, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 0) {
			return (obj->*func)();
		} else {
			return (obj->*func)(detail::to_value<I, Args>(num_args, args)...);
		}
	}

	template <typename R, typename ...Args, std::size_t ...I>
	R function_desc::call_impl(R(*func)(Args...), std::size_t num_args, gsdk::ScriptVariant_t *args, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 0) {
			return func();
		} else {
			return func(detail::to_value<I, Args>(num_args, args)...);
		}
	}

	template <typename R, typename C, typename ...Args>
	bool function_desc::binding_member_singleton(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
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

	namespace detail
	{
		template <typename T>
		static constexpr bool can_be_optional() noexcept;

		template <std::size_t i, typename T>
		static constexpr bool can_variant_be_optional() noexcept
		{
			if constexpr(can_be_optional<std::variant_alternative_t<i, T>>()) {
				return true;
			} else if constexpr(i < (std::variant_size_v<T>-1)) {
				return can_variant_be_optional<i+1, T>();
			} else {
				return false;
			}
		}

		template <typename T>
		static constexpr bool can_be_optional() noexcept
		{
			using decayed_t = std::decay_t<T>;

			if constexpr(is_optional<decayed_t>::value) {
				return true;
			} else if constexpr(is_std_variant<decayed_t>::value) {
				return can_variant_be_optional<0, T>();
			} else {
				return false;
			}
		}

		template <std::size_t i, typename T>
		static constexpr void num_required_args_impl(std::size_t &num) noexcept
		{
			if constexpr(can_be_optional<std::tuple_element_t<i, T>>()) {
				--num;

				if constexpr(i > 0) {
					num_required_args_impl<i-1, T>(num);
				}
			}
		}

		template <std::size_t offset, typename ...Args>
		static constexpr std::size_t num_required_args() noexcept
		{
			constexpr std::size_t actual_num{sizeof...(Args)};
			static_assert(offset <= actual_num);
			constexpr std::size_t offseted_num{actual_num - offset};
			std::size_t target_num{offseted_num};
			if constexpr(offseted_num > 0) {
				num_required_args_impl<offseted_num-1, std::tuple<Args...>>(target_num);
			}
			return target_num;
		}
	}

	template <typename R, typename C, typename ...Args>
	bool function_desc::binding_member(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		constexpr std::size_t num_required_args{detail::num_required_args<0, Args...>()};

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
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
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

			call_member<R, C, Args...>(func, obj, static_cast<std::size_t>(num_args), args);
		} else {
			if(!ret) {
				call_member<R, C, Args...>(func, obj, static_cast<std::size_t>(num_args), args);
			} else {
				if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<R>>) {
					*ret = call_member<R, C, Args...>(func, obj, static_cast<std::size_t>(num_args), args);
				} else {
					R ret_val{call_member<R, C, Args...>(func, obj, static_cast<std::size_t>(num_args), args)};
					to_variant<R>(*ret, std::forward<R>(ret_val));
				}
			#ifndef __VMOD_USING_CUSTOM_VM
				gsdk::IScriptVM::fixup_var(*ret);
			#endif
			}
		}

		return true;
	}

	template <typename R, typename ...Args>
	bool function_desc::binding(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		constexpr std::size_t num_required_args{detail::num_required_args<0, Args...>()};

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

			call<R, Args...>(func, static_cast<std::size_t>(num_args), args);
		} else {
			if(!ret) {
				call<R, Args...>(func, static_cast<std::size_t>(num_args), args);
			} else {
				if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<R>>) {
					*ret = call<R, Args...>(func, static_cast<std::size_t>(num_args), args);
				} else {
					R ret_val{call<R, Args...>(func, static_cast<std::size_t>(num_args), args)};
					to_variant<R>(*ret, std::forward<R>(ret_val));
				}
			#ifndef __VMOD_USING_CUSTOM_VM
				gsdk::IScriptVM::fixup_var(*ret);
			#endif
			}
		}

		return true;
	}

	template <typename R, typename C, typename ...Args, std::size_t ...I>
	R function_desc::call_member_va_impl(R(C::*func)(Args..., ...), C *obj, std::size_t num_args, gsdk::ScriptVariant_t *args, gsdk::ScriptVariant_t *args_va, std::size_t num_va, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 2) {
			return (obj->*func)(static_cast<const variant *>(args_va), num_va);
		} else {
			return (obj->*func)(detail::to_value<I, Args>(num_args, args)..., static_cast<const variant *>(args_va), num_va);
		}
	}

	template <typename R, typename ...Args, std::size_t ...I>
	R function_desc::call_va_impl(R(*func)(Args..., ...), std::size_t num_args, gsdk::ScriptVariant_t *args, gsdk::ScriptVariant_t *args_va, std::size_t num_va, std::index_sequence<I...>) noexcept
	{
		if constexpr(sizeof...(Args) == 2) {
			return func(static_cast<const variant *>(args_va), num_va);
		} else {
			return func(detail::to_value<I, Args>(num_args, args)..., static_cast<const variant *>(args_va), num_va);
		}
	}

	template <typename R, typename C, typename ...Args>
	bool function_desc::binding_member_singleton_va(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
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
	bool function_desc::binding_member_va(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		constexpr std::size_t num_required_args{detail::num_required_args<2, Args...>()};

		C *obj{static_cast<C *>(obj_ptr)};

		if(!obj) {
			detail::raise_exception("vmod: missing object");
			return false;
		}

		gsdk::ScriptVariant_t *args_va{args + num_required_args};

	#ifndef __VMOD_USING_CUSTOM_VM
		std::size_t num_va{0};
		for(int i{num_required_args}; i < num_args-1; ++i) {
			if(args[i].m_type == gsdk::FIELD_HSCRIPT &&
				args[i+1].m_type == gsdk::FIELD_VOID) {
				break;
			}

			++num_va;
		}
	#else
		std::size_t num_va{static_cast<std::size_t>(num_args) - num_required_args};
	#endif

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
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
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

			call_member_va<R, C, Args...>(func, obj, static_cast<std::size_t>(num_args), args, args_va, num_va);
		} else {
			if(!ret) {
				call_member_va<R, C, Args...>(func, obj, static_cast<std::size_t>(num_args), args, args_va, num_va);
			} else {
				if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<R>>) {
					*ret = call_member_va<R, C, Args...>(func, obj, static_cast<std::size_t>(num_args), args, args_va, num_va);
				} else {
					R ret_val{call_member_va<R, C, Args...>(func, obj, static_cast<std::size_t>(num_args), args, args_va, num_va)};
					to_variant<R>(*ret, std::forward<R>(ret_val));
				}
			#ifndef __VMOD_USING_CUSTOM_VM
				gsdk::IScriptVM::fixup_var(*ret);
			#endif
			}
		}

		return true;
	}

	template <typename R, typename ...Args>
	bool function_desc::binding_va(gsdk::ScriptFunctionBindingStorageType_t func_storage, void *obj_ptr, gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept
	{
		constexpr std::size_t num_required_args{detail::num_required_args<2, Args...>()};

		if(obj_ptr) {
			detail::raise_exception("vmod: not a member function");
			return false;
		}

		gsdk::ScriptVariant_t *args_va{args + num_required_args};

	#ifndef __VMOD_USING_CUSTOM_VM
		std::size_t num_va{0};
		for(int i{num_required_args}; i < num_args-1; ++i) {
			if(args[i].m_type == gsdk::FIELD_HSCRIPT &&
				args[i+1].m_type == gsdk::FIELD_VOID) {
				break;
			}

			++num_va;
		}
	#else
		std::size_t num_va{static_cast<std::size_t>(num_args) - num_required_args};
	#endif

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

			call_va<R, Args...>(func, static_cast<std::size_t>(num_args), args, args_va, num_va);
		} else {
			if(!ret) {
				call_va<R, Args...>(func, static_cast<std::size_t>(num_args), args, args_va, num_va);
			} else {
				if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<R>>) {
					*ret = call_va<R, Args...>(func, static_cast<std::size_t>(num_args), args, args_va, num_va);
				} else {
					R ret_val{call_va<R, Args...>(func, static_cast<std::size_t>(num_args), args, args_va, num_va)};
					to_variant<R>(*ret, std::forward<R>(ret_val));
				}
			#ifndef __VMOD_USING_CUSTOM_VM
				gsdk::IScriptVM::fixup_var(*ret);
			#endif
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
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
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
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
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
