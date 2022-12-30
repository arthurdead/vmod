#pragma once

#include "../vscript.hpp"
#include "../function_desc.hpp"
#include <string>
#include <string_view>

namespace vmod::vscript
{
	namespace detail
	{
		template <typename T>
		class base_class_desc;
	}

	struct extra_class_desc final
	{
		template <typename T>
		friend class detail::base_class_desc;

		extra_class_desc() noexcept = default;

		inline const std::string &obfuscated_class_name() const noexcept
		{ return obfuscated_class_name_; }
		inline const std::string &obfuscated_name() const noexcept
		{ return obfuscated_name_; }
		inline const std::string &space() const noexcept
		{ return space_; }
		inline const std::string &name() const noexcept
		{ return name_; }

	private:
		bool singleton{false};
		std::string space_;
		std::string name_;
		std::string obfuscated_class_name_;
		std::string obfuscated_name_;

	private:
		extra_class_desc(const extra_class_desc &) = delete;
		extra_class_desc &operator=(const extra_class_desc &) = delete;
		extra_class_desc(extra_class_desc &&) = delete;
		extra_class_desc &operator=(extra_class_desc &&) = delete;
	};
}

namespace vmod::vscript::detail
{
	template <typename T>
	class base_class_desc : public gsdk::ScriptClassDesc_t
	{
	public:
		template <typename R, typename ...Args>
		inline function_desc &func(R(*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{
			function_desc temp{func, name, script_name};
			return static_cast<function_desc &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline function_desc &func(R(*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{
			function_desc temp{func, name, script_name};
			return static_cast<function_desc &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline function_desc &func(R(T::*func)(Args...), std::string_view name, std::string_view script_name) noexcept
		{
			function_desc temp{func, name, script_name};
			return static_cast<function_desc &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline function_desc &func(R(T::*func_)(Args...) const, std::string_view name, std::string_view script_name) noexcept
		{ return func<R, Args...>(reinterpret_cast<R(T::*)(Args...)>(func_), name, script_name); }

		template <typename R, typename ...Args>
		inline function_desc &func(R(T::*func)(Args..., ...), std::string_view name, std::string_view script_name) noexcept
		{
			function_desc temp{func, name, script_name};
			return static_cast<function_desc &>(m_FunctionBindings.emplace_back(std::move(temp)));
		}

		template <typename R, typename ...Args>
		inline function_desc &func(R(T::*func_)(Args..., ...) const, std::string_view name, std::string_view script_name) noexcept
		{ return func<R, Args...>(reinterpret_cast<R(T::*)(Args..., ...)>(func_), name, script_name); }

		inline const extra_class_desc &extra() const noexcept
		{ return extra_; }

		template <typename U>
		inline base_class_desc &base(base_class_desc<U> &other) noexcept
		{
			static_assert(std::is_base_of_v<U, T>);
			m_pBaseDesc = &other;
			return *this;
		}

	protected:
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
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
		#pragma GCC diagnostic pop

		base_class_desc(std::string_view path, bool obfuscate, bool singleton) noexcept;

		extra_class_desc extra_;

	private:
		base_class_desc() = delete;
		base_class_desc(const base_class_desc &) = delete;
		base_class_desc &operator=(const base_class_desc &) = delete;
		base_class_desc(base_class_desc &&) = delete;
		base_class_desc &operator=(base_class_desc &&) = delete;
	};
}

#include "base_class_desc.tpp"
