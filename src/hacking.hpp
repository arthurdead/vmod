#pragma once

#include <string>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/mman.h>
#include "type_traits.hpp"
#include <memory>
#include <typeinfo>

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
	constexpr std::uintptr_t uninitialized_memory{0xbebebebe};

	inline __attribute__((__always_inline__)) void debugtrap() noexcept
	{
	#ifdef __clang__
		__builtin_debugtrap();
	#else
		__asm__ volatile("int $0x03");
	#endif
	}

	template <typename T>
	const std::string &demangle() noexcept;

	template <>
	const std::string &demangle<void>() noexcept = delete;

	extern const std::string &demangle(std::string_view mangled) noexcept;

	class generic_class final
	{
	public:
		[[noreturn]] inline void generic_function() noexcept
		{ __builtin_trap(); }
	private:
		generic_class() = delete;
		~generic_class() = delete;
		generic_class(const generic_class &) = delete;
		generic_class &operator=(const generic_class &) = delete;
		generic_class(generic_class &&) = delete;
		generic_class &operator=(generic_class &&) = delete;
	};

	using generic_object_t = generic_class;
	using generic_func_t = void(*)();
	using generic_func_va_t = void(*)(...);
	using generic_plain_mfp_t = void(__attribute__((__thiscall__)) *)(generic_class *);
	using generic_plain_mfp_va_t = void(*)(generic_class *, ...);
	using generic_mfp_t = void(generic_class::*)();
	using generic_mfp_va_t = void(generic_class::*)(...);

	static_assert(sizeof(&generic_class::generic_function) == sizeof(std::uint64_t));
	static_assert(alignof(&generic_class::generic_function) == alignof(std::uint64_t));

	template <typename R, typename C, typename ...Args>
	union alignas(std::uint64_t) mfp_internal_t
	{
		constexpr mfp_internal_t() noexcept = default;

		constexpr inline mfp_internal_t(std::nullptr_t) noexcept
			: func{nullptr}
		{
		}

		constexpr inline mfp_internal_t(std::uint64_t value_) noexcept
			: value{value_}
		{
		}

		constexpr inline mfp_internal_t(R(C::*func_)(Args...)) noexcept
			: func{func_}
		{
		}

		constexpr inline mfp_internal_t(R(__attribute__((__thiscall__)) *addr_)(C *, Args...)) noexcept
			: addr{addr_}, adjustor{0}
		{
		}

		constexpr inline mfp_internal_t(R(__attribute__((__thiscall__)) *addr_)(C *, Args...), std::size_t adjustor_) noexcept
			: addr{addr_}, adjustor{adjustor_}
		{
		}

		constexpr inline mfp_internal_t &operator=(std::uint64_t value_) noexcept
		{
			value = value_;
			return *this;
		}

		constexpr inline mfp_internal_t &operator=(R(C::*func_)(Args...)) noexcept
		{
			func = func_;
			return *this;
		}

		constexpr inline mfp_internal_t &operator=(R(__attribute__((__thiscall__)) *addr_)(C *, Args...)) noexcept
		{
			addr = addr_;
			adjustor = 0;
			return *this;
		}

		constexpr mfp_internal_t(mfp_internal_t &&) noexcept = default;
		constexpr mfp_internal_t &operator=(mfp_internal_t &&) noexcept = default;
		constexpr mfp_internal_t(const mfp_internal_t &) noexcept = default;
		constexpr mfp_internal_t &operator=(const mfp_internal_t &) noexcept = default;

		constexpr inline operator bool() const noexcept
		{ return addr; }
		constexpr inline bool operator!() const noexcept
		{ return !addr; }

		std::uint64_t value;
		struct {
			R(__attribute__((__thiscall__)) *addr)(C *, Args...);
			std::size_t adjustor;
		};
		R(C::*func)(Args...) {nullptr};
	};

	template <typename R, typename C, typename ...Args>
	union alignas(std::uint64_t) mfp_internal_va_t
	{
		constexpr mfp_internal_va_t() noexcept = default;

		constexpr inline mfp_internal_va_t(std::nullptr_t) noexcept
			: func{nullptr}
		{
		}

		constexpr inline mfp_internal_va_t(std::uint64_t value_) noexcept
			: value{value_}
		{
		}

		constexpr inline mfp_internal_va_t(R(C::*func_)(Args..., ...)) noexcept
			: func{func_}
		{
		}

		constexpr inline mfp_internal_va_t(R(*addr_)(C *, Args..., ...)) noexcept
			: addr{addr_}, adjustor{0}
		{
		}

		constexpr inline mfp_internal_va_t(R(*addr_)(C *, Args..., ...), std::size_t adjustor_) noexcept
			: addr{addr_}, adjustor{adjustor_}
		{
		}

		constexpr inline mfp_internal_va_t &operator=(std::uint64_t value_) noexcept
		{
			value = value_;
			return *this;
		}

		constexpr inline mfp_internal_va_t &operator=(R(C::*func_)(Args..., ...)) noexcept
		{
			func = func_;
			return *this;
		}

		constexpr inline mfp_internal_va_t &operator=(R(*addr_)(C *, Args..., ...)) noexcept
		{
			addr = addr_;
			adjustor = 0;
			return *this;
		}

		constexpr mfp_internal_va_t(mfp_internal_va_t &&) noexcept = default;
		constexpr mfp_internal_va_t &operator=(mfp_internal_va_t &&) noexcept = default;
		constexpr mfp_internal_va_t(const mfp_internal_va_t &) noexcept = default;
		constexpr mfp_internal_va_t &operator=(const mfp_internal_va_t &) noexcept = default;

		constexpr inline operator bool() const noexcept
		{ return addr; }
		constexpr inline bool operator!() const noexcept
		{ return !addr; }

		std::uint64_t value;
		struct {
			R(*addr)(C *, Args..., ...);
			std::size_t adjustor;
		};
		R(C::*func)(Args..., ...) {nullptr};
	};

	using generic_internal_mfp_t = mfp_internal_t<void, generic_class>;
	using generic_internal_mfp_va_t = mfp_internal_va_t<void, generic_class>;

	static_assert(sizeof(generic_internal_mfp_t) == sizeof(&generic_class::generic_function));
	static_assert(alignof(generic_internal_mfp_t) == alignof(&generic_class::generic_function));

	static_assert(sizeof(generic_internal_mfp_va_t) == sizeof(generic_internal_mfp_t));
	static_assert(alignof(generic_internal_mfp_va_t) == alignof(generic_internal_mfp_t));

	union mfp_or_func_t
	{
		constexpr mfp_or_func_t() noexcept = default;

		constexpr mfp_or_func_t(mfp_or_func_t &&) noexcept = default;
		constexpr mfp_or_func_t &operator=(mfp_or_func_t &&) noexcept = default;
		constexpr mfp_or_func_t(const mfp_or_func_t &) noexcept = default;
		constexpr mfp_or_func_t &operator=(const mfp_or_func_t &) noexcept = default;

		constexpr inline mfp_or_func_t(std::nullptr_t) noexcept
			: mfp{nullptr}
		{
		}

		generic_func_t func;
		generic_plain_mfp_t plain;
		generic_internal_mfp_t mfp{};

		constexpr inline operator bool() const noexcept
		{ return static_cast<bool>(mfp); }
		constexpr inline bool operator!() const noexcept
		{ return !mfp; }
	};

	using generic_vtable_t = generic_plain_mfp_t *;

	template <typename R, typename C, typename ...Args>
	constexpr inline mfp_internal_t<R, C, Args...> get_internal_mfp(R(C::*func)(Args...)) noexcept
	{
		mfp_internal_t<R, C, Args...> internal{func};
		return internal;
	}

	template <typename R, typename C, typename ...Args>
	constexpr inline mfp_internal_va_t<R, C, Args...> get_internal_mfp(R(C::*func)(Args..., ...)) noexcept
	{
		mfp_internal_va_t<R, C, Args...> internal{func};
		return internal;
	}

	template <typename R, typename C, typename ...Args>
	constexpr inline std::pair<R(__attribute__((__thiscall__)) *)(C *, Args...), std::size_t> mfp_to_func(R(C::*func)(Args...)) noexcept
	{
		mfp_internal_t<R, C, Args...> internal{func};
		return {internal.addr, internal.adjustor};
	}

	template <typename R, typename C, typename ...Args>
	constexpr inline std::pair<R(*)(C *, Args..., ...), std::size_t> mfp_to_func(R(C::*func)(Args..., ...)) noexcept
	{
		mfp_internal_va_t<R, C, Args...> internal{func};
		return {internal.addr, internal.adjustor};
	}

	template <typename R, typename C, typename ...Args>
	constexpr inline auto mfp_from_func(R(__attribute__((__thiscall__)) *addr)(C *, Args...)) noexcept -> R(C::*)(Args...)
	{
		mfp_internal_t<R, C, Args...> internal{addr};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	constexpr inline auto mfp_from_func(R(__attribute__((__thiscall__)) *addr)(C *, Args...), std::size_t adjustor) noexcept -> R(C::*)(Args...)
	{
		mfp_internal_t<R, C, Args...> internal{addr, adjustor};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	constexpr inline auto mfp_from_func(R(*addr)(C *, Args..., ...)) noexcept -> R(C::*)(Args..., ...)
	{
		mfp_internal_va_t<R, C, Args...> internal{addr};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	constexpr inline auto mfp_from_func(R(*addr)(C *, Args..., ...), std::size_t adjustor) noexcept -> R(C::*)(Args..., ...)
	{
		mfp_internal_va_t<R, C, Args...> internal{addr, adjustor};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	constexpr inline std::size_t vfunc_index(R(C::*func)(Args...)) noexcept
	{
		mfp_internal_t<R, C, Args...> internal{func};
		std::uintptr_t addr_value{0};
		if(std::is_constant_evaluated()) {
			addr_value = (__builtin_bit_cast(std::uint64_t, internal.func) & 0xFFFFFFFF);
		} else {
			addr_value = reinterpret_cast<std::uintptr_t>(internal.addr);
		}
		if(!(addr_value & 1)) {
			return static_cast<std::size_t>(-1);
		} else {
			return ((addr_value-1) / sizeof(generic_plain_mfp_t));
		}
	}

	template <typename C>
	inline __cxxabiv1::vtable_prefix *vtable_prefix_from_object(C *ptr) noexcept
	{
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
	#else
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-align"
	#endif
		return reinterpret_cast<__cxxabiv1::vtable_prefix *>(reinterpret_cast<unsigned char *>(*reinterpret_cast<generic_vtable_t *>(ptr)) - offsetof(__cxxabiv1::vtable_prefix, origin));
	#ifdef __clang__
		#pragma clang diagnostic pop
	#else
		#pragma GCC diagnostic pop
	#endif
	}

	template <typename C>
	inline __cxxabiv1::vtable_prefix *swap_prefix(C *ptr, __cxxabiv1::vtable_prefix *new_prefix) noexcept
	{
		__cxxabiv1::vtable_prefix *old_prefix{vtable_prefix_from_object<C>(ptr)};
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
	#else
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-align"
	#endif
		*reinterpret_cast<generic_vtable_t *>(ptr) = reinterpret_cast<generic_vtable_t>(const_cast<void **>(&new_prefix->origin));
	#ifdef __clang__
		#pragma clang diagnostic pop
	#else
		#pragma GCC diagnostic pop
	#endif
		return old_prefix;
	}

	inline generic_vtable_t vtable_from_prefix(__cxxabiv1::vtable_prefix *prefix) noexcept
	{
		return reinterpret_cast<generic_vtable_t>(const_cast<void **>(&prefix->origin));
	}

	template <typename C>
	inline generic_vtable_t vtable_from_object(C *ptr) noexcept
	{
		__cxxabiv1::vtable_prefix *prefix{vtable_prefix_from_object(ptr)};
		return vtable_from_prefix(prefix);
	}

	extern std::unique_ptr<__cxxabiv1::vtable_prefix> copy_prefix(__cxxabiv1::vtable_prefix *other, std::size_t num_funcs) noexcept;

	inline void *align(void *ptr, std::size_t alignment) noexcept
	{ return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(ptr) & ~(alignment - 1)); }
	constexpr inline std::size_t align(std::size_t value, std::size_t alignment) noexcept
	{ return (value & ~(alignment - 1)); }

	struct page_info final
	{
		inline page_info(void *ptr, std::size_t len) noexcept
		{
			std::size_t pagesize{static_cast<std::size_t>(sysconf(_SC_PAGESIZE))};
			start = align(ptr, pagesize);
			void *end{align(static_cast<unsigned char *>(ptr) + len, pagesize)};
			size = ((reinterpret_cast<std::uintptr_t>(start) - reinterpret_cast<std::uintptr_t>(end)) - pagesize);
		}

		inline void protect(int flags) noexcept
		{ mprotect(start, size, flags); }

	private:
		void *start;
		std::size_t size;

	private:
		page_info() = delete;
		page_info(const page_info &) = delete;
		page_info &operator=(const page_info &) = delete;
		page_info(page_info &&) = delete;
		page_info &operator=(page_info &&) = delete;
	};

	template <typename R, typename C, typename U, typename ...Args>
	inline auto swap_vfunc(generic_vtable_t vtable, R(U::*old_func)(Args...), R(*new_func)(C *, Args...)) noexcept -> R(C::*)(Args...)
	{
		static_assert(std::is_base_of_v<U, C>);
		std::size_t index{vfunc_index<R, U, Args...>(old_func)};
		if(index == static_cast<std::size_t>(-1)) {
			debugtrap();
			return nullptr;
		}
		generic_plain_mfp_t old_vfunc{vtable[index]};
		page_info func_page{vtable + ((index > 0) ? (index-1) : 0), sizeof(generic_plain_mfp_t)};
		func_page.protect(PROT_READ|PROT_WRITE|PROT_EXEC);
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		vtable[index] = reinterpret_cast<generic_plain_mfp_t>(new_func);
		auto mfp{mfp_from_func<R, C, Args...>(reinterpret_cast<R(__attribute__((__thiscall__)) *)(C *, Args...)>(old_vfunc))};
		#pragma GCC diagnostic pop
		return mfp;
	}

	template <typename T>
	class detour_base
	{
	public:
		inline detour_base() noexcept
		{
			std::memset(old_bytes, 0, sizeof(old_bytes));
		}

		inline ~detour_base() noexcept
		{ disable(); }

		void enable() noexcept;

		inline void disable() noexcept
		{
			if(!old_target) {
				return;
			}

		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
			std::memcpy(bytes, old_bytes, sizeof(old_bytes));
		}

	protected:
		void backup_bytes() noexcept;

		mfp_or_func_t old_target;
		mfp_or_func_t new_target;

		unsigned char old_bytes[1 + sizeof(std::uintptr_t)];

	private:
		detour_base(const detour_base &) = delete;
		detour_base &operator=(const detour_base &) = delete;
		detour_base(detour_base &&) = delete;
		detour_base &operator=(detour_base &&) = delete;
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
		detour() noexcept = default;

		inline void initialize(function_pointer_t<T> old_func_, function_pointer_t<T> new_func_) noexcept
		{
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wcast-function-type"
			this->old_target.mfp.addr = reinterpret_cast<generic_plain_mfp_t>(old_func_);
			this->new_target.func = reinterpret_cast<generic_func_t>(new_func_);
			#pragma GCC diagnostic pop
			this->old_target.mfp.adjustor = 0;

			this->backup_bytes();
		}

		template <typename ...Args>
		inline function_return_t<T> operator()(Args &&...args) noexcept
		{
			__detour_scope_enable<T> se{*this};
			return reinterpret_cast<function_pointer_t<T>>(this->old_target.func)(std::forward<Args>(args)...);
		}

	private:
		detour(const detour &) = delete;
		detour &operator=(const detour &) = delete;
		detour(detour &&) = delete;
		detour &operator=(detour &&) = delete;
	};

	template <typename T>
	class detour<T, true> final : public detour_base<T>
	{
	public:
		detour() noexcept = default;

		inline void initialize(function_pointer_t<T> old_func_, function_plain_pointer_t<T> new_func_) noexcept
		{
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wcast-function-type"
			this->old_target.mfp.func = reinterpret_cast<generic_mfp_t>(old_func_);
			this->new_target.plain = reinterpret_cast<generic_plain_mfp_t>(new_func_);
			#pragma GCC diagnostic pop

			this->backup_bytes();
		}

		template <typename ...Args>
		inline function_return_t<T> operator()(function_class_t<T> *obj, Args &&...args) noexcept
		{
			__detour_scope_enable<T> se{*this};
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wcast-function-type"
			return (obj->*reinterpret_cast<function_pointer_t<T>>(this->old_target.mfp.func))(std::forward<Args>(args)...);
			#pragma GCC diagnostic pop
		}

	private:
		detour(const detour &) = delete;
		detour &operator=(const detour &) = delete;
		detour(detour &&) = delete;
		detour &operator=(detour &&) = delete;
	};
}

#include "hacking.tpp"
