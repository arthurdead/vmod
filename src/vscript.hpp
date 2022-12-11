#pragma once

#include "gsdk/vscript/vscript.hpp"
#include <string_view>
#include <utility>
#include "hacking.hpp"
#include <cstring>

namespace vmod
{
	template <typename T>
	constexpr gsdk::ScriptDataType_t type_to_field() noexcept = delete;

	template <typename T>
	std::remove_reference_t<T> variant_to_value(const gsdk::ScriptVariant_t &) noexcept = delete;

	template <typename T>
	gsdk::ScriptVariant_t value_to_variant(T &&value) noexcept;
}

#include "vscript_variant.tpp"

namespace vmod
{
	template <typename T>
	inline void value_to_variant(gsdk::ScriptVariant_t &var, T &&value) noexcept
	{
		var.m_type = static_cast<short>(__type_to_field_impl<std::decay_t<T>>());
		var.m_flags = gsdk::SV_NOFLAGS;
		initialize_variant_value(var, std::forward<T>(value));
	}
}

namespace vmod
{
	inline void null_variant(gsdk::ScriptVariant_t &var) noexcept
	{
		var.m_type = gsdk::FIELD_VOID;
		var.m_ulonglong = 0;
		var.m_flags = gsdk::SV_NOFLAGS;
	}

	extern void free_variant_hscript(gsdk::ScriptVariant_t &var) noexcept;

	class alignas(gsdk::ScriptVariant_t) script_variant_t final : public gsdk::ScriptVariant_t
	{
	public:
		inline script_variant_t() noexcept
		{
			m_type = gsdk::FIELD_VOID;
			m_ulonglong = 0;
			m_flags = gsdk::SV_NOFLAGS;
		}

		inline ~script_variant_t() noexcept
		{ free(); }

		inline script_variant_t(script_variant_t &&other) noexcept
		{ operator=(std::move(other)); }

		inline script_variant_t &operator=(script_variant_t &&other) noexcept
		{
			m_type = other.m_type;
			m_ulonglong = other.m_ulonglong;
			m_flags = other.m_flags;
			other.m_flags = gsdk::SV_NOFLAGS;
			return *this;
		}

		script_variant_t(const script_variant_t &) = delete;
		script_variant_t &operator=(const script_variant_t &) = delete;

		template <typename T>
		inline script_variant_t(T &&value) noexcept
		{ value_to_variant<T>(*this, std::forward<T>(value)); }

		template <typename T>
		inline script_variant_t &operator=(T &&value) noexcept
		{
			free();
			value_to_variant<T>(*this, std::forward<T>(value));
			return *this;
		}

		template <typename T>
		inline script_variant_t &assign(T value) noexcept
		{
			free();
			value_to_variant<T>(*this, std::forward<T>(value));
			return *this;
		}

		template <typename T>
		inline bool operator==(const T &value) const noexcept
		{ return variant_to_value<T>(*this) == value; }
		template <typename T>
		inline bool operator!=(const T &value) const noexcept
		{ return variant_to_value<T>(*this) != value; }

		template <typename T>
		explicit inline operator T() const noexcept
		{ return variant_to_value<T>(*this); }

		template <typename T>
		inline T get() const noexcept
		{ return variant_to_value<T>(*this); }

	private:
		inline void free() noexcept
		{
			if(m_flags & gsdk::SV_FREE) {
				switch(m_type) {
					case gsdk::FIELD_VECTOR: {
						delete m_pVector;
					} break;
					case gsdk::FIELD_VECTOR2D: {
						//delete m_pVector2D;
					} break;
					case gsdk::FIELD_QANGLE: {
						delete m_pQAngle;
					} break;
					case gsdk::FIELD_QUATERNION: {
						//delete m_pQuaternion;
					} break;
					case gsdk::FIELD_UTLSTRINGTOKEN: {
						//delete m_pUtlStringToken;
					} break;
					case gsdk::FIELD_CSTRING: {
						std::free(const_cast<char *>(m_pszString));
					} break;
					case gsdk::FIELD_HSCRIPT: {
						free_variant_hscript(*this);
					} break;
					default: {
						std::free(reinterpret_cast<void *>(m_ulonglong));
					} break;
				}
				m_flags &= ~gsdk::SV_FREE;
			}
		}
	};

