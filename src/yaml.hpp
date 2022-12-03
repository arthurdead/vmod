#pragma once

#include <yaml.h>
#include "vscript.hpp"
#include <filesystem>
#include <vector>
#include <cstddef>

namespace vmod
{
	template <>
	inline void initialize_variant_value<yaml_node_type_t>(gsdk::ScriptVariant_t &var, yaml_node_type_t &&value) noexcept
	{ var.m_int = value; }

	class yaml final
	{
		friend class vmod;

	public:
		yaml(std::filesystem::path &&path_) noexcept;
		~yaml() noexcept;

		class document;

		class node_reference final
		{
			friend class yaml;

		public:
			node_reference(const node_reference &) = delete;
			node_reference &operator=(const node_reference &) = delete;
			inline node_reference(node_reference &&other) noexcept
			{ operator=(std::move(other)); }
			inline node_reference &operator=(node_reference &&other) noexcept
			{
				node = other.node;
				other.node = nullptr;
				owner = other.owner;
				other.owner = nullptr;
				instance = other.instance;
				other.instance = gsdk::INVALID_HSCRIPT;
				return *this;
			}
			~node_reference() noexcept;

		private:
			node_reference() noexcept = default;

			inline yaml_node_type_t script_get_type() const noexcept
			{ return node->type; }

			yaml_node_t *node;
			gsdk::HSCRIPT instance;
			document *owner;
		};

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
				instance = other.instance;
				other.instance = gsdk::INVALID_HSCRIPT;
			}

			inline document(yaml_document_t &&other) noexcept
				: yaml_document_t{std::move(other)}
			{
				instance = gsdk::INVALID_HSCRIPT;
			}

			inline document &operator=(document &&other) noexcept
			{
				*static_cast<yaml_document_t *>(this) = std::move(static_cast<yaml_document_t &>(other));
				instance = other.instance;
				other.instance = gsdk::INVALID_HSCRIPT;
				return *this;
			}

			~document() noexcept;

		private:
			inline yaml_node_t *root_node() noexcept
			{ return yaml_document_get_root_node(this); }

			gsdk::HSCRIPT get_instance_for_node(yaml_node_t *node) noexcept;

			inline gsdk::HSCRIPT script_get_root_node() noexcept
			{ return get_instance_for_node(root_node()); }

			gsdk::HSCRIPT instance;

			std::unordered_map<yaml_node_t *, std::unique_ptr<node_reference>> node_references;
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
