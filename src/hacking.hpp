#pragma once

#include <string>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/mman.h>

#include <cxxabi.h>

#if __has_include(<tinfo.h>)
	#include <tinfo.h>
#endif

namespace vmod::__cxxabi
{
#if __has_include(<tinfo.h>)
	#define __VMOD_CXXABI_VTABLE_PREFIX_ALIGN alignas(__cxxabiv1::vtable_prefix)
#else
	#define __VMOD_CXXABI_VTABLE_PREFIX_ALIGN
#endif

	struct __VMOD_CXXABI_VTABLE_PREFIX_ALIGN vtable_prefix final
	{
		std::ptrdiff_t whole_object;
	#ifdef _GLIBCXX_VTABLE_PADDING
		std::ptrdiff_t padding1;
	#endif
		const __cxxabiv1::__class_type_info *whole_type;
	#ifdef _GLIBCXX_VTABLE_PADDING
		std::ptrdiff_t padding2;
	#endif
		const void *origin;
	};

#if __has_include(<tinfo.h>)
	static_assert(sizeof(vtable_prefix) == sizeof(__cxxabiv1::vtable_prefix));
	static_assert(alignof(vtable_prefix) == alignof(__cxxabiv1::vtable_prefix));
#endif
}

#if !__has_include(<tinfo.h>)
namespace __cxxabiv1
{
	using vtable_prefix = vmod::__cxxabi::vtable_prefix;
}
#endif

namespace vmod
{
	inline __attribute__((__always_inline__)) void debugtrap() noexcept
	{
	#ifdef __clang__
		__builtin_debugtrap();
	#else
		__asm__ volatile("int $0x03");
	#endif
	}

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

	template <typename T>
	using function_return_t = typename function_traits<T>::return_type;

	template <typename T>
	using function_args_tuple_t = typename function_traits<T>::args_tuple;

	template <typename T>
	using function_pointer_t = typename function_traits<T>::pointer_type;

	template <typename T>
	using function_thiscall_pointer_t = typename function_traits<T>::thiscall_pointer_type;

	template <typename T>
	using function_plain_pointer_t = typename function_traits<T>::plain_pointer_type;

	template <typename T>
	using function_class_t = typename function_traits<T>::class_type;

	template <typename T>
	constexpr bool function_is_member_v{function_traits<T>::member};

	template <typename T>
	constexpr bool function_is_va_v{function_traits<T>::va};

	template <typename T>
	const std::string &demangle() noexcept;

	class empty_class final
	{
	public:
		[[noreturn]] inline void empty_function() noexcept
		{ __builtin_trap(); }
	private:
		empty_class() = delete;
		~empty_class() = delete;
		empty_class(const empty_class &) = delete;
		empty_class &operator=(const empty_class &) = delete;
		empty_class(empty_class &&) = delete;
		empty_class &operator=(empty_class &&) = delete;
	};

	using generic_object_t = empty_class;
	using generic_func_t = void(*)();
	using generic_plain_mfp_t = void(__attribute__((__thiscall__)) *)(empty_class *);
	using generic_mfp_t = void(empty_class::*)();

	static_assert(sizeof(&empty_class::empty_function) == sizeof(std::uint64_t));
	static_assert(alignof(&empty_class::empty_function) == alignof(std::uint64_t));

	template <typename R, typename C, typename ...Args>
	union alignas(std::uint64_t) mfp_internal_t
	{
		inline mfp_internal_t() noexcept
			: func{nullptr}
		{
		}

		inline mfp_internal_t(std::uint64_t value_) noexcept
			: value{value_}
		{
		}

		inline mfp_internal_t(R(C::*func_)(Args...)) noexcept
			: func{func_}
		{
		}

		inline mfp_internal_t(R(__attribute__((__thiscall__)) *addr_)(C *, Args...)) noexcept
			: addr{addr_}, adjustor{0}
		{
		}

		inline mfp_internal_t(R(__attribute__((__thiscall__)) *addr_)(C *, Args...), std::size_t adjustor_) noexcept
			: addr{addr_}, adjustor{adjustor_}
		{
		}

		inline mfp_internal_t &operator=(std::uint64_t value_) noexcept
		{
			value = value_;
			return *this;
		}

		inline mfp_internal_t &operator=(R(C::*func_)(Args...)) noexcept
		{
			func = func_;
			return *this;
		}

		inline mfp_internal_t &operator=(R(__attribute__((__thiscall__)) *addr_)(C *, Args...)) noexcept
		{
			addr = addr_;
			adjustor = 0;
			return *this;
		}

		inline operator bool() const noexcept
		{ return func; }
		inline bool operator!() const noexcept
		{ return !func; }

		std::uint64_t value;
		struct {
			R(__attribute__((__thiscall__)) *addr)(C *, Args...);
			std::size_t adjustor;
		};
		R(C::*func)(Args...);
	};

