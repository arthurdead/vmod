#pragma once

#include "gsdk/vscript/vscript.hpp"
#include <string_view>
#include <utility>
#include "hacking.hpp"

namespace vmod
{
	template <typename T>
	constexpr gsdk::ScriptDataType_t type_to_field() noexcept = delete;

	template <typename T>
	T variant_to_value(const gsdk::ScriptVariant_t &) noexcept = delete;

	template <typename T>
	void initialize_variant_value(gsdk::ScriptVariant_t &, T) noexcept = delete;

	inline void null_variant(gsdk::ScriptVariant_t &var) noexcept
	{
		var.m_type = gsdk::FIELD_VOID;
		var.m_hScript = nullptr;
		var.m_flags = 0;
	}

	template <typename T>
	inline void value_to_variant(gsdk::ScriptVariant_t &var, T &&value) noexcept
	{
		var.m_type = static_cast<short>(type_to_field<T>());
		var.m_flags = 0;
		initialize_variant_value(var, std::forward<T>(value));
	}

	template <typename T>
	inline gsdk::ScriptVariant_t value_to_variant(T &&value) noexcept
	{
		gsdk::ScriptVariant_t var;
		value_to_variant(var, std::forward<T>(value));
		return var;
	}

	class alignas(gsdk::ScriptVariant_t) script_variant_t final : public gsdk::ScriptVariant_t
	{
	public:
		inline script_variant_t() noexcept
		{
			m_type = gsdk::FIELD_VOID;
			m_hScript = gsdk::INVALID_HSCRIPT;
			m_flags = 0;
		}

		inline script_variant_t(gsdk::ScriptVariant_t &&other) noexcept
		{ operator=(std::move(other)); }
		inline script_variant_t &operator=(gsdk::ScriptVariant_t &&other) noexcept
		{
			m_type = other.m_type;
			m_hScript = other.m_hScript;
			m_flags = other.m_flags;
			return *this;
		}
		script_variant_t(const gsdk::ScriptVariant_t &) = delete;
		script_variant_t &operator=(const gsdk::ScriptVariant_t &) = delete;

		script_variant_t(script_variant_t &&) noexcept = default;
		script_variant_t &operator=(script_variant_t &&) = default;
		script_variant_t(const script_variant_t &) = delete;
		script_variant_t &operator=(const script_variant_t &) = delete;

		template <typename T>
		inline script_variant_t(T &&value)
		{ value_to_variant<T>(*this, std::forward<T>(value)); }
	};

	static_assert(sizeof(script_variant_t) == sizeof(gsdk::ScriptVariant_t));
	static_assert(alignof(script_variant_t) == alignof(gsdk::ScriptVariant_t));

	template <typename T>
	class singleton_class_desc_t;

	class alignas(gsdk::ScriptFunctionBinding_t) func_desc_t : public gsdk::ScriptFunctionBinding_t
	{
	private:
		template <typename M>
		func_desc_t(M, std::string_view) = delete;

		template <typename T>
		friend class singleton_class_desc_t;

		template <typename R, typename C, typename ...Args>
		inline func_desc_t(R(C::*func)(Args...), std::string_view name, std::string_view rename) noexcept
		{ initialize_member<R, C, Args...>(func, name, rename); }

		template <typename R, typename C, typename ...Args>
		void initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view rename);

		template <typename R, typename ...Args>
		void initialize_shared(std::string_view name, std::string_view rename);

		template <typename R, typename C, typename ...Args, std::size_t ...I>
		static inline R call_impl(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *ctx, gsdk::ScriptVariant_t *args_var, std::index_sequence<I...>) noexcept
		{
			if constexpr(sizeof...(Args) == 0) {
				return (static_cast<C *>(ctx)->*mfp_from_func<R, C, Args...>(reinterpret_cast<generic_func_t>(binding_func)))();
			} else {
				return (static_cast<C *>(ctx)->*mfp_from_func<R, C, Args...>(reinterpret_cast<generic_func_t>(binding_func)))(variant_to_value<Args>(args_var[I])...);
			}
		}

		template <typename R, typename C, typename ...Args>
		static inline R call(gsdk::ScriptFunctionBindingStorageType_t binding_func, void *ctx, gsdk::ScriptVariant_t *args_var) noexcept
		{ return call_impl<R, C, Args...>(binding_func, ctx, args_var, std::make_index_sequence<sizeof...(Args)>()); }
	};

	static_assert(sizeof(func_desc_t) == sizeof(gsdk::ScriptFunctionBinding_t));
	static_assert(alignof(func_desc_t) == alignof(gsdk::ScriptFunctionBinding_t));

	template <typename T>
	class singleton_class_desc_t final : public gsdk::ScriptClassDesc_t
	{
	public:
		singleton_class_desc_t(std::string_view classname, std::string_view name = {}) noexcept;

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(T::*func)(Args...), std::string_view name, std::string_view rename = {}) noexcept
		{ return static_cast<func_desc_t &>(m_FunctionBindings.emplace_back(func_desc_t{func, name, rename})); }
	};
}

#include "vscript.tpp"
