#pragma once

#include <cstddef>
#include "../../vscript/vscript.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../../gsdk/server/baseentity.hpp"
#include "../singleton.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include "datamap.hpp"
#include "sendtable.hpp"
#include "serverclass.hpp"
#include "factory.hpp"
#include <variant>

namespace vmod::bindings::ent
{
	class singleton final : public singleton_base
	{
		friend void write_docs(const std::filesystem::path &) noexcept;
		friend class allocated_datamap;
		friend class allocated_dataprop;

	public:
		inline singleton() noexcept
			: singleton_base{"ent"}
		{
		}

		~singleton() noexcept override;

		bool bindings() noexcept;
		void unbindings() noexcept;

		static singleton &instance() noexcept;

	private:
		static vscript::singleton_class_desc<singleton> desc;

		static vscript::instance_handle_ref script_from_ptr(gsdk::CBaseEntity *ptr) noexcept;
		static void *script_to_ptr(vscript::instance_handle_wrapper obj) noexcept;

		vscript::instance_handle_ref script_lookup_sendprop(std::string_view path) noexcept;
		vscript::instance_handle_ref script_lookup_sendtable(std::string_view path) noexcept;

		vscript::instance_handle_ref script_lookup_dataprop(std::string_view path) noexcept;
		vscript::instance_handle_ref script_lookup_datatable(std::string_view path) noexcept;

		vscript::instance_handle_ref script_lookup_serverclass(std::string_view path) noexcept;

		vscript::instance_handle_ref script_find_factory(std::string &&name) noexcept;
		static vscript::instance_handle_ref script_create_factory(std::variant<vscript::array_handle_wrapper, std::string_view> names, vscript::func_handle_wrapper func, std::optional<vscript::func_handle_wrapper> size_func) noexcept;

		vscript::instance_handle_ref script_create_datatable(vscript::table_handle_wrapper datadesc) noexcept;

		enum class prop_result_type : unsigned char
		{
			none,
			prop,
			table
		};

		struct prop_data_result
		{
			inline prop_data_result(prop_result_type type_, std::uintptr_t value_) noexcept
				: type{type_}, value{value_}
			{
			}

			inline prop_data_result(gsdk::datamap_t *table_) noexcept
				: type{prop_result_type::table}, table{table_}
			{
			}

			inline prop_data_result(gsdk::typedescription_t *prop_) noexcept
				: type{prop_result_type::prop}, prop{prop_}
			{
			}

			prop_result_type type;

			union {
				std::uintptr_t value;

				gsdk::datamap_t *table;
				gsdk::typedescription_t *prop;
			};
		};

		struct prop_send_result
		{
			inline prop_send_result(prop_result_type type_, std::uintptr_t value_) noexcept
				: type{type_}, value{value_}
			{
			}

			inline prop_send_result(gsdk::SendTable *table_) noexcept
				: type{prop_result_type::table}, table{table_}
			{
			}

			inline prop_send_result(gsdk::SendProp *prop_) noexcept
				: type{prop_result_type::prop}, prop{prop_}
			{
			}

			prop_result_type type;

			union {
				std::uintptr_t value;

				gsdk::SendTable *table;
				gsdk::SendProp *prop;
			};
		};

		struct prop_result
		{
			enum class which : unsigned char
			{
				none,
				send,
				data,
				both
			};

			which which;

			prop_result_type type;

			union {
				std::uintptr_t value;

				gsdk::datamap_t *datatable;
				gsdk::typedescription_t *dataprop;

				gsdk::SendTable *sendtable;
				gsdk::SendProp *sendprop;
			};

			prop_result &operator+=(const prop_data_result &other) noexcept;
			prop_result &operator+=(const prop_send_result &other) noexcept;

			inline explicit operator prop_data_result() const noexcept
			{ return prop_data_result{type, value}; }
			inline explicit operator prop_send_result() const noexcept
			{ return prop_send_result{type, value}; }
		};

		enum class prop_tree_flags : unsigned char
		{
			data =             (1 << 0),
			send =             (1 << 1),
			lazy =             (1 << 2),
			ignore_exclude =   (1 << 3),
			only_prop =        (1 << 4),
			only_table =       (1 << 5),
			both =             (data|send),
		};
		friend constexpr inline bool operator&(prop_tree_flags lhs, prop_tree_flags rhs) noexcept
		{ return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); }
		friend constexpr inline prop_tree_flags operator|(prop_tree_flags lhs, prop_tree_flags rhs) noexcept
		{ return static_cast<prop_tree_flags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); }
		friend constexpr inline prop_tree_flags operator~(prop_tree_flags lhs) noexcept
		{ return static_cast<prop_tree_flags>(~static_cast<unsigned char>(lhs)); }
		friend constexpr inline prop_tree_flags &operator&=(prop_tree_flags &lhs, prop_tree_flags rhs) noexcept
		{ lhs = static_cast<prop_tree_flags>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); return lhs; }
		friend constexpr inline prop_tree_flags &operator|=(prop_tree_flags &lhs, prop_tree_flags rhs) noexcept
		{ lhs = static_cast<prop_tree_flags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); return lhs; }

		bool walk_prop_tree(std::string_view path, prop_tree_flags flags, prop_result &result) noexcept;

		struct prop_tree_cache_t
		{
			std::unordered_map<std::string, prop_data_result> data;
			std::unordered_map<std::string, prop_send_result> send;

			std::unordered_map<std::string, std::string> lazy_to_full;

			std::unordered_map<std::uintptr_t, std::string> ptr_to_path;
		};

		prop_tree_cache_t prop_tree_cache;

		std::unordered_map<gsdk::SendProp *, std::unique_ptr<sendprop>> sendprops;
		std::unordered_map<gsdk::SendTable *, std::unique_ptr<sendtable>> sendtables;

		std::unordered_map<gsdk::typedescription_t *, std::unique_ptr<dataprop>> dataprops;
		std::unordered_map<gsdk::datamap_t *, std::unique_ptr<datamap>> datatables;

		std::unordered_map<gsdk::ServerClass *, std::unique_ptr<serverclass>> serverclasses;

		void erase(std::uintptr_t value) noexcept;

		void erase(gsdk::datamap_t *map) noexcept;
		void erase(gsdk::typedescription_t *prop) noexcept;

		std::unordered_map<std::string, std::unique_ptr<factory_ref>> factories;

	private:
		singleton(const singleton &) = delete;
		singleton &operator=(const singleton &) = delete;
		singleton(singleton &&) = delete;
		singleton &operator=(singleton &&) = delete;
	};
}
