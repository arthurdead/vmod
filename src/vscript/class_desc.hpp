#pragma once

#include <type_traits>
#include "detail/base_class_desc.hpp"
#include <string_view>

namespace vmod::vscript
{
	template <typename T>
	class class_desc final : public detail::base_class_desc<T>
	{
	public:
		inline class_desc(std::string_view name, bool obfuscate = true) noexcept
			: detail::base_class_desc<T>{name, obfuscate, false}
		{
			this->pHelper = &detail::base_class_desc<T>::instance_helper::singleton();
		}

		inline void ctor(T *(*func)()) noexcept
		{
			this->m_pfnConstruct = reinterpret_cast<void *(*)()>(func);
		}

		inline void ctor() noexcept
		{
			static_assert(std::is_default_constructible_v<T>);
			this->m_pfnConstruct = static_cast<void *(*)()>(construct);
		}

		inline void dtor(void(*func)(T *)) noexcept
		{
			this->m_pfnDestruct = reinterpret_cast<void(*)(void *)>(func);
		}

		inline void dtor() noexcept
		{
			static_assert(std::is_destructible_v<T>);
			this->m_pfnDestruct = static_cast<void(*)(void *)>(destruct);
		}

	private:
		static inline void *construct() noexcept
		{ return new T; }
		static inline void destruct(void *ptr) noexcept
		{ delete static_cast<T *>(ptr); }

	private:
		class_desc() = delete;
		class_desc(const class_desc &) = delete;
		class_desc &operator=(const class_desc &) = delete;
		class_desc(class_desc &&) = delete;
		class_desc &operator=(class_desc &&) = delete;
	};
}
