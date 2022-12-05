#pragma once

#include <string>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <cstring>
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
	inline __attribute__((__always_inline__)) void debugbreak() noexcept
	{ __asm__ volatile("int $0x03"); }

	template <typename T>
	struct function_traits;

	template <typename R, typename ...Args>
	struct function_traits<R(*)(Args...)>
	{
		using return_type = R;

		template <std::size_t i>
		struct arg
		{
			using type = std::tuple_element_t<i, std::tuple<Args...>>;
		};

		using pointer_type = R(*)(Args...);
	};

	template <typename R, typename C, typename ...Args>
	struct function_traits<R(C::*)(Args...)>
	{
		using return_type = R;
		using class_type = C;

		template <std::size_t i>
		struct arg
		{
			using type = std::tuple_element_t<i, std::tuple<Args...>>;
		};

		using pointer_type = R(C::*)(Args...);
	};

	template <typename R, typename ...Args>
	struct function_traits<R(Args...)> : function_traits<R(*)(Args...)>
	{
	};

	template <typename T>
	using function_return_t = typename function_traits<T>::return_type;

	template <typename T>
	using function_pointer_t = typename function_traits<T>::pointer_type;

	template <typename T>
	const std::string &demangle() noexcept
	{
		static std::string buffer;

		if(buffer.empty()) {
			const char *mangled{typeid(T).name()};

			int status;
			std::size_t allocated;
			char *temp_buffer{__cxxabiv1::__cxa_demangle(mangled, nullptr, &allocated, &status)};
			if(status == 0 && allocated > 0 && temp_buffer) {
				buffer = temp_buffer;
			}
			if(temp_buffer) {
				std::free(temp_buffer);
			}
		}

		return buffer;
	}

	class empty_class final
	{
	public:
		void empty_function() {}
	};

	using generic_func_t = void(*)();
	using generic_mfp_t = void(empty_class::*)();

	static_assert(sizeof(&empty_class::empty_function) == sizeof(std::uint64_t));
	static_assert(alignof(&empty_class::empty_function) == alignof(std::uint64_t));

	template <typename R, typename C, typename ...Args>
	union alignas(std::uint64_t) mfp_internal_t
	{
		mfp_internal_t() noexcept = default;

		inline mfp_internal_t(R(C::*func_)(Args...)) noexcept
			: func{func_}
		{
		}

		inline mfp_internal_t(generic_func_t addr_) noexcept
			: addr{addr_}, adjustor{0}
		{
		}

		inline mfp_internal_t(generic_func_t addr_, std::size_t adjustor_) noexcept
			: addr{addr_}, adjustor{adjustor_}
		{
		}

		struct {
			generic_func_t addr;
			std::size_t adjustor;
		};
		R(C::*func)(Args...);
	};

	static_assert(sizeof(mfp_internal_t<void, empty_class>) == sizeof(&empty_class::empty_function));
	static_assert(alignof(mfp_internal_t<void, empty_class>) == alignof(&empty_class::empty_function));

	using generic_vtable_t = generic_func_t *;

	template <typename R, typename C, typename ...Args>
	inline std::pair<generic_func_t, std::size_t> mfp_to_func(R (C::*func)(Args...)) noexcept
	{
		mfp_internal_t<R, C, Args...> internal{func};
		return {internal.addr, internal.adjustor};
	}

	template <typename R, typename C, typename ...Args>
	inline auto mfp_from_func(generic_func_t addr) noexcept -> R(C::*)(Args...)
	{
		mfp_internal_t<R, C, Args...> internal{addr};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	inline auto mfp_from_func(generic_func_t addr, std::size_t adjustor) noexcept -> R(C::*)(Args...)
	{
		mfp_internal_t<R, C, Args...> internal{addr, adjustor};
		return internal.func;
	}

	template <typename R, typename C, typename ...Args>
	inline std::size_t vfunc_index(R(C::*func)(Args...)) noexcept
	{
		mfp_internal_t<R, C, Args...> internal{func};
		return ((reinterpret_cast<std::uintptr_t>(internal.addr)-1) / sizeof(generic_func_t));
	}

	template <typename C>
	inline generic_func_t *vtable_from_addr(void *addr) noexcept
	{ return reinterpret_cast<generic_func_t *>(addr); }

	template <typename C>
	inline generic_func_t *vtable_from_object(C *ptr) noexcept
	{
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
	#endif
		return *reinterpret_cast<generic_func_t **>(ptr);
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

	template <typename R, typename C, typename ...Args>
	inline auto swap_vfunc(C *ptr, R(C::*old_func)(Args...), R(*new_func)(C *, Args...)) noexcept -> R(C::*)(Args...)
	{
		generic_func_t *vtable{vtable_from_object<C>(ptr)};
		std::size_t index{vfunc_index(old_func)};
		generic_func_t old_vfunc{vtable[index]};
		page_info func_page{vtable + ((index > 0) ? (index-1) : 0), sizeof(generic_func_t)};
		func_page.protect(PROT_READ|PROT_WRITE|PROT_EXEC);
		vtable[index] = reinterpret_cast<generic_func_t>(new_func);
		return mfp_from_func<R, C, Args...>(old_vfunc);
	}

	class detour
	{
	public:
		detour() noexcept = default;
		~detour() noexcept;

		template <typename R, typename ...Args>
		inline void initialize(R(*old_func_)(Args...), R(*new_func_)(Args...)) noexcept
		{
			old_func = reinterpret_cast<generic_func_t>(old_func_);
			new_func = reinterpret_cast<generic_func_t>(new_func_);

			backup_bytes();
		}

		template <typename T, typename ...Args>
		inline auto call(Args &&...args) noexcept -> function_return_t<T>
		{
			scope_enable se{*this};
			return reinterpret_cast<function_pointer_t<T>>(old_func)(std::forward<Args>(args)...);
		}

		void enable() noexcept;

		inline void disable() noexcept
		{
			unsigned char *bytes{reinterpret_cast<unsigned char *>(old_func)};
			std::memcpy(bytes, old_bytes, sizeof(old_bytes));
		}

	protected:
		struct scope_enable final {
			inline scope_enable(detour &det_) noexcept
				: det{det_} {
				det.disable();
			}
			inline ~scope_enable() noexcept {
				det.enable();
			}
			detour &det;
		};

		void backup_bytes() noexcept;

		union {
			generic_func_t old_func;
			generic_mfp_t old_mfp;
		};

		generic_func_t new_func;

	private:
		unsigned char old_bytes[1 + sizeof(std::uintptr_t)];
	};

	class detour_va_args final : public detour
	{
	public:
		template <typename T>
		inline void initialize(T old_func_, T new_func_) noexcept
		{
			old_func = reinterpret_cast<generic_func_t>(old_func_);
			new_func = reinterpret_cast<generic_func_t>(new_func_);

			backup_bytes();
		}

		template <typename R, typename T, typename ...Args>
		inline R call(Args &&...args) noexcept
		{
			scope_enable se{*this};
			return reinterpret_cast<T>(old_func)(std::forward<Args>(args)...);
		}
	};
}
