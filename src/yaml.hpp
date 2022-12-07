#pragma once

#include <yaml.h>
#include "vscript.hpp"
#include <filesystem>
#include <vector>
#include <stack>
#include <cstddef>

namespace vmod
{
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
				root_object{gsdk::INVALID_HSCRIPT}
			{
			}

			inline document &operator=(document &&other) noexcept
			{
				*static_cast<yaml_document_t *>(this) = std::move(static_cast<yaml_document_t &>(other));
				object_stack = std::move(other.object_stack);
				root_object = other.root_object;
				other.root_object = gsdk::INVALID_HSCRIPT;
				return *this;
			}

			~document() noexcept;

		private:
			bool node_to_variant(yaml_node_t *node, script_variant_t &var) noexcept;

			std::stack<gsdk::HSCRIPT> object_stack;
			gsdk::HSCRIPT root_object;
		};

	private:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		friend class yaml_singleton;

		inline std::size_t script_num_documents() const noexcept
		{ return documents.size(); }

		inline void script_delete() const noexcept
		{ delete this; }

		gsdk::HSCRIPT script_get_document(std::size_t i) noexcept;

		std::vector<std::unique_ptr<document>> documents;

		gsdk::HSCRIPT instance;
	};
}
