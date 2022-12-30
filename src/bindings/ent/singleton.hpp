#pragma once

#include <cstddef>
#include "../../vscript/vscript.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../../gsdk/server/baseentity.hpp"
#include "../../gsdk/engine/dt_send.hpp"
#include "../singleton.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>

namespace vmod::bindings::ent
{
	class sendprop;

	class singleton final : public singleton_base
	{
		friend void write_docs(const std::filesystem::path &) noexcept;

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

		static gsdk::HSCRIPT script_from_ptr(gsdk::CBaseEntity *ptr) noexcept;

		gsdk::HSCRIPT script_lookup_sendprop(std::string_view path) noexcept;

		static gsdk::HSCRIPT script_find_factory(std::string_view name) noexcept;
		static gsdk::HSCRIPT script_create_factory(std::string_view name, gsdk::HSCRIPT func, std::size_t size) noexcept;

		enum class prop_result_type : unsigned char
		{
			none,
			prop,
			table
		};

		struct prop_data_result
		{
			prop_result_type type;

			union {
				gsdk::datamap_t *table;
				gsdk::typedescription_t *prop;
			};
		};

		struct prop_send_result
		{
			prop_result_type type;

			union {
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

			enum which which;

			prop_result_type type;

			union {
				gsdk::datamap_t *datatable;
				gsdk::typedescription_t *dataprop;

				gsdk::SendTable *sendtable;
				gsdk::SendProp *sendprop;
			};

			prop_result &operator+=(const prop_data_result &other) noexcept;
			prop_result &operator+=(const prop_send_result &other) noexcept;

			inline explicit operator prop_data_result() const noexcept
			{ return prop_data_result{type, {datatable}}; }
			inline explicit operator prop_send_result() const noexcept
			{ return prop_send_result{type, {sendtable}}; }
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
		};

		prop_tree_cache_t prop_tree_cache;

		std::unordered_map<gsdk::SendProp *, std::unique_ptr<sendprop>> sendprops;

	private:
		singleton(const singleton &) = delete;
		singleton &operator=(const singleton &) = delete;
		singleton(singleton &&) = delete;
		singleton &operator=(singleton &&) = delete;
	};
}
