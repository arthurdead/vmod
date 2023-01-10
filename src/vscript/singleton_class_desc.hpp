#pragma once

#include <type_traits>
#include "vscript.hpp"
#include "detail/base_class_desc.hpp"
#include <utility>
#include "../type_traits.hpp"

namespace vmod::vscript
{
	template <typename T>
	class singleton_class_desc final : public detail::base_class_desc<T>
	{
		class instance_helper final : public detail::base_class_desc<T>::instance_helper
		{
		public:
			static inline instance_helper &singleton() noexcept
			{
				static instance_helper singleton_;
				return singleton_;
			}

			inline void *GetProxied([[maybe_unused]] void *ptr) noexcept override final
			{
				if constexpr(class_is_singleton_v<T>) {
					return &T::instance();
				} else {
					return ptr;
				}
			}

			inline void *BindOnRead([[maybe_unused]] gsdk::HSCRIPT, [[maybe_unused]] void *ptr, [[maybe_unused]] const char *) noexcept override final
			{
				if constexpr(class_is_singleton_v<T>) {
					return &T::instance();
				} else {
					return ptr;
				}
			}
		};

	public:
		inline singleton_class_desc(std::string_view name, bool obfuscate = true) noexcept
			: detail::base_class_desc<T>{name, obfuscate, true}
		{
			this->pHelper = &instance_helper::singleton();
		}

		template <typename ...Args>
		inline function_desc &func(Args &&...args) noexcept
		{
			function_desc &temp{detail::base_class_desc<T>::func(std::forward<Args>(args)...)};
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
		void make_singleton(function_desc &desc, std::type_identity<std::tuple<Args...>>) noexcept
		{
			if constexpr(class_is_singleton_v<T>) {
				desc.m_flags &= ~static_cast<unsigned int>(gsdk::SF_MEMBER_FUNC);
			}

			if constexpr(function_is_va_v<F>) {
				desc.m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(function_desc::binding_member_singleton_va<R, T, Args...>);
			} else {
				desc.m_pfnBinding = static_cast<gsdk::ScriptBindingFunc_t>(function_desc::binding_member_singleton<R, T, Args...>);
			}
		}

	private:
		singleton_class_desc() = delete;
		singleton_class_desc(const singleton_class_desc &) = delete;
		singleton_class_desc &operator=(const singleton_class_desc &) = delete;
		singleton_class_desc(singleton_class_desc &&) = delete;
		singleton_class_desc &operator=(singleton_class_desc &&) = delete;
	};
}
