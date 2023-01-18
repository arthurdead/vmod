#pragma once

#include <type_traits>
#include <optional>
#include <utility>
#include <cstddef>

namespace vmod
{
	template <typename T>
	struct false_t : std::false_type
	{
	};

	template <typename T>
	struct is_optional : std::false_type
	{
	};

	template <typename T>
	struct is_optional<std::optional<T>> : std::true_type
	{
		using type = T;
	};

	namespace detail
	{
		template <typename T>
		struct function_traits;

		template <typename R, typename ...Args>
		struct function_traits<R(*)(Args...)>
		{
			using return_type = R;

			static constexpr bool va{false};
			static constexpr bool member{false};

			using args_tuple = std::tuple<Args...>;

			template <std::size_t i>
			struct arg
			{
				using type = std::tuple_element_t<i, args_tuple>;
			};

			using pointer_type = R(*)(Args...);
			using plain_pointer_type = pointer_type;
			using thiscall_pointer_type = R(__attribute__((__thiscall__)) *)(Args...);
		};

		template <typename R, typename C, typename ...Args>
		struct function_traits<R(C::*)(Args...)>
		{
			using return_type = R;

			using class_type = C;

			static constexpr bool va{false};
			static constexpr bool member{true};

			using args_tuple = std::tuple<Args...>;

			template <std::size_t i>
			struct arg
			{
				using type = std::tuple_element_t<i, args_tuple>;
			};

			using pointer_type = R(C::*)(Args...);
			using plain_pointer_type = R(*)(C *, Args...);
			using thiscall_pointer_type = R(__attribute__((__thiscall__)) *)(C *, Args...);
		};

		template <typename R, typename C, typename ...Args>
		struct function_traits<R(C::*)(Args...) const> : function_traits<R(C::*)(Args...)>
		{
			using class_type = const C;

			using pointer_type = R(C::*)(Args...) const;
			using plain_pointer_type = R(*)(const C *, Args...);
			using thiscall_pointer_type = R(__attribute__((__thiscall__)) *)(const C *, Args...);
		};

		template <typename R, typename ...Args>
		struct function_traits<R(*)(Args..., ...)> : function_traits<R(*)(Args...)>
		{
			static constexpr bool va{true};

			using pointer_type = R(*)(Args..., ...);
			using plain_pointer_type = pointer_type;
			using thiscall_pointer_type = plain_pointer_type;
		};

		template <typename R, typename C, typename ...Args>
		struct function_traits<R(C::*)(Args..., ...)> : function_traits<R(C::*)(Args...)>
		{
			static constexpr bool va{true};

			using pointer_type = R(C::*)(Args..., ...);
			using plain_pointer_type = R(*)(C *, Args..., ...);
			using thiscall_pointer_type = plain_pointer_type;
		};

		template <typename R, typename C, typename ...Args>
		struct function_traits<R(C::*)(Args..., ...) const> : function_traits<R(C::*)(Args...)>
		{
			static constexpr bool va{true};

			using pointer_type = R(C::*)(Args..., ...) const;
			using plain_pointer_type = R(*)(const C *, Args..., ...);
			using thiscall_pointer_type = plain_pointer_type;
		};

		template <typename R, typename ...Args>
		struct function_traits<R(*)(Args...) noexcept> : public function_traits<R(*)(Args...)>
		{
		};

		template <typename R, typename ...Args>
		struct function_traits<R(*)(Args..., ...) noexcept> : public function_traits<R(*)(Args..., ...)>
		{
		};

		template <typename R, typename C, typename ...Args>
		struct function_traits<R(C::*)(Args...) noexcept> : function_traits<R(C::*)(Args...)>
		{
		};

		template <typename R, typename C, typename ...Args>
		struct function_traits<R(C::*)(Args...) const noexcept> : function_traits<R(C::*)(Args...) const>
		{
		};

		template <typename R, typename C, typename ...Args>
		struct function_traits<R(C::*)(Args..., ...) noexcept> : function_traits<R(C::*)(Args..., ...)>
		{
		};

		template <typename R, typename C, typename ...Args>
		struct function_traits<R(C::*)(Args..., ...) const noexcept> : function_traits<R(C::*)(Args..., ...) const>
		{
		};

		template <typename R, typename ...Args>
		struct function_traits<R(Args...)> : function_traits<R(*)(Args...)>
		{
		};

		template <typename R, typename ...Args>
		struct function_traits<R(Args...) noexcept> : function_traits<R(*)(Args...) noexcept>
		{
		};

		template <typename R, typename ...Args>
		struct function_traits<R(Args..., ...)> : function_traits<R(*)(Args..., ...)>
		{
		};

		template <typename R, typename ...Args>
		struct function_traits<R(Args..., ...) noexcept> : function_traits<R(*)(Args..., ...) noexcept>
		{
		};
	}

	template <typename T>
	using function_return_t = typename detail::function_traits<T>::return_type;

	template <typename T>
	using function_args_tuple_t = typename detail::function_traits<T>::args_tuple;

	template <typename T>
	using function_pointer_t = typename detail::function_traits<T>::pointer_type;

	template <typename T>
	using function_thiscall_pointer_t = typename detail::function_traits<T>::thiscall_pointer_type;

	template <typename T>
	using function_plain_pointer_t = typename detail::function_traits<T>::plain_pointer_type;

	template <typename T>
	using function_class_t = typename detail::function_traits<T>::class_type;

	template <typename T>
	constexpr bool function_is_member_v{detail::function_traits<T>::member};

	template <typename T>
	constexpr bool function_is_va_v{detail::function_traits<T>::va};

	namespace detail
	{
		template <typename T>
		concept class_is_singleton =
			requires () { static_cast<T &(*)()>(&T::instance); }
		;
	}

	template <typename T>
	constexpr bool class_is_singleton_v{detail::class_is_singleton<T>};

	namespace detail
	{
		template <typename T, typename U>
		concept is_addable =
			requires () { std::declval<T>() += std::declval<U>(); }
		;
	}

	struct is_addable
	{
		template <typename T, typename U>
		static constexpr bool value{detail::is_addable<T, U>};
	};

	template <typename T, typename U>
	constexpr bool is_addable_v{detail::is_addable<T, U>};

	namespace detail
	{
		template <typename T, typename U>
		concept is_subtractable =
			requires () { std::declval<T>() -= std::declval<U>(); }
		;
	}

	struct is_subtractable
	{
		template <typename T, typename U>
		static constexpr bool value{detail::is_subtractable<T, U>};
	};

	template <typename T, typename U>
	constexpr bool is_subtractable_v{detail::is_subtractable<T, U>};

	namespace detail
	{
		template <typename T, typename U>
		concept is_multiplicable =
			requires () { std::declval<T>() *= std::declval<U>(); }
		;
	}

	struct is_multiplicable
	{
		template <typename T, typename U>
		static constexpr bool value{detail::is_multiplicable<T, U>};
	};

	template <typename T, typename U>
	constexpr bool is_multiplicable_v{detail::is_multiplicable<T, U>};

	namespace detail
	{
		template <typename T, typename U>
		concept is_divisible =
			requires () { std::declval<T>() /= std::declval<U>(); }
		;
	}

	struct is_divisible
	{
		template <typename T, typename U>
		static constexpr bool value{detail::is_divisible<T, U>};
	};

	template <typename T, typename U>
	constexpr bool is_divisible_v{detail::is_divisible<T, U>};
}
