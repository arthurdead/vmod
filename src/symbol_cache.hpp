#pragma once

#ifndef __VMOD_COMPILING_VTABLE_DUMPER
#include "gsdk/config.hpp"
#endif

#if !defined GSDK_NO_SYMBOLS || defined __VMOD_COMPILING_VTABLE_DUMPER

#include <cstddef>
#include <string_view>
#include <string>
#include <vector>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <functional>
#include "hacking.hpp"
#include "type_traits.hpp"

#include <libelf.h>
#include <gelf.h>
#include "libiberty.hpp"

#ifdef __VMOD_COMPILING_VTABLE_DUMPER
#include <llvm/Object/MachO.h>
#endif

namespace vmod
{
	class symbol_cache final
	{
	public:
		static bool initialize() noexcept;

		symbol_cache() noexcept = default;

		bool load(const std::filesystem::path &path, void *base) noexcept;

		inline const std::string &error_string() const noexcept
		{ return err_str; }

		struct qualification_info
		{
			friend class symbol_cache;

			virtual ~qualification_info() noexcept;

			qualification_info() noexcept = default;
			qualification_info(qualification_info &&) noexcept = default;
			qualification_info &operator=(qualification_info &&) noexcept = default;

			struct name_info;

		private:
			using names_t = std::unordered_map<std::string, std::unique_ptr<name_info>>;

		public:
			struct name_info
			{
				friend class symbol_cache;

				virtual ~name_info() noexcept;

				name_info() noexcept = default;
				name_info(name_info &&) noexcept = default;
				name_info &operator=(name_info &&) noexcept = default;

			#ifndef __VMOD_COMPILING_VTABLE_DUMPER
				template <typename T>
				inline T addr() const noexcept
				{ return reinterpret_cast<T>(const_cast<void *>(addr_)); }

				template <typename T>
				inline auto func() const noexcept -> function_pointer_t<T>
				{ return reinterpret_cast<function_pointer_t<T>>(func_); }

				template <typename T>
				inline auto mfp() const noexcept -> function_pointer_t<T>
				{
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wcast-function-type"
					return reinterpret_cast<function_pointer_t<T>>(mfp_.func);
					#pragma GCC diagnostic pop
				}

				inline std::size_t size() const noexcept
				{ return size_; }
			#endif

				inline std::size_t virtual_index() const noexcept
				{ return vindex; }

			#ifndef __VMOD_COMPILING_VTABLE_DUMPER
				using const_iterator = names_t::const_iterator;

				inline const_iterator find(const std::string &name) const noexcept
				{ return names.find(name); }

				inline const_iterator begin() const noexcept
				{ return names.cbegin(); }
				inline const_iterator end() const noexcept
				{ return names.cend(); }

				inline const_iterator cbegin() const noexcept
				{ return names.cbegin(); }
				inline const_iterator cend() const noexcept
				{ return names.cend(); }
			#endif

			private:
			#ifndef __VMOD_COMPILING_VTABLE_DUMPER
				virtual void resolve(void *base) noexcept;
			#endif

			#ifndef __VMOD_COMPILING_VTABLE_DUMPER
				std::uint64_t offset_{0};
			#endif
				std::size_t vindex{static_cast<std::size_t>(-1)};
			#ifndef __VMOD_COMPILING_VTABLE_DUMPER
				std::size_t size_{0};
				union {
					void *addr_;
					generic_func_t func_;
					generic_internal_mfp_t mfp_{nullptr};
				};

				names_t names;
			#endif

			private:
				name_info(const name_info &) = delete;
				name_info &operator=(const name_info &) = delete;
			};

		public:
			using const_iterator = names_t::const_iterator;

			inline const_iterator find(const std::string &name) const noexcept
			{ return names.find(name); }

			inline const_iterator begin() const noexcept
			{ return names.cbegin(); }
			inline const_iterator end() const noexcept
			{ return names.cend(); }

			inline const_iterator cbegin() const noexcept
			{ return names.cbegin(); }
			inline const_iterator cend() const noexcept
			{ return names.cend(); }

		private:
			names_t names;

		private:
			qualification_info(const qualification_info &) = delete;
			qualification_info &operator=(const qualification_info &) = delete;
		};

	private:
		using qualifications_t = std::unordered_map<std::string, std::unique_ptr<qualification_info>>;

		using vtable_sizes_t = std::unordered_map<std::string, std::size_t>;

	public:
		struct pair_t
		{
			qualifications_t::const_iterator qual;
			qualification_info::names_t::const_iterator func;
		};

		struct class_info final : qualification_info
		{
			friend class symbol_cache;

		public:
			~class_info() noexcept override;

			struct vtable_info final
			{
				friend class symbol_cache;

			private:
				using funcs_t = std::vector<pair_t>;

