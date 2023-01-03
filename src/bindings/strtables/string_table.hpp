#pragma once

#include <string_view>
#include "../../vscript/class_desc.hpp"
#include "../../gsdk/engine/stringtable.hpp"
#include "bindings.hpp"
#include "../instance.hpp"
#include <cstddef>

namespace vmod
{
	class main;
}

namespace vmod::bindings::strtables
{
	class string_table final : public instance_base
	{
		friend class vmod::main;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		~string_table() noexcept override;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		inline string_table(gsdk::INetworkStringTable *table_) noexcept
			: table{table_}
		{
		}

		inline string_table() noexcept
			: table{nullptr}
		{
		}

		static vscript::class_desc<string_table> desc;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

		std::size_t script_find_index(std::string_view name) const noexcept;
		std::size_t script_num_strings() const noexcept;
		std::string_view script_get_string(std::size_t i) const noexcept;
		std::size_t script_add_string(std::string_view name, ssize_t bytes, const void *data) noexcept;

		gsdk::INetworkStringTable *table;

	private:
		string_table(const string_table &) = delete;
		string_table &operator=(const string_table &) = delete;
		string_table(string_table &&) = delete;
		string_table &operator=(string_table &&) = delete;
	};
}