	template <typename R, typename C, typename ...Args>
	union alignas(std::uint64_t) mfp_internal_va_t
	{
		inline mfp_internal_va_t() noexcept
			: func{nullptr}
		{
		}

		inline mfp_internal_va_t(std::uint64_t value_) noexcept
			: value{value_}
		{
		}

		inline mfp_internal_va_t(R(C::*func_)(Args..., ...)) noexcept
			: func{func_}
		{
		}

		inline mfp_internal_va_t(R(*addr_)(C *, Args..., ...)) noexcept
			: addr{addr_}, adjustor{0}
		{
		}

		inline mfp_internal_va_t(R(*addr_)(C *, Args..., ...), std::size_t adjustor_) noexcept
			: addr{addr_}, adjustor{adjustor_}
		{
		}

		inline mfp_internal_va_t &operator=(std::uint64_t value_) noexcept
		{
			value = value_;
			return *this;
		}

		inline mfp_internal_va_t &operator=(R(C::*func_)(Args..., ...)) noexcept
		{
			func = func_;
			return *this;
		}

		inline mfp_internal_va_t &operator=(R(*addr_)(C *, Args..., ...)) noexcept
		{
			addr = addr_;
			adjustor = 0;
			return *this;
		}

		inline operator bool() const noexcept
		{ return func; }
		inline bool operator!() const noexcept
		{ return !func; }

		std::uint64_t value;
		struct {
			R(*addr)(C *, Args..., ...);
			std::size_t adjustor;
		};
		R(C::*func)(Args..., ...);
	};

	using generic_internal_mfp_t = mfp_internal_t<void, empty_class>;
	using generic_internal_mfp_va_t = mfp_internal_va_t<void, empty_class>;

	static_assert(sizeof(generic_internal_mfp_t) == sizeof(&empty_class::empty_function));
	static_assert(alignof(generic_internal_mfp_t) == alignof(&empty_class::empty_function));

	static_assert(sizeof(generic_internal_mfp_va_t) == sizeof(generic_internal_mfp_t));
	static_assert(alignof(generic_internal_mfp_va_t) == alignof(generic_internal_mfp_t));

	using generic_vtable_t = generic_plain_mfp_t *;

	template <typename R, typename C, typename ...Args>
	inline mfp_internal_t<R, C, Args...> get_internal_mfp(R(C::*func)(Args...)) noexcept
	{
		mfp_internal_t<R, C, Args...> internal{func};
		return internal;
	}

	template <typename R, typename C, typename ...Args>
	inline std::pair<R(__attribute__((__thiscall__)) *)(C *, Args...), std::size_t> mfp_to_func(R(C::*func)(Args...)) noexcept
	{
		mfp_internal_t<R, C, Args...> internal{func};
		return {internal.addr, internal.adjustor};
	}

	template <typename R, typename C, typename ...Args>
	inline std::pair<R(*)(C *, Args..., ...), std::size_t> mfp_to_func(R(C::*func)(Args..., ...)) noexcept
	{
		mfp_internal_va_t<R, C, Args...> internal{func};
		return {internal.addr, internal.adjustor};
	}

