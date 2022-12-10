#pragma once

#include <yaml.h>
#include "vscript.hpp"
#include "plugin.hpp"
#include <filesystem>
#include <vector>
#include <stack>
#include <cstddef>

namespace vmod
{
	class yaml final : plugin::owned_instance
	{
		friend class vmod;

	public:
		~yaml() noexcept override;
		yaml() noexcept = default;

	private:
		bool initialize(std::filesystem::path &&path_) noexcept;

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

	class yaml_singleton final
	{
	public:
		~yaml_singleton() noexcept;

		bool bindings() noexcept;
		void unbindings() noexcept;

		static yaml_singleton &instance() noexcept;

	private:
		static gsdk::HSCRIPT script_load(std::filesystem::path &&path_) noexcept;

		gsdk::HSCRIPT vs_instance_;
	};

	extern class_desc_t<yaml> yaml_desc;
	extern singleton_class_desc_t<yaml_singleton> yaml_singleton_desc;
}
