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

		inline script_variant_t(gsdk::HSCRIPT value) noexcept
		{
			m_type = gsdk::FIELD_HSCRIPT;
			m_hScript = value;
			m_flags = 0;
			std::memset(unk1, 0, sizeof(unk1));
		}

		template <typename T>
		inline script_variant_t(T &&value) noexcept
		{
			std::memset(unk1, 0, sizeof(unk1));
			value_to_variant<T>(*this, std::forward<T>(value));
		}
	};

	static_assert(sizeof(script_variant_t) == sizeof(gsdk::ScriptVariant_t));
	static_assert(alignof(script_variant_t) == alignof(gsdk::ScriptVariant_t));

	template <typename T>
	T variant_to_value(const gsdk::ScriptVariant_t &) noexcept = delete;

	template <typename T>
	void initialize_variant_value(gsdk::ScriptVariant_t &, T) noexcept = delete;

	inline void null_variant(gsdk::ScriptVariant_t &var) noexcept
	{
		std::memset(var.unk1, 0, sizeof(gsdk::ScriptVariant_t::unk1));
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
		script_variant_t var;
		value_to_variant(var, std::forward<T>(value));
		return var;
	}

	template <typename T>
	class singleton_class_desc_t;

	class alignas(gsdk::ScriptFunctionBinding_t) func_desc_t : public gsdk::ScriptFunctionBinding_t
	{
	private:
		func_desc_t() = delete;

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

		template <typename R, typename C, typename ...Args>
		void initialize_member(R(C::*func)(Args...), std::string_view name, std::string_view rename);

		template <typename R, typename ...Args>
		void initialize_shared(std::string_view name, std::string_view rename);

		template <typename R, typename C, typename ...Args>
		static bool binding(gsdk::ScriptFunctionBindingStorageType_t binding_func, [[maybe_unused]] char unkarg1[sizeof(int)], void *obj, gsdk::ScriptVariant_t *args_var, int num_args, gsdk::ScriptVariant_t *ret_var) noexcept;

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
	class script_instance_helper final : public gsdk::IScriptInstanceHelper
	{
	public:
		virtual ~script_instance_helper() noexcept = default;

		inline void *GetProxied(void *ptr) noexcept override final
		{ return ptr; }

		bool ToString(void *ptr, char *buff, int siz) noexcept override
		{
			const std::string &name{demangle<T>()};
			std::snprintf(buff, static_cast<std::size_t>(siz), "(%s)%p", name.c_str(), ptr);
			return true;
		}

		void *BindOnRead([[maybe_unused]] gsdk::HSCRIPT instance, void *old_ptr, [[maybe_unused]] const char *id) noexcept override final
		{ return old_ptr; }

		static inline script_instance_helper &singleton() noexcept
		{
			static script_instance_helper singleton_;
			return singleton_;
		}
	};

	template <typename T>
	class alignas(gsdk::ScriptClassDesc_t) class_desc_t : public gsdk::ScriptClassDesc_t
	{
	public:
		class_desc_t(const class_desc_t &) = delete;
		class_desc_t &operator=(const class_desc_t &) = delete;

		inline class_desc_t(class_desc_t &&other) noexcept
			: gsdk::ScriptClassDesc_t{}
		{
			operator=(std::move(other));
		}

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

		inline class_desc_t() noexcept
			: class_desc_t{std::string_view{}}
		{
		}

		template <typename R, typename ...Args>
		inline func_desc_t &func(R(T::*func)(Args...), std::string_view name, std::string_view rename = {}) noexcept
		{
			func_desc_t temp{func, name, rename};
			return static_cast<func_desc_t &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}
	};
}

#include "vscript.tpp"