	template <typename R, typename C, typename ...Args>
	inline auto mfp_from_func(R(__attribute__((__thiscall__)) *addr)(C *, Args...)) noexcept -> R(C::*)(Args...)
	{
		mfp_internal_t<R, C, Args...> internal{addr};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	inline auto mfp_from_func(R(*addr)(C *, Args..., ...)) noexcept -> R(C::*)(Args..., ...)
	{
		mfp_internal_va_t<R, C, Args...> internal{addr};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	inline auto mfp_from_func(R(__attribute__((__thiscall__)) *addr)(C *, Args...), std::size_t adjustor) noexcept -> R(C::*)(Args...)
	{
		mfp_internal_t<R, C, Args...> internal{addr, adjustor};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	inline auto mfp_from_func(R(*addr)(C *, Args..., ...), std::size_t adjustor) noexcept -> R(C::*)(Args..., ...)
	{
		mfp_internal_va_t<R, C, Args...> internal{addr, adjustor};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	inline std::size_t vfunc_index(R(C::*func)(Args...)) noexcept
	{
		mfp_internal_t<R, C, Args...> internal{func};
		std::uintptr_t addr_value{reinterpret_cast<std::uintptr_t>(internal.addr)};
		if(!(addr_value & 1)) {
			return static_cast<std::size_t>(-1);
		}
		return ((addr_value-1) / sizeof(generic_plain_mfp_t));
	}

	template <typename C>
	inline generic_plain_mfp_t *vtable_from_object(C *ptr) noexcept
	{
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
	#endif
		return *reinterpret_cast<generic_plain_mfp_t **>(ptr);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
	}

	inline void *align(void *ptr, std::size_t alignment) noexcept
	{ return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(ptr) & ~(alignment - 1)); }

	struct page_info final
	{
		inline page_info(void *ptr, std::size_t len) noexcept
		{
			std::size_t pagesize{static_cast<std::size_t>(sysconf(_SC_PAGESIZE))};
			start = align(ptr, pagesize);
			void *end{align(reinterpret_cast<unsigned char *>(ptr) + len, pagesize)};
			size = ((reinterpret_cast<std::uintptr_t>(start) - reinterpret_cast<std::uintptr_t>(end)) - pagesize);
		}

		inline void protect(int flags) noexcept
		{ mprotect(start, size, flags); }

	private:
		void *start;
		std::size_t size;
	};

	template <typename R, typename C, typename U, typename ...Args>
	inline auto swap_vfunc(C *ptr, R(U::*old_func)(Args...), R(*new_func)(C *, Args...)) noexcept -> R(C::*)(Args...)
	{
		std::size_t index{vfunc_index(old_func)};
		if(index == static_cast<std::size_t>(-1)) {
			debugtrap();
			return nullptr;
		}
		generic_plain_mfp_t *vtable{vtable_from_object<C>(ptr)};
		generic_plain_mfp_t old_vfunc{vtable[index]};
		page_info func_page{vtable + ((index > 0) ? (index-1) : 0), sizeof(generic_plain_mfp_t)};
		func_page.protect(PROT_READ|PROT_WRITE|PROT_EXEC);
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wcast-function-type"
	#endif
		vtable[index] = reinterpret_cast<generic_plain_mfp_t>(new_func);
		auto mfp{mfp_from_func<R, C, Args...>(reinterpret_cast<R(__attribute__((__thiscall__)) *)(C *, Args...)>(old_vfunc))};
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		return mfp;
	}

	template <typename T>
	class detour_base
	{
	public:
		inline detour_base() noexcept
		{
		}

		inline ~detour_base() noexcept
		{
			if(old_func) {
				disable();
			}
		}

		void enable() noexcept;

		inline void disable() noexcept
		{
			unsigned char *bytes{reinterpret_cast<unsigned char *>(old_func)};
			std::memcpy(bytes, old_bytes, sizeof(old_bytes));
		}

	protected:
		void backup_bytes() noexcept;

		union {
			generic_func_t old_func;
			generic_internal_mfp_t old_mfp;
		};

		union {
			generic_func_t new_func;
			generic_plain_mfp_t new_mfp;
		};

		unsigned char old_bytes[1 + sizeof(std::uintptr_t)];
	};

	template <typename T>
	struct __detour_scope_enable final {
		inline __detour_scope_enable(detour_base<T> &det_) noexcept
			: det{det_} {
			det.disable();
		}
		inline ~__detour_scope_enable() noexcept {
			det.enable();
		}
		detour_base<T> &det;
	};

	template <typename T, bool = function_is_member_v<T>>
	class detour;

	template <typename T>
	class detour<T, false> final : public detour_base<T>
	{
	public:
		inline void initialize(function_pointer_t<T> old_func_, function_pointer_t<T> new_func_) noexcept
		{
		#ifdef __clang__
			#pragma clang diagnostic push
			#pragma clang diagnostic ignored "-Wcast-function-type"
		#endif
			this->old_mfp.addr = reinterpret_cast<generic_plain_mfp_t>(old_func_);
			this->new_func = reinterpret_cast<generic_func_t>(new_func_);
		#ifdef __clang__
			#pragma clang diagnostic pop
		#endif
			this->old_mfp.adjustor = 0;

			this->backup_bytes();
		}

		template <typename ...Args>
		inline function_return_t<T> operator()(Args &&...args) noexcept
		{
			__detour_scope_enable<T> se{*this};
			return reinterpret_cast<function_pointer_t<T>>(this->old_func)(std::forward<Args>(args)...);
		}
	};

	template <typename T>
	class detour<T, true> final : public detour_base<T>
	{
	public:
		inline void initialize(function_pointer_t<T> old_func_, function_plain_pointer_t<T> new_func_) noexcept
		{
		#ifdef __clang__
			#pragma clang diagnostic push
			#pragma clang diagnostic ignored "-Wcast-function-type"
		#endif
			this->old_mfp.func = reinterpret_cast<generic_mfp_t>(old_func_);
			this->new_mfp = reinterpret_cast<generic_plain_mfp_t>(new_func_);
		#ifdef __clang__
			#pragma clang diagnostic pop
		#endif

			this->backup_bytes();
		}

		template <typename ...Args>
		inline function_return_t<T> operator()(function_class_t<T> *obj, Args &&...args) noexcept
		{
			__detour_scope_enable<T> se{*this};
			return (obj->*reinterpret_cast<function_pointer_t<T>>(this->old_mfp.func))(std::forward<Args>(args)...);
		}
	};
}

#include "hacking.tpp"
