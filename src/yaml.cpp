#include "yaml.hpp"
#include "filesystem.hpp"
#include <cstring>
#include "hacking.hpp"
#include <string_view>
#include "vmod.hpp"

namespace vmod
{
	static class_desc_t<yaml> yaml_desc{"yaml"};
	static class_desc_t<yaml::document> yaml_doc_desc{"yaml_document"};
	static class_desc_t<yaml::node_reference> yaml_node_desc{"yaml_node"};

	gsdk::HSCRIPT yaml::script_get_document(std::size_t i) noexcept
	{
		if(i >= documents.size()) {
			return nullptr;
		}

		document &doc{*documents[i]};
		return doc.instance;
	}

	gsdk::HSCRIPT yaml::document::get_instance_for_node(yaml_node_t *node) noexcept
	{
		auto it{node_references.find(node)};
		if(it == node_references.end()) {
			std::unique_ptr<node_reference> ref{new node_reference};

			gsdk::HSCRIPT node_instance{vm->RegisterInstance(&yaml_node_desc, ref.get())};
			if(!node_instance || node_instance == gsdk::INVALID_HSCRIPT) {
				return nullptr;
			}

			ref->owner = this;
			ref->node = node;
			ref->instance = node_instance;

			it = node_references.emplace(node, std::move(ref)).first;
		}

		return it->second->instance;
	}

	gsdk::HSCRIPT yaml::script_load(std::filesystem::path &&path_) noexcept
	{
		if(!path_.is_absolute()) {
			path_ = std::filesystem::current_path() / path_;
		}

		yaml *temp_yaml{new yaml{std::move(path_)}};

		return temp_yaml->instance;
	}

	bool yaml::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		yaml_desc.func(&yaml::script_load, "load"sv);
		yaml_desc.func(&yaml::script_num_documents, "num_documents"sv);
		yaml_desc.func(&yaml::script_get_document, "get_document"sv);
		yaml_desc.func(&yaml::script_delete, "free"sv);
		yaml_desc.dtor();

		yaml_doc_desc.func(&yaml::document::script_get_root_node, "get_root_node"sv);

		yaml_node_desc.func(&yaml::node_reference::script_get_type, "get_type"sv);

		if(!vm->RegisterClass(&yaml_desc)) {
			return false;
		}

		return true;
	}

	void yaml::unbindings() noexcept
	{
		
	}

	yaml::node_reference::~node_reference() noexcept
	{
		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}
	}

	yaml::document::~document() noexcept
	{
		node_references.clear();

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}

		yaml_document_delete(this);
	}

	yaml::yaml(std::filesystem::path &&path_) noexcept
	{
		instance = vm->RegisterInstance(&yaml_desc, this);

		yaml_parser_t parser;
		if(yaml_parser_initialize(&parser) != 1) {
			return;
		}

		std::size_t size;
		std::unique_ptr<unsigned char[]> data{read_file(path_, size)};

		if(size > 0) {
			yaml_parser_set_input_string(&parser, data.get(), size);

			while(true) {
				yaml_document_t temp_doc;
				if(yaml_parser_load(&parser, &temp_doc) != 1) {
					documents.clear();
					break;
				}

				if(!yaml_document_get_root_node(&temp_doc)) {
					break;
				}

				document *doc{new document{std::move(temp_doc)}};
				documents.emplace_back(doc);

				gsdk::HSCRIPT doc_instance{vm->RegisterInstance(&yaml_doc_desc, doc)};
				if(!doc_instance || doc_instance == gsdk::INVALID_HSCRIPT) {
					doc_instance = gsdk::INVALID_HSCRIPT;
				}
				doc->instance = doc_instance;
			}
		}

		yaml_parser_delete(&parser);
	}

	yaml::~yaml() noexcept
	{
		documents.clear();

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}
	}
}
