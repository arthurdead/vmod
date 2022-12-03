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
	void initialize_variant_value(gsdk::ScriptVariant_t &, T) noexcept;

	template <typename T>
	gsdk::ScriptVariant_t value_to_variant(T value) noexcept;
}

#include "vscript_variant.tpp"

namespace vmod
{
	template <typename T>
	inline void value_to_variant(gsdk::ScriptVariant_t &var, T value) noexcept
	{
		var.m_type = static_cast<short>(type_to_field<std::decay_t<T>>());
		var.m_flags = 0;
		initialize_variant_value<std::decay_t<T>>(var, std::forward<T>(value));
	}
}

namespace vmod
{
	inline void null_variant(gsdk::ScriptVariant_t &var) noexcept
	{
		std::memset(var.unk1, 0, sizeof(gsdk::ScriptVariant_t::unk1));
		var.m_type = gsdk::FIELD_VOID;
		var.m_hScript = nullptr;
		var.m_flags = 0;
	}

	extern void free_variant_hscript(gsdk::ScriptVariant_t &var) noexcept;

	class alignas(gsdk::ScriptVariant_t) script_variant_t final : public gsdk::ScriptVariant_t
	{
	public:
		inline script_variant_t() noexcept
		{
			m_type = gsdk::FIELD_VOID;
			m_hScript = gsdk::INVALID_HSCRIPT;
			m_flags = 0;
			std::memset(unk1, 0, sizeof(unk1));
		}

		inline ~script_variant_t() noexcept
		{ free(); }

		inline script_variant_t(script_variant_t &&other) noexcept
		{ operator=(std::move(other)); }

		inline script_variant_t &operator=(script_variant_t &&other) noexcept
		{
			m_type = other.m_type;
			m_hScript = other.m_hScript;
			m_flags = other.m_flags;
			std::memcpy(unk1, other.unk1, sizeof(unk1));
			std::memset(other.unk1, 0, sizeof(unk1));
			return *this;
		}

		script_variant_t(const script_variant_t &) = delete;
		script_variant_t &operator=(const script_variant_t &) = delete;

		template <typename T>
		inline script_variant_t(T value) noexcept
		{
			std::memset(unk1, 0, sizeof(unk1));
			value_to_variant<T>(*this, std::forward<T>(value));
		}

		template <typename T>
		inline script_variant_t &operator=(T value) noexcept
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

