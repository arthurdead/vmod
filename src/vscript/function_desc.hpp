#pragma once

#include "vscript.hpp"

namespace vmod::vscript
{
	namespace detail
	{
		template <typename T>
		class base_class_desc;
	}

	template <typename T>
	class singleton_class_desc;

	class alignas(gsdk::ScriptFunctionBinding_t) function_desc : public gsdk::ScriptFunctionBinding_t
	{
	public:
		function_desc() noexcept = default;

		template <typename R, typename ...Args>
		inline void initialize(R(*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{ initialize_static<R, Args...>(func, name, script_name); }

		template <typename R, typename ...Args>
		inline void initialize(R(*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{ initialize_static<R, Args...>(func, name, script_name); }

		void desc(std::string &&description) = delete;

		inline void desc(std::string_view description) noexcept
		{
			m_desc.m_pszDescription = description.data();
		}

	private:
		function_desc(function_desc &&other) noexcept = default;
		function_desc &operator=(function_desc &&other) noexcept = default;

		template <typename T>
		friend class detail::base_class_desc;
		template <typename T>
		friend class singleton_class_desc;

		template <typename R, typename C, typename ...Args>
		inline function_desc(R(C::*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{ initialize_member<R, C, Args...>(func, name, script_name); }

		template <typename R, typename C, typename ...Args>
		inline function_desc(R(C::*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{ initialize_member<R, C, Args...>(func, name, script_name); }

		template <typename R, typename ...Args>
		inline function_desc(R(*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{ initialize<R, Args...>(func, name, script_name); }

		template <typename R, typename ...Args>
		inline function_desc(R(*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{ initialize<R, Args...>(func, name, script_name); }

		template <typename R, typename C, typename ...Args>
		void initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view script_name);

		template <typename R, typename C, typename ...Args>
		void initialize_member(R(C::*func)(Args..., ...), std::string_view name, std::string_view script_name);

		template <typename R, typename ...Args>
		void initialize_static(R(*func)(Args...), std::string_view name, std::string_view script_name);

		template <typename R, typename ...Args>
		void initialize_static(R(*func)(Args..., ...), std::string_view name, std::string_view script_name);

		template <typename R, typename ...Args>
		void initialize_shared(std::string_view name, std::string_view script_name, bool va);

		template <typename R, typename C, typename ...Args>
		static bool binding_member_singleton(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept;

		template <typename R, typename C, typename ...Args>
		static bool binding_member_singleton_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept;

		template <typename R, typename C, typename ...Args>
		static bool binding_member(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept;

		template <typename R, typename C, typename ...Args>
		static bool binding_member_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept;

		template <typename R, typename ...Args>
		static bool binding(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept;

		template <typename R, typename ...Args>
		static bool binding_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *obj, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret) noexcept;

		template <typename R, typename C, typename ...Args, std::size_t ...I>
		static R call_member_impl(R(C::*func)(Args...), C *obj, const gsdk::ScriptVariant_t *args, std::index_sequence<I...>) noexcept;

		template <typename R, typename C, typename ...Args>
		static inline R call_member(R(C::*func)(Args...), C *obj, const gsdk::ScriptVariant_t *args) noexcept
		{ return call_member_impl<R, C, Args...>(func, obj, args, std::make_index_sequence<sizeof...(Args)>()); }

		template <typename R, typename C, typename ...Args, std::size_t ...I>
		static R call_member_va_impl(R(C::*func)(Args..., ...), C *obj, const gsdk::ScriptVariant_t *args, const gsdk::ScriptVariant_t *args_va, std::size_t num_va, std::index_sequence<I...>) noexcept;

		template <typename R, typename C, typename ...Args>
		static inline R call_member_va(R(C::*func)(Args..., ...), C *obj, const gsdk::ScriptVariant_t *args, const gsdk::ScriptVariant_t *args_va, std::size_t num_va) noexcept
		{ return call_member_va_impl<R, C, Args...>(func, obj, args, args_va, num_va, std::make_index_sequence<sizeof...(Args)>()); }

		template <typename R, typename ...Args, std::size_t ...I>
		static R call_impl(R(*func)(Args...), const gsdk::ScriptVariant_t *args, std::index_sequence<I...>) noexcept;

		template <typename R, typename ...Args>
		static inline R call(R(*func)(Args...), const gsdk::ScriptVariant_t *args) noexcept
		{ return call_impl<R, Args...>(func, args, std::make_index_sequence<sizeof...(Args)>()); }

		template <typename R, typename ...Args, std::size_t ...I>
		static R call_va_impl(R(*func)(Args..., ...), const gsdk::ScriptVariant_t *args, const gsdk::ScriptVariant_t *args_va, std::size_t num_va, std::index_sequence<I...>) noexcept;

		template <typename R, typename ...Args>
		static inline R call_va(R(*func)(Args..., ...), const gsdk::ScriptVariant_t *args, const gsdk::ScriptVariant_t *args_va, std::size_t num_va) noexcept
		{ return call_va_impl<R, Args...>(func, args, args_va, num_va, std::make_index_sequence<sizeof...(Args)>()); }

	private:
		function_desc(const function_desc &) = delete;
		function_desc &operator=(const function_desc &) = delete;
	};

	static_assert(sizeof(function_desc) == sizeof(gsdk::ScriptFunctionBinding_t));
	static_assert(alignof(function_desc) == alignof(gsdk::ScriptFunctionBinding_t));
}

#include "function_desc.tpp"
