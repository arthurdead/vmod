#pragma once

#if defined __VMOD_COMPILING_VTABLE_DUMPER || defined __VMOD_COMPILING_SIGNATURE_GUESSER
	#define __VMOD_COMPILING_SYMBOL_TOOL
#endif

#ifndef __VMOD_COMPILING_VTABLE_DUMPER
#include "gsdk/config.hpp"
#endif

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

#ifndef __VMOD_COMPILING_SYMBOL_TOOL
#include <yaml.h>
#endif

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

		bool load(const std::filesystem::path &path, unsigned char *base) noexcept;

		inline const std::string &error_string() const noexcept
		{ return err_str; }

	public:
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

				inline name_info() noexcept
				{
				}
				inline name_info(name_info &&other) noexcept
				{ operator=(std::move(other)); }
				name_info &operator=(name_info &&other) noexcept;

			#ifndef __VMOD_COMPILING_SYMBOL_TOOL
				template <typename T>
				inline T addr() const noexcept
				{ return reinterpret_cast<T>(const_cast<unsigned char *>(addr_)); }

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

				#ifndef GSDK_NO_SYMBOLS
				inline std::uint64_t size() const noexcept
				{ return size_; }
				#endif
			#endif

				inline std::size_t virtual_index() const noexcept
				{ return vindex; }

			#ifndef __VMOD_COMPILING_SYMBOL_TOOL
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
			#if !defined __VMOD_COMPILING_SYMBOL_TOOL
				void resolve_from_base(unsigned char *base) noexcept;
				virtual void resolve_absolute(unsigned char *addr) noexcept;
			#endif

			#if !defined __VMOD_COMPILING_SYMBOL_TOOL
			public:
				struct alignas(unsigned char) null_or_byte_t final
				{
					static constexpr unsigned char null{42};

					using value_t = std::decay_t<decltype(null)>;

				public:
					constexpr null_or_byte_t() noexcept = default;
					constexpr null_or_byte_t(const null_or_byte_t &) noexcept = default;
					constexpr null_or_byte_t &operator=(const null_or_byte_t &) noexcept = default;
					constexpr null_or_byte_t(null_or_byte_t &&) noexcept = default;
					constexpr null_or_byte_t &operator=(null_or_byte_t &&) noexcept = default;

					constexpr inline null_or_byte_t(unsigned char other) noexcept
						: value{static_cast<value_t>(other)}
					{
					}

					constexpr inline null_or_byte_t(std::nullptr_t) noexcept
						: value{null}
					{
					}

					constexpr inline operator bool() const noexcept
					{ return value != null; }
					constexpr inline bool operator!() const noexcept
					{ return value == null; }

					constexpr inline bool operator==(std::nullptr_t) const noexcept
					{ return value == null; }
					constexpr inline bool operator!=(std::nullptr_t) const noexcept
					{ return value != null; }

					constexpr inline bool operator==(unsigned char other) const noexcept
					{ return (value == null || static_cast<unsigned char>(value) == other); }
					constexpr inline bool operator!=(unsigned char other) const noexcept
					{ return (value != null && static_cast<unsigned char>(value) != other); }

					constexpr inline operator unsigned char() const noexcept
					{ return static_cast<unsigned char>(value); }

				private:
					value_t value{null};
				};

				static_assert(sizeof(null_or_byte_t) == sizeof(unsigned char));
				static_assert(alignof(null_or_byte_t) == alignof(unsigned char));

				struct bytes_info_t final
				{
					bytes_info_t() noexcept = default;
					bytes_info_t(bytes_info_t &&) noexcept = default;
					bytes_info_t &operator=(bytes_info_t &&) noexcept = default;

					using values_t = std::vector<null_or_byte_t>;

					values_t values;
					std::vector<std::size_t> null_positions;

					inline bytes_info_t &emplace_back(std::nullptr_t) noexcept
					{
						null_positions.emplace_back(values.size());
						values.emplace_back(nullptr);
						return *this;
					}

					inline bytes_info_t &emplace_back(unsigned char byte) noexcept
					{
						values.emplace_back(byte);
						return *this;
					}

					inline std::size_t size() const noexcept
					{ return values.size(); }

					inline bool empty() const noexcept
					{ return values.empty(); }

					inline bool has_null() const noexcept
					{ return !null_positions.empty(); }

					using const_iterator = values_t::const_iterator;

					inline unsigned char operator[](std::size_t i) const noexcept
					{ return values[i]; }

					inline const_iterator begin() const noexcept
					{ return values.begin(); }
					inline const_iterator end() const noexcept
					{ return values.end(); }

					inline const_iterator cbegin() const noexcept
					{ return values.cbegin(); }
					inline const_iterator cend() const noexcept
					{ return values.cend(); }

					inline const unsigned char *data() const noexcept
					{ return reinterpret_cast<const unsigned char *>(values.data()); }

				private:
					bytes_info_t(const bytes_info_t &) = delete;
					bytes_info_t &operator=(const bytes_info_t &) = delete;
				};

			private:
				#ifndef GSDK_NO_SYMBOLS
				enum class type : unsigned char
				{
					offset,
					bytes
				};
				type type_{type::offset};
				union {
					std::uint64_t offset_{0};
				#endif
					bytes_info_t bytes_info;
				#ifndef GSDK_NO_SYMBOLS
				};
				#endif

			#ifndef GSDK_NO_SYMBOLS
				inline void set_offset(std::uint64_t off) noexcept
				{
					type_ = type::offset;
					offset_ = off;
				}
			#endif

				inline void set_bytes(bytes_info_t &&info) noexcept
				{
				#ifndef GSDK_NO_SYMBOLS
					type_ = type::bytes;
					new (&bytes_info) bytes_info_t{std::move(info)};
				#else
					bytes_info = std::move(info);
				#endif
				}
			#endif
				std::size_t vindex{static_cast<std::size_t>(-1)};
			#ifndef __VMOD_COMPILING_SYMBOL_TOOL
				#ifndef GSDK_NO_SYMBOLS
				std::uint64_t size_{0};
				#endif
				union {
					unsigned char *addr_;
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
			#ifndef GSDK_NO_SYMBOLS
				void resolve_from_base(unsigned char *base) noexcept;
			#endif

				vtable_info() noexcept = default;

			#ifndef GSDK_NO_SYMBOLS
				std::uint64_t offset{0};
			#endif
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

		#ifndef __VMOD_COMPILING_SYMBOL_TOOL
			struct ctor_info final : name_info
			{
			private:
				friend class symbol_cache;

				using kind_t = gnu_v3_ctor_kinds;

				void resolve_absolute(unsigned char *addr) noexcept override;

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

			#ifndef __VMOD_COMPILING_SYMBOL_TOOL
				void resolve_absolute(unsigned char *addr) noexcept override;
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

	#ifndef __VMOD_COMPILING_SYMBOL_TOOL
		inline const qualification_info &global() const noexcept
		{ return global_qual; }
	#endif

		std::size_t vtable_size(const std::string &name) const noexcept;

	#ifndef __VMOD_COMPILING_SYMBOL_TOOL
		static std::uint64_t uncached_find_mangled_func(const std::filesystem::path &path, std::string_view search) noexcept;
	#endif

	private:
		std::string err_str;

	#ifndef __VMOD_COMPILING_SYMBOL_TOOL
		static std::filesystem::path yamls_dir;
	#endif

	#ifndef GSDK_NO_SYMBOLS
		bool read_elf_symbols(int fd, unsigned char *base) noexcept;
	#endif

	#ifdef __VMOD_COMPILING_VTABLE_DUMPER
		bool read_macho(llvm::object::MachOObjectFile &obj, unsigned char *base) noexcept;
	#endif

	#ifndef __VMOD_COMPILING_SYMBOL_TOOL
		bool read_elf_info(const std::filesystem::path &path) noexcept;
		bool read_elf_info(int fd) noexcept;

		bool read_yamls(const std::filesystem::path &dir, unsigned char *base) noexcept;
		bool read_yaml(const std::filesystem::path &path, unsigned char *base) noexcept;

		bool read_array_node(yaml_document_t &doc, yaml_node_t *node, std::string_view name, unsigned char *base) noexcept;
		bool read_map_node(yaml_document_t &doc, yaml_node_t *node, std::string_view name, unsigned char *base) noexcept;
		bool read_final_map_node(yaml_document_t &doc, yaml_node_t *node, std::string &&qualifier, std::string &&name, unsigned char *base) noexcept;
	#endif

	#ifndef GSDK_NO_SYMBOLS
		static int demangle_flags;

		struct basic_sym_t
		{
			std::uint64_t off;
			std::uint64_t size;
		};

		bool handle_component(std::string_view name_mangled, demangle_component *component, qualifications_t::iterator &qual_it, qualification_info::names_t::iterator &name_it, basic_sym_t &&sym, unsigned char *base, bool elf) noexcept;

		void resolve_vtables(unsigned char *base, bool elf) noexcept;

		std::unordered_map<std::uint64_t, pair_t> offset_map;
	#endif

		qualifications_t qualifications;
		vtable_sizes_t vtable_sizes;

		qualification_info global_qual;

	#ifndef __VMOD_COMPILING_SYMBOL_TOOL
		std::uint64_t memory_size{0};

		std::uint64_t data_size{0};
		std::uint64_t data_offset{0};
	#endif

	private:
		symbol_cache(const symbol_cache &) = delete;
		symbol_cache &operator=(const symbol_cache &) = delete;
		symbol_cache(symbol_cache &&) = delete;
		symbol_cache &operator=(symbol_cache &&) = delete;
	};
}
