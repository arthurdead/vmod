#pragma once

#include <yaml.h>
#include "vscript.hpp"
#include <filesystem>
#include <vector>
#include <cstddef>

namespace vmod
{
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<yaml_node_type_t>() noexcept
	{ return gsdk::FIELD_INTEGER; }

	template <>
	inline void initialize_variant_value<yaml_node_type_t>(gsdk::ScriptVariant_t &var, yaml_node_type_t value) noexcept
	{ var.m_int = static_cast<int>(value); }

	class yaml final
	{
		friend class vmod;

	public:
		yaml(std::filesystem::path &&path_) noexcept;
		~yaml() noexcept;

		class document final : public yaml_document_t
		{
			friend class yaml;

		public:
			document() noexcept = default;
			document(const document &) = delete;
			document &operator=(const document &) = delete;

			inline document(document &&other) noexcept
				: yaml_document_t{std::move(other)}
			{
			}

			inline document(yaml_document_t &&other) noexcept
				: yaml_document_t{std::move(other)},
				mapped{gsdk::INVALID_HSCRIPT}
			{
			}

			inline document &operator=(document &&other) noexcept
			{
				*static_cast<yaml_document_t *>(this) = std::move(static_cast<yaml_document_t &>(other));
				mapped = other.mapped;
				other.mapped = gsdk::INVALID_HSCRIPT;
				return *this;
			}

			~document() noexcept;

		private:
			bool node_to_variant(yaml_node_t *node, script_variant_t &var) noexcept;

			gsdk::HSCRIPT mapped;
		};

	private:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		static gsdk::HSCRIPT script_load(std::filesystem::path &&path_) noexcept;

		inline std::size_t script_num_documents() const noexcept
		{ return documents.size(); }

		inline void script_delete() const noexcept
		{ delete this; }

		gsdk::HSCRIPT script_get_document(std::size_t i) noexcept;

		std::vector<std::unique_ptr<document>> documents;

		gsdk::HSCRIPT instance;
	};
}
