#pragma once

#include "../../gsdk/server/datamap.hpp"
#include "../../vscript/vscript.hpp"
#include "../../vscript/class_desc.hpp"
#include "../../plugin.hpp"
#include "../mem/singleton.hpp"
#include "../../ffi.hpp"
#include <memory>
#include <vector>
#include "../../xxhash.hpp"

namespace vmod::bindings::ent
{
	class singleton;
	class factory_impl;

	namespace detail
	{
		class dataprop_base
		{
			friend class ent::singleton;

		public:
			inline dataprop_base(dataprop_base &&other) noexcept
			{ operator=(std::move(other)); }
			inline dataprop_base &operator=(dataprop_base &&other) noexcept
			{
				prop = other.prop;
				other.prop = nullptr;
				type_ptr = other.type_ptr;
				type = other.type;
				return *this;
			}

		protected:
			static vscript::class_desc<dataprop_base> desc;

			static ffi_type *guess_type(const gsdk::typedescription_t *prop, const gsdk::datamap_t *table) noexcept;

			inline gsdk::HSCRIPT script_type() noexcept
			{ return type->table(); }

			dataprop_base(gsdk::typedescription_t *prop_) noexcept;

			gsdk::typedescription_t *prop;

		private:
			ffi_type *type_ptr;
			mem::singleton::type *type;

		private:
			dataprop_base() = delete;
			dataprop_base(const dataprop_base &) = delete;
			dataprop_base &operator=(const dataprop_base &) = delete;
		};

		class datamap_base
		{
			friend class ent::singleton;
			friend class ent::factory_impl;

		public:
			inline datamap_base(datamap_base &&other)
			{ operator=(std::move(other)); }
			inline datamap_base &operator=(datamap_base &&other) noexcept
			{
				map = other.map;
				other.map = nullptr;
				return *this;
			}

		protected:
			static vscript::class_desc<datamap_base> desc;

			datamap_base(gsdk::datamap_t *map_) noexcept;

		public:
			gsdk::datamap_t *map;

		private:
			datamap_base() = delete;
			datamap_base(const datamap_base &) = delete;
			datamap_base &operator=(const datamap_base &) = delete;
		};
	}

	class dataprop final : public detail::dataprop_base, public instance_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		~dataprop() noexcept override;

	private:
		using detail::dataprop_base::dataprop_base;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

	private:
		dataprop() = delete;
		dataprop(const dataprop &) = delete;
		dataprop &operator=(const dataprop &) = delete;
		dataprop(dataprop &&) = delete;
		dataprop &operator=(dataprop &&) = delete;
	};

	class datamap final : public detail::datamap_base, public instance_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		~datamap() noexcept override;

	private:
		using detail::datamap_base::datamap_base;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

	private:
		datamap() = delete;
		datamap(const datamap &) = delete;
		datamap &operator=(const datamap &) = delete;
		datamap(datamap &&) = delete;
		datamap &operator=(datamap &&) = delete;
	};

	extern XXH64_hash_t hash_datamap(gsdk::datamap_t *map) noexcept;

	class allocated_datamap : public detail::datamap_base
	{
		friend class factory_impl;

	public:
		~allocated_datamap() noexcept;

		allocated_datamap(allocated_datamap &&) noexcept = default;
		allocated_datamap &operator=(allocated_datamap &&) noexcept = default;

		void append(const allocated_datamap &other) noexcept;

		std::unique_ptr<allocated_datamap> calculate_offsets(std::size_t base) const noexcept;

		inline XXH64_hash_t hash() const noexcept
		{ return hash_; }

	private:
		static vscript::class_desc<allocated_datamap> desc;

		inline allocated_datamap() noexcept
			: detail::datamap_base{new gsdk::datamap_t}
		{
			maps.emplace_back(map);
			maps_storage.emplace_back(new map_storage);
		}

		inline allocated_datamap(std::nullptr_t) noexcept
			: detail::datamap_base{nullptr}
		{
		}

		inline allocated_datamap(const allocated_datamap &other) noexcept
			: allocated_datamap{nullptr}
		{ operator=(other); }
		allocated_datamap &operator=(const allocated_datamap &other) noexcept;

		struct prop_storage
		{
			std::string name;
			std::string external_name;
		};

		struct map_storage
		{
			std::vector<std::unique_ptr<prop_storage>> props_storage;
			std::vector<gsdk::typedescription_t> props;

			std::string name;
		};

		std::vector<std::unique_ptr<map_storage>> maps_storage;

	public:
		std::vector<std::unique_ptr<gsdk::datamap_t>> maps;

	private:
		std::size_t total_size{0};

		std::size_t last_offset{0};

		XXH64_hash_t hash_;
	};
}
