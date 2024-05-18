#pragma once

#include "../../gsdk/config.hpp"

#ifndef GSDK_NO_SYMBOLS

#include "../../vscript/vscript.hpp"
#include "../../vscript/class_desc.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../../symbol_cache.hpp"
#include "../../plugin.hpp"
#include <string_view>

namespace vmod
{
	class main;
}

namespace vmod::bindings::syms
{
	class singleton
	{
		friend class vmod::main;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline singleton(std::string_view name_) noexcept
			: name{name_}
		{
		}

		static bool bindings() noexcept;
		static void unbindings() noexcept;

		virtual ~singleton() noexcept;

		class name_it;

		using name_cache_t = std::unordered_map<symbol_cache::qualification_info::const_iterator, std::unique_ptr<name_it>>;

		class qualification_it final : public instance_base
		{
			friend class singleton;
			friend void write_docs(const std::filesystem::path &) noexcept;

		public:
			static bool bindings() noexcept;
			static void unbindings() noexcept;

			inline qualification_it(symbol_cache::const_iterator it_) noexcept
				: it{it_}
			{
			}

			~qualification_it() noexcept override;

		private:
			static vscript::class_desc<qualification_it> desc;

			inline bool initialize() noexcept
			{ return register_instance(&desc, this); }

			vscript::instance_handle_ref script_lookup(std::string_view symname) noexcept;

			inline std::string_view script_name() const noexcept
			{ return it->first; }

			symbol_cache::const_iterator it;

			name_cache_t name_cache;

		private:
			qualification_it(const qualification_it &) = delete;
			qualification_it &operator=(const qualification_it &) = delete;
			qualification_it(qualification_it &&) = delete;
			qualification_it &operator=(qualification_it &&) = delete;
		};

		using qual_cache_t = std::unordered_map<symbol_cache::const_iterator, std::unique_ptr<qualification_it>>;

		class name_it final : public instance_base
		{
			friend class singleton;
			friend void write_docs(const std::filesystem::path &) noexcept;

		public:
			static bool bindings() noexcept;
			static void unbindings() noexcept;

			inline name_it(symbol_cache::qualification_info::const_iterator it_) noexcept
				: it{it_}
			{
			}

			~name_it() noexcept override;

		private:
			static vscript::class_desc<name_it> desc;

			inline bool initialize() noexcept
			{ return register_instance(&desc, this); }

			vscript::instance_handle_ref script_lookup(std::string_view symname) noexcept;

			inline std::string_view script_name() const noexcept
			{ return it->first; }
			inline void *script_addr() const noexcept
			{ return it->second->addr<void *>(); }
			inline generic_func_t script_func() const noexcept
			{ return it->second->func<generic_func_t>(); }
			inline generic_mfp_t script_mfp() const noexcept
			{ return it->second->mfp<generic_mfp_t>(); }
			inline std::uint64_t script_size() const noexcept
			{ return it->second->size(); }
			inline std::size_t script_vindex() const noexcept
			{ return it->second->virtual_index(); }

			symbol_cache::qualification_info::const_iterator it;

			name_cache_t name_cache;

		private:
			name_it(const name_it &) = delete;
			name_it &operator=(const name_it &) = delete;
			name_it(name_it &&) = delete;
			name_it &operator=(name_it &&) = delete;
		};

	private:
		static vscript::singleton_class_desc<singleton> desc;

	protected:
		bool initialize() noexcept;

	private:
		void free() noexcept;

		qual_cache_t glob_qual_cache;
		name_cache_t glob_name_cache;

		static vscript::instance_handle_ref script_lookup_qual(qual_cache_t &cache, symbol_cache::const_iterator it) noexcept;
		static vscript::instance_handle_ref script_lookup_name(name_cache_t &cache, symbol_cache::qualification_info::const_iterator it) noexcept;

		vscript::instance_handle_ref script_lookup(std::string_view symname) noexcept;
		vscript::instance_handle_ref script_lookup_global(std::string_view symname) noexcept;

		virtual const symbol_cache &cache() const noexcept = 0;

		vscript::instance_handle_wrapper instance{};

		std::string_view name;

	private:
		singleton(const singleton &) = delete;
		singleton &operator=(const singleton &) = delete;
		singleton(singleton &&) = delete;
		singleton &operator=(singleton &&) = delete;
	};

	class sv final : public singleton
	{
	public:
		inline sv() noexcept
			: singleton{"sv"}
		{
		}

		const symbol_cache &cache() const noexcept override;
	};
}

#endif