			public:
				using const_iterator = funcs_t::const_iterator;

				inline const_iterator begin() const noexcept
				{ return funcs_.cbegin(); }
				inline const_iterator end() const noexcept
				{ return funcs_.cend(); }

				inline const_iterator cbegin() const noexcept
				{ return funcs_.cbegin(); }
				inline const_iterator cend() const noexcept
				{ return funcs_.cend(); }

				inline std::size_t size() const noexcept
				{ return size_; }

				inline funcs_t::value_type operator[](std::size_t i) const noexcept
				{ return funcs_[i]; }

				inline const funcs_t &funcs() const noexcept
				{ return funcs_; }

			private:
				void resolve(void *base) noexcept;

				vtable_info() noexcept = default;

				std::uint64_t offset{0};
				std::size_t size_{0};
				__cxxabiv1::vtable_prefix *prefix{nullptr};
				funcs_t funcs_;

			private:
				vtable_info(const vtable_info &) = delete;
				vtable_info &operator=(const vtable_info &) = delete;
				vtable_info(vtable_info &&) = delete;
				vtable_info &operator=(vtable_info &&) = delete;
			};

			inline const vtable_info &vtable() const noexcept
			{ return vtable_; }

		private:
			class_info() noexcept = default;

		#ifndef __VMOD_COMPILING_VTABLE_DUMPER
			struct ctor_info final : name_info
			{
			private:
				friend class symbol_cache;

				using kind_t = gnu_v3_ctor_kinds;

				void resolve(void *base) noexcept override;

				ctor_info() noexcept = default;

				kind_t kind;

			private:
				ctor_info(const ctor_info &) = delete;
				ctor_info &operator=(const ctor_info &) = delete;
				ctor_info(ctor_info &&) = delete;
				ctor_info &operator=(ctor_info &&) = delete;
			};
		#endif

			struct dtor_info final : name_info
			{
			public:
				~dtor_info() noexcept override;

			private:
				friend class symbol_cache;

				using kind_t = gnu_v3_dtor_kinds;

			#ifndef __VMOD_COMPILING_VTABLE_DUMPER
				void resolve(void *base) noexcept override;
			#endif

				dtor_info() noexcept = default;

				kind_t kind;

			private:
				dtor_info(const dtor_info &) = delete;
				dtor_info &operator=(const dtor_info &) = delete;
				dtor_info(dtor_info &&) = delete;
				dtor_info &operator=(dtor_info &&) = delete;
			};

			vtable_info vtable_;

		private:
			class_info(const class_info &) = delete;
			class_info &operator=(const class_info &) = delete;
			class_info(class_info &&) = delete;
			class_info &operator=(class_info &&) = delete;
		};

	public:
		using const_iterator = qualifications_t::const_iterator;

		inline const_iterator find(const std::string &name) const noexcept
		{ return qualifications.find(name); }

		inline const_iterator begin() const noexcept
		{ return qualifications.cbegin(); }
		inline const_iterator end() const noexcept
		{ return qualifications.cend(); }

		inline const_iterator cbegin() const noexcept
		{ return qualifications.cbegin(); }
		inline const_iterator cend() const noexcept
		{ return qualifications.cend(); }

	#ifndef __VMOD_COMPILING_VTABLE_DUMPER
		inline const qualification_info &global() const noexcept
		{ return global_qual; }
	#endif

		std::size_t vtable_size(const std::string &name) const noexcept;

	#ifndef __VMOD_COMPILING_VTABLE_DUMPER
		static std::uint64_t uncached_find_mangled_func(const std::filesystem::path &path, std::string_view search) noexcept;
	#endif

	private:
		std::string err_str;

		bool read_elf(int fd, void *base) noexcept;

	#ifdef __VMOD_COMPILING_VTABLE_DUMPER
		bool read_macho(llvm::object::MachOObjectFile &obj, void *base) noexcept;
	#endif

		static int demangle_flags;

		struct basic_sym_t
		{
			std::uint64_t off;
			std::size_t size;
		};

		bool handle_component(std::string_view name_mangled, demangle_component *component, qualifications_t::iterator &qual_it, qualification_info::names_t::iterator &name_it, basic_sym_t &&sym, void *base, bool elf) noexcept;

		void resolve_vtables(void *base, bool elf) noexcept;

		std::unordered_map<std::uint64_t, pair_t> offset_map;

		qualifications_t qualifications;
		vtable_sizes_t vtable_sizes;

		qualification_info global_qual;

	private:
		symbol_cache(const symbol_cache &) = delete;
		symbol_cache &operator=(const symbol_cache &) = delete;
		symbol_cache(symbol_cache &&) = delete;
		symbol_cache &operator=(symbol_cache &&) = delete;
	};
}

#endif