	static_assert(sizeof(script_variant_t) == sizeof(gsdk::ScriptVariant_t));
	static_assert(alignof(script_variant_t) == alignof(gsdk::ScriptVariant_t));

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<script_variant_t>() noexcept
	{ return gsdk::FIELD_VARIANT; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<const script_variant_t *>() noexcept
	{ return gsdk::FIELD_VOID; }
	template <>
	inline script_variant_t variant_to_value<script_variant_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		script_variant_t temp_var;
		temp_var.m_type = var.m_type;
		temp_var.m_flags = gsdk::SV_NOFLAGS;
		temp_var.m_ulonglong = var.m_ulonglong;
		return temp_var;
	}

	template <typename T>
	inline gsdk::ScriptVariant_t value_to_variant(T &&value) noexcept
	{
		script_variant_t var;
		value_to_variant<T>(var, std::forward<T>(value));
		return var;
	}

	class alignas(gsdk::ScriptFunctionBinding_t) func_desc_t : public gsdk::ScriptFunctionBinding_t
	{
	public:
		func_desc_t() noexcept = default;

		template <typename R, typename ...Args>
		inline void initialize(R(*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{ initialize_static<R, Args...>(func, name, script_name); }

		template <typename R, typename ...Args>
		inline void initialize(R(*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{ initialize_static<R, Args...>(func, name, script_name); }

		static constexpr int SF_VA_FUNC{(1 << 1)};

	private:
		func_desc_t(const func_desc_t &) = delete;
		func_desc_t &operator=(const func_desc_t &) = delete;

		inline func_desc_t(func_desc_t &&other) noexcept
		{ operator=(std::move(other)); }

		inline func_desc_t &operator=(func_desc_t &&other) noexcept
		{
			m_desc = std::move(other.m_desc);
			m_pfnBinding = other.m_pfnBinding;
			other.m_pfnBinding = nullptr;
			m_pFunction = other.m_pFunction;
			other.m_pFunction = nullptr;
			m_adjustor = other.m_adjustor;
			other.m_adjustor = 0;
			m_flags = other.m_flags;
			return *this;
		}

		template <typename M>
		func_desc_t(M, std::string_view) = delete;

		template <typename T>
		friend class base_class_desc_t;
		template <typename T>
		friend class singleton_class_desc_t;

		template <typename R, typename C, typename ...Args>
		inline func_desc_t(R(C::*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{ initialize_member<R, C, Args...>(func, name, script_name); }

		template <typename R, typename C, typename ...Args>
		inline func_desc_t(R(C::*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{ initialize_member<R, C, Args...>(func, name, script_name); }

		template <typename R, typename ...Args>
		inline func_desc_t(R(*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{ initialize<R, Args...>(func, name, script_name); }

		template <typename R, typename ...Args>
		inline func_desc_t(R(*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
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
		static bool binding_member_singleton(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept;

		template <typename R, typename C, typename ...Args>
		static bool binding_member_singleton_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept;

		template <typename R, typename C, typename ...Args>
		static bool binding_member(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept;

		template <typename R, typename C, typename ...Args>
		static bool binding_member_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept;

		template <typename R, typename ...Args>
		static bool binding(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept;

		template <typename R, typename ...Args>
		static bool binding_va(gsdk::ScriptFunctionBindingStorageType_t binding_func, int adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept;

		template <typename R, typename C, typename ...Args, std::size_t ...I>
		static R call_member_impl(R(__attribute__((__thiscall__)) *binding_func)(C *, Args...), std::size_t adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept;

		template <typename R, typename C, typename ...Args>
		static inline R call_member(R(__attribute__((__thiscall__)) *binding_func)(C *, Args...), std::size_t adjustor, void *obj, const gsdk::ScriptVariant_t *args_var) noexcept
		{ return call_member_impl<R, C, Args...>(binding_func, adjustor, obj, args_var, std::make_index_sequence<sizeof...(Args)>()); }

		template <typename R, typename C, typename ...Args, std::size_t ...I>
		static R call_member_va_impl(R(*binding_func)(C *, Args..., ...), std::size_t adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, const gsdk::ScriptVariant_t *args_var_va, std::size_t num_va, std::index_sequence<I...>) noexcept;

		template <typename R, typename C, typename ...Args>
		static inline R call_member_va(R(*binding_func)(C *, Args..., ...), std::size_t adjustor, void *obj, const gsdk::ScriptVariant_t *args_var, const gsdk::ScriptVariant_t *args_var_va, std::size_t num_va) noexcept
		{ return call_member_va_impl<R, C, Args...>(binding_func, adjustor, obj, args_var, args_var_va, num_va, std::make_index_sequence<sizeof...(Args)>()); }

		template <typename R, typename ...Args, std::size_t ...I>
		static R call_impl(R(*binding_func)(Args...), const gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept;

		template <typename R, typename ...Args>
		static inline R call(R(*binding_func)(Args...), const gsdk::ScriptVariant_t *args_var) noexcept
		{ return call_impl<R, Args...>(binding_func, args_var, std::make_index_sequence<sizeof...(Args)>()); }

		template <typename R, typename ...Args, std::size_t ...I>
		static R call_va_impl(R(*binding_func)(Args..., ...), const gsdk::ScriptVariant_t *args_var, const gsdk::ScriptVariant_t *args_var_va, std::size_t num_va, std::index_sequence<I...>) noexcept;

		template <typename R, typename ...Args>
		static inline R call_va(R(*binding_func)(Args..., ...), const gsdk::ScriptVariant_t *args_var, const gsdk::ScriptVariant_t *args_var_va, std::size_t num_va) noexcept
		{ return call_va_impl<R, Args...>(binding_func, args_var, args_var_va, num_va, std::make_index_sequence<sizeof...(Args)>()); }
	};

	static_assert(sizeof(func_desc_t) == sizeof(gsdk::ScriptFunctionBinding_t));
	static_assert(alignof(func_desc_t) == alignof(gsdk::ScriptFunctionBinding_t));

	template <typename T>
	class instance_helper : public gsdk::IScriptInstanceHelper
	{
	public:
		virtual ~instance_helper() noexcept = default;

		inline void *GetProxied(void *ptr) noexcept override
		{ return ptr; }

		inline bool ToString(void *ptr, char *buff, int siz) noexcept override
		{
			const std::string &name{demangle<T>()};
			std::snprintf(buff, static_cast<std::size_t>(siz), "(%s : %p)", name.c_str(), ptr);
			return true;
		}

		inline void *BindOnRead([[maybe_unused]] gsdk::HSCRIPT instance, void *ptr, [[maybe_unused]] const char *id) noexcept override
		{ return ptr; }

		static inline instance_helper &singleton() noexcept
		{
			static instance_helper singleton_;
			return singleton_;
		}
	};

	template <typename T>
	class singleton_instance_helper final : public instance_helper<T>
	{
	public:
		class instance_func_available
		{
			template <typename C>
			static std::true_type test(decltype(&C::instance));
			template <typename C>
			static std::false_type test(...);

		public:
			static constexpr bool value{std::is_same_v<decltype(test<T>(nullptr)), std::true_type>};
		};

		static constexpr bool instance_func_available_v{instance_func_available::value};

		static inline T &instance() noexcept
		{ return T::instance(); }

		inline void *GetProxied([[maybe_unused]] void *ptr) noexcept override final
		{
			if constexpr(instance_func_available_v) {
				return static_cast<T *>(&singleton_instance_helper<T>::instance());
			} else {
				return ptr;
			}
		}

		inline void *BindOnRead([[maybe_unused]] gsdk::HSCRIPT instance_, [[maybe_unused]] void *ptr, [[maybe_unused]] const char *id) noexcept override final
		{
			if constexpr(instance_func_available_v) {
				return static_cast<T *>(&singleton_instance_helper<T>::instance());
			} else {
				return ptr;
			}
		}

		static inline singleton_instance_helper &singleton() noexcept
		{
			static singleton_instance_helper singleton_;
			return singleton_;
		}
	};

	struct extra_class_desc_t final
	{
		std::string doc_class_name;
		bool singleton;
		std::string doc_singleton_name;
	};

	template <typename T>
	class base_class_desc_t : public gsdk::ScriptClassDesc_t
	{
	public:
		base_class_desc_t() = delete;
		base_class_desc_t(const base_class_desc_t &) = delete;
		base_class_desc_t &operator=(const base_class_desc_t &) = delete;

		inline base_class_desc_t(base_class_desc_t &&other) noexcept
			: gsdk::ScriptClassDesc_t{}
		{ operator=(std::move(other)); }

		base_class_desc_t &operator=(base_class_desc_t &&other) noexcept;

		base_class_desc_t(std::string_view name) noexcept;

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{
			func_desc_t temp{func, name, script_name};
			return static_cast<func_desc_t &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{
			func_desc_t temp{func, name, script_name};
			return static_cast<func_desc_t &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(T::*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{
			func_desc_t temp{func, name, script_name};
			return static_cast<func_desc_t &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(T::*func_)(Args...) const, std::string_view name, std::string_view script_name) noexcept
		{ return func<R, Args...>(reinterpret_cast<R(T::*)(Args...)>(func_), name, script_name); }

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(T::*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{
			func_desc_t temp{func, name, script_name};
			return static_cast<func_desc_t &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(T::*func_)(Args..., ...) const, std::string_view name, std::string_view script_name) noexcept
		{ return func<R, Args...>(reinterpret_cast<R(T::*)(Args..., ...)>(func_), name, script_name); }

		inline const extra_class_desc_t &extra() const noexcept
		{ return extra_; }

		inline base_class_desc_t &doc_class_name(std::string_view name) noexcept
		{
			extra_.doc_class_name = name;
			return *this;
		}

	protected:
		extra_class_desc_t extra_;
	};

	template <typename T>
	class class_desc_t final : public base_class_desc_t<T>
	{
	public:
		inline class_desc_t(std::string_view name) noexcept
			: base_class_desc_t<T>{name}
		{
			this->pHelper = &instance_helper<T>::singleton();
			this->extra_.singleton = true;
		}

		inline class_desc_t &operator=(instance_helper<T> &helper) noexcept
		{
			this->pHelper = &helper;
			return *this;
		}

		inline class_desc_t &dtor(void(*func)(T *)) noexcept
		{
			this->m_pfnDestruct = reinterpret_cast<void(*)(void *)>(func);
			return *this;
		}

		inline class_desc_t &dtor() noexcept
		{
			static_assert(std::is_destructible_v<T>);
			this->m_pfnDestruct = static_cast<void(*)(void *)>(destruct);
			return *this;
		}

		inline class_desc_t &ctor(T *(*func)()) noexcept
		{
			this->m_pfnConstruct = reinterpret_cast<void *(*)()>(func);
			return *this;
		}

		inline class_desc_t &ctor() noexcept
		{
			static_assert(std::is_default_constructible_v<T>);
			this->m_pfnConstruct = static_cast<void *(*)()>(construct);
			return *this;
		}

	private:
		static inline void *construct() noexcept
		{ return new T; }
		static inline void destruct(void *ptr) noexcept
		{ delete reinterpret_cast<T *>(ptr); }
	};

	template <typename T>
	class singleton_class_desc_t final : public base_class_desc_t<T>
	{
	public:
		using base_class_desc_t<T>::base_class_desc_t;
		using base_class_desc_t<T>::operator=;

		inline singleton_class_desc_t(std::string_view name) noexcept
			: base_class_desc_t<T>{name}
		{
			this->pHelper = &singleton_instance_helper<T>::singleton();
		}

		inline singleton_class_desc_t<T> &operator=(singleton_instance_helper<T> &helper) noexcept
		{
			this->pHelper = &helper;
			return *this;
		}

		template <typename ...Args>
		inline func_desc_t &func(Args &&...args) noexcept
		{
			func_desc_t &temp{base_class_desc_t<T>::func(std::forward<Args>(args)...)};
			using F = std::tuple_element_t<0, std::tuple<Args...>>;
			if constexpr(function_is_member_v<F>) {
				using R = function_return_t<F>;
				using A = function_args_tuple_t<F>;
				make_singleton<F, R>(temp, std::type_identity<A>{});
			}
			return temp;
		}

	private:
		template <typename F, typename R, typename ...Args>
		void make_singleton(func_desc_t &desc, std::type_identity<std::tuple<Args...>>) noexcept
		{
			if constexpr(singleton_instance_helper<T>::instance_func_available_v) {
				desc.m_flags &= ~gsdk::SF_MEMBER_FUNC;
			}

			if constexpr(function_is_va_v<F>) {
				desc.m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(func_desc_t::binding_member_singleton_va<R, T, Args...>);
			} else {
				desc.m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(func_desc_t::binding_member_singleton<R, T, Args...>);
			}
		}
	};
}

#include "vscript.tpp"