	private:
		inline void free() noexcept
		{
			if(m_flags & gsdk::SV_FREE) {
				switch(m_type) {
					case gsdk::FIELD_VECTOR: {
						delete m_pVector;
					} break;
					case gsdk::FIELD_CSTRING: {
						std::free(const_cast<char *>(m_pszString));
					} break;
					case gsdk::FIELD_HSCRIPT: {
						free_variant_hscript(*this);
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

	template <typename T>
	inline gsdk::ScriptVariant_t value_to_variant(T value) noexcept
	{
		script_variant_t var;
		value_to_variant<T>(var, std::forward<T>(value));
		return var;
	}

	template <typename T>
	class singleton_class_desc_t;

	class alignas(gsdk::ScriptFunctionBinding_t) func_desc_t : public gsdk::ScriptFunctionBinding_t
	{
	public:
		func_desc_t() noexcept = default;

		template <typename R, typename ...Args>
		inline void initialize(R(*func)(Args...), std::string_view name, std::string_view rename = {}) noexcept
		{
			std::memset(unk1, 0, sizeof(unk1));
			initialize_static<R, Args...>(func, name, rename);
		}

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
			m_flags = other.m_flags;
			std::memcpy(unk1, other.unk1, sizeof(unk1));
			std::memset(other.unk1, 0, sizeof(unk1));
			return *this;
		}

		template <typename M>
		func_desc_t(M, std::string_view) = delete;

		template <typename T>
		friend class class_desc_t;

		template <typename R, typename C, typename ...Args>
		inline func_desc_t(R(C::*func)(Args...), std::string_view name, std::string_view rename) noexcept
		{
			std::memset(unk1, 0, sizeof(unk1));
			initialize_member<R, C, Args...>(func, name, rename);
		}

		template <typename R, typename ...Args>
		inline func_desc_t(R(*func)(Args...), std::string_view name, std::string_view rename) noexcept
		{ initialize(func, name, rename); }

		template <typename R, typename C, typename ...Args>
		void initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view rename);

		template <typename R, typename ...Args>
		void initialize_static(R(*func)(Args...), std::string_view name, std::string_view rename);

		template <typename R, typename ...Args>
		void initialize_shared(std::string_view name, std::string_view rename);

		template <typename R, typename C, typename ...Args>
		static bool binding(gsdk::ScriptFunctionBindingStorageType_t binding_func, [[maybe_unused]] char unkarg1[sizeof(int)], void *obj, gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept;

		template <typename R, typename C, typename ...Args, std::size_t ...I>
		static R call_impl(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *ctx, gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept;

		template <typename R, typename C, typename ...Args>
		static inline R call(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *ctx, gsdk::ScriptVariant_t *args_var) noexcept
		{ return call_impl<R, C, Args...>(binding_func, ctx, args_var, std::make_index_sequence<sizeof...(Args)>()); }
	};

	static_assert(sizeof(func_desc_t) == sizeof(gsdk::ScriptFunctionBinding_t));
	static_assert(alignof(func_desc_t) == alignof(gsdk::ScriptFunctionBinding_t));

	template <typename T>
	class alignas(gsdk::ScriptClassDesc_t) class_desc_t : public gsdk::ScriptClassDesc_t
	{
	public:
		class_desc_t(const class_desc_t &) = delete;
		class_desc_t &operator=(const class_desc_t &) = delete;

		class instance_helper final : public gsdk::IScriptInstanceHelper
		{
		public:
			virtual ~instance_helper() noexcept = default;

			inline void *GetProxied(void *ptr) noexcept override final
			{ return ptr; }

			inline bool ToString(void *ptr, char *buff, int siz) noexcept override final
			{
				const std::string &name{demangle<T>()};
				std::snprintf(buff, static_cast<std::size_t>(siz), "(%s : %p)", name.c_str(), ptr);
				return true;
			}

			inline void *BindOnRead([[maybe_unused]] gsdk::HSCRIPT instance, void *ptr, [[maybe_unused]] const char *id) noexcept override final
			{ return ptr; }

			static inline instance_helper &singleton() noexcept
			{
				static instance_helper singleton_;
				return singleton_;
			}
		};

		inline class_desc_t(class_desc_t &&other) noexcept
			: gsdk::ScriptClassDesc_t{}
		{ operator=(std::move(other)); }

		inline class_desc_t &operator=(class_desc_t &&other) noexcept
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

		class_desc_t(std::string_view name) noexcept;

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(*func)(Args...), std::string_view name, std::string_view rename = {}) noexcept
		{
			func_desc_t temp{func, name, rename};
			return static_cast<func_desc_t &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(T::*func)(Args...), std::string_view name, std::string_view rename = {}) noexcept
		{
			func_desc_t temp{func, name, rename};
			return static_cast<func_desc_t &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(T::*func_)(Args...) const, std::string_view name, std::string_view rename = {}) noexcept
		{ return func<R, Args...>(reinterpret_cast<R(T::*)(Args...)>(func_), name, rename); }

		inline class_desc_t &dtor(void(*func)(T *)) noexcept
		{
			m_pfnDestruct = reinterpret_cast<void(*)(void *)>(func);
			return *this;
		}

		inline class_desc_t &dtor() noexcept
		{
			static_assert(std::is_destructible_v<T>);
			m_pfnDestruct = static_cast<void(*)(void *)>(destruct);
			return *this;
		}

		inline class_desc_t &ctor(T *(*func)()) noexcept
		{
			m_pfnConstruct = reinterpret_cast<void *(*)()>(func);
			return *this;
		}

		inline class_desc_t &ctor() noexcept
		{
			static_assert(std::is_default_constructible_v<T>);
			m_pfnConstruct = static_cast<void *(*)()>(construct);
			return *this;
		}

	private:
		static inline void *construct() noexcept
		{ return new T; }
		static inline void destruct(void *ptr) noexcept
		{ delete reinterpret_cast<T *>(ptr); }
	};

	static_assert(sizeof(class_desc_t<empty_class>) == sizeof(gsdk::ScriptClassDesc_t));
	static_assert(alignof(class_desc_t<empty_class>) == alignof(gsdk::ScriptClassDesc_t));
}

#include "vscript.tpp"
