#pragma once

#include "../../vscript/vscript.hpp"
#include "../../gsdk/server/baseentity.hpp"
#include "../../plugin.hpp"
#include "../instance.hpp"
#include <vector>
#include <string>
#include <string_view>
#include "datamap.hpp"
#include "sendtable.hpp"

namespace vmod::bindings::ent
{
	class factory_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		virtual ~factory_base() noexcept;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	protected:
		inline factory_base(gsdk::IEntityFactory *factory_) noexcept
			: factory{factory_}
		{
		}

		static vscript::class_desc<factory_base> desc;

	private:
		gsdk::IServerNetworkable *script_create(std::string_view classname) noexcept;

		inline std::size_t script_size() const noexcept
		{ return factory->GetEntitySize(); }

	protected:
		gsdk::IEntityFactory *factory;

	private:
		factory_base() = delete;
		factory_base(const factory_base &) = delete;
		factory_base &operator=(const factory_base &) = delete;
		factory_base(factory_base &&) = delete;
		factory_base &operator=(factory_base &&) = delete;
	};

	class factory_ref : public factory_base, public instance_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;
		friend class factory_base;

	public:
		using factory_base::factory_base;

		~factory_ref() noexcept override;

		static bool detours() noexcept;

	protected:
		static vscript::class_desc<factory_ref> desc;

	private:
		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

		gsdk::IServerNetworkable *script_create_sized(std::string_view classname, std::size_t new_size) noexcept;

	private:
		factory_ref() = delete;
		factory_ref(const factory_ref &) = delete;
		factory_ref &operator=(const factory_ref &) = delete;
		factory_ref(factory_ref &&) = delete;
		factory_ref &operator=(factory_ref &&) = delete;
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class factory_impl final : public gsdk::IEntityFactory, public factory_base, public plugin::owned_instance
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;
		friend class factory_base;

	public:
		~factory_impl() noexcept override;

		static bool detours() noexcept;

	private:
		static vscript::class_desc<factory_impl> desc;

		bool initialize(std::vector<std::string> &&names_, vscript::func_handle_wrapper &&callback_, vscript::func_handle_wrapper &&size_callback_) noexcept;

		gsdk::IServerNetworkable *script_create(std::string_view classname, std::optional<vscript::table_handle_wrapper> opts) noexcept;

		inline factory_impl() noexcept
			: factory_base{this}
		{
		}

		vscript::handle_wrapper callback{};
		vscript::handle_wrapper size_callback{};
		std::vector<std::string> names;

		struct create_options
		{
			std::optional<std::size_t> new_size;
		};

		gsdk::IServerNetworkable *create(std::string_view classname, std::optional<create_options> opts) noexcept;
		inline gsdk::IServerNetworkable *create(std::string_view classname) noexcept
		{ return create(classname, std::nullopt); }

		gsdk::IServerNetworkable *Create(const char *classname) override;
		void Destroy(gsdk::IServerNetworkable *net) override;
		size_t GetEntitySize() override;

	private:
		factory_impl(gsdk::IEntityFactory *) noexcept = delete;
		factory_impl(const factory_impl &) = delete;
		factory_impl &operator=(const factory_impl &) = delete;
		factory_impl(factory_impl &&) = delete;
		factory_impl &operator=(factory_impl &&) = delete;
	};
	#pragma GCC diagnostic pop
}
