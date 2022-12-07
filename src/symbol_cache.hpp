#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <functional>
#include "hacking.hpp"

#include <libelf.h>
#include <gelf.h>

#pragma push_macro("HAVE_DECL_BASENAME")
#undef HAVE_DECL_BASENAME
#define HAVE_DECL_BASENAME 1
#include <libiberty/demangle.h>
#pragma pop_macro("HAVE_DECL_BASENAME")

namespace vmod
{
	class symbol_cache final
	{
	public:
		static bool initialize() noexcept;

		inline bool load(const std::filesystem::path &path) noexcept
		{ return load(path, nullptr); }
		bool load(const std::filesystem::path &path, void *base) noexcept;

		inline const std::string &error_string() const noexcept
		{ return err_str; }

		void resolve(void *base) noexcept;

		struct qualification_info
		{
			virtual ~qualification_info() noexcept = default;

			qualification_info() noexcept = default;
			qualification_info(const qualification_info &) = delete;
			qualification_info &operator=(const qualification_info &) = delete;
			qualification_info(qualification_info &&) noexcept = default;
			qualification_info &operator=(qualification_info &&) noexcept = default;

			struct name_info;

		private:
			using names_t = std::unordered_map<std::string, name_info>;

		public:
			struct name_info
			{
				virtual ~name_info() noexcept = default;

				inline name_info() noexcept
				{
				}

				name_info(const name_info &) = delete;
				name_info &operator=(const name_info &) = delete;
				name_info(name_info &&) noexcept = default;
				name_info &operator=(name_info &&) noexcept = default;

				template <typename T>
				inline T addr() const noexcept
				{ return reinterpret_cast<T>(const_cast<void *>(addr_)); }

				template <typename T>
				inline auto func() const noexcept -> function_pointer_t<T>
				{ return reinterpret_cast<function_pointer_t<T>>(func_); }

				template <typename T>
				inline auto mfp() const noexcept -> function_pointer_t<T>
				{ return reinterpret_cast<function_pointer_t<T>>(mfp_.func); }

				inline std::size_t size() const noexcept
				{ return size_; }

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
				friend class symbol_cache;

				virtual void resolve(void *base) noexcept;

				std::ptrdiff_t offset;
				std::size_t size_;
				union {
					void *addr_;
					generic_func_t func_;
					generic_internal_mfp_t mfp_;
				};

				names_t names;
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
			friend class symbol_cache;

			virtual void resolve(void *base) noexcept;

			names_t names;
		};

		struct class_info final : qualification_info
		{
		private:
			void resolve(void *base) noexcept override;

			struct vtable_info final
			{
			private:
				friend class symbol_cache;

				void resolve(void *base) noexcept;

				std::ptrdiff_t offset;
				std::vector<name_info> funcs;
				generic_vtable_t vtable;
			};

			struct ctor_info final : name_info
			{
			private:
				friend class symbol_cache;

				void resolve(void *base) noexcept override;
			};

			struct dtor_info final : name_info
			{
			private:
				friend class symbol_cache;

				void resolve(void *base) noexcept override;
			};

			vtable_info vtable;
			std::vector<ctor_info> ctors;
			std::vector<dtor_info> dtors;
		};

	private:
		using qualifications_t = std::unordered_map<std::string, qualification_info>;

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

		inline const qualification_info &global() const noexcept
		{ return global_qual; }

	private:
		std::string err_str;

		bool read_elf(int fd, void *base) noexcept;

		bool handle_component(std::string_view name_mangled, int base_demangle_flags, demangle_component *component, qualifications_t::iterator &qual_it, qualification_info::names_t::iterator &name_it, GElf_Sym &&sym, void *base) noexcept;

		qualifications_t qualifications;
		qualification_info global_qual;
	};
}
