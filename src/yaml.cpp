#include "yaml.hpp"
#include "filesystem.hpp"
#include <cstring>
#include "hacking.hpp"
#include <string_view>
#include "vmod.hpp"

namespace vmod
{
	class_desc_t<yaml> yaml_desc{"__vmod_yaml_class"};

	gsdk::HSCRIPT yaml::script_get_document(std::size_t i) noexcept
	{
		if(i >= documents.size()) {
			return nullptr;
		}

		return documents[i]->root_object;
	}

	gsdk::HSCRIPT yaml_singleton::script_load(std::filesystem::path &&path_) noexcept
	{
		if(!path_.is_absolute()) {
			path_ = std::filesystem::current_path() / path_;
		}

		yaml *temp_yaml{new yaml};

		if(!temp_yaml->initialize(std::move(path_))) {
			delete temp_yaml;
			return nullptr;
		}

		temp_yaml->set_plugin();

		return temp_yaml->instance;
	}

	yaml_singleton::~yaml_singleton() noexcept
	{
	}

	singleton_class_desc_t<yaml_singleton> yaml_singleton_desc{"__vmod_yaml_singleton_class"};

	static class yaml_singleton yaml_singleton;

	inline class yaml_singleton &yaml_singleton::instance() noexcept
	{ return ::vmod::yaml_singleton; }

	bool yaml_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		yaml_singleton_desc.func(&yaml_singleton::script_load, "script_load"sv, "load"sv);

		if(!vm->RegisterClass(&yaml_singleton_desc)) {
			error("vmod: failed to register yaml singleton script class\n"sv);
			return false;
		}

		vs_instance_ = vm->RegisterInstance(&yaml_singleton_desc, &::vmod::yaml_singleton);
		if(!vs_instance_ || vs_instance_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to register yaml singleton instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(vs_instance_, "__vmod_yaml_singleton");

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(!vm->SetValue(vmod_scope, "yaml", vs_instance_)) {
			error("vmod: failed to set yaml singleton value\n"sv);
			return false;
		}

		return true;
	}

	bool yaml::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		yaml_desc.func(&yaml::script_num_documents, "script_num_documents"sv, "num_documents"sv);
		yaml_desc.func(&yaml::script_get_document, "script_get_document"sv, "get_document"sv);
		yaml_desc.func(&yaml::script_delete, "script_delete"sv, "free"sv);
		yaml_desc.dtor();
		yaml_desc.doc_class_name("yaml"sv);

		if(!vmod.vm()->RegisterClass(&yaml_desc)) {
			error("vmod: failed to register yaml script class\n"sv);
			return false;
		}

		if(!yaml_singleton.bindings()) {
			return false;
		}

		return true;
	}

	void yaml_singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(vs_instance_ && vs_instance_ != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(vs_instance_);
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(vm->ValueExists(vmod_scope, "yaml")) {
			vm->ClearValue(vmod_scope, "yaml");
		}
	}

	void yaml::unbindings() noexcept
	{
		yaml_singleton.unbindings();
	}

	yaml::document::~document() noexcept
	{
		while(!object_stack.empty()) {
			gsdk::HSCRIPT obj{object_stack.top()};
			vmod.vm()->ReleaseObject(obj);
			object_stack.pop();
		}

		yaml_document_delete(this);
	}

	bool yaml::document::node_to_variant(yaml_node_t *node, script_variant_t &var) noexcept
	{
		switch(node->type) {
			case YAML_NO_NODE: {
				null_variant(var);
				return true;
			}
			case YAML_SCALAR_NODE: {
				if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_NULL_TAG) == 0) {
					null_variant(var);
					return true;
				} else if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_BOOL_TAG) == 0) {
					var = *reinterpret_cast<bool *>(node->data.scalar.value);
					return true;
				} else if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_STR_TAG) == 0) {
					var = std::string_view{reinterpret_cast<const char *>(node->data.scalar.value)};
					return true;
				} else if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_INT_TAG) == 0) {
					switch(node->data.scalar.length) {
						case sizeof(char):
						var = static_cast<short>(*reinterpret_cast<char *>(node->data.scalar.value));
						return true;
						case sizeof(short):
						var = *reinterpret_cast<short *>(node->data.scalar.value);
						return true;
						case sizeof(int):
						var = *reinterpret_cast<int *>(node->data.scalar.value);
						return true;
						case sizeof(long long):
						var = *reinterpret_cast<long long *>(node->data.scalar.value);
						return true;
					}

					null_variant(var);
					return false;
				} else if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_FLOAT_TAG) == 0) {
					switch(node->data.scalar.length) {
						case sizeof(float):
						var = *reinterpret_cast<float *>(node->data.scalar.value);
						return true;
						case sizeof(double):
						var = *reinterpret_cast<double *>(node->data.scalar.value);
						return true;
						case sizeof(long double):
						var = *reinterpret_cast<long double *>(node->data.scalar.value);
						return true;
					}

					null_variant(var);
					return false;
				} else {
					null_variant(var);
					return false;
				}
			}
			case YAML_SEQUENCE_NODE: {
				gsdk::IScriptVM *vm{vmod.vm()};

				gsdk::HSCRIPT temp_array{vm->CreateArray()};

				for(yaml_node_item_t *item{node->data.sequence.items.start}; item != node->data.sequence.items.top; ++item) {
					yaml_node_t *value_node{yaml_document_get_node(this, *item)};

					script_variant_t value_var;
					if(!node_to_variant(value_node, value_var)) {
						vm->ReleaseArray(temp_array);
						null_variant(var);
						return false;
					}

					vm->ArrayAddToTail(temp_array, value_var);
				}

				object_stack.emplace(temp_array);
				var = temp_array;
				return true;
			}
			case YAML_MAPPING_NODE: {
				gsdk::IScriptVM *vm{vmod.vm()};

				gsdk::HSCRIPT temp_table{vm->CreateTable()};

				for(yaml_node_pair_t *pair{node->data.mapping.pairs.start}; pair != node->data.mapping.pairs.top; ++pair) {
					yaml_node_t *key_node{yaml_document_get_node(this, pair->key)};
					if(std::strcmp(reinterpret_cast<const char *>(key_node->tag), YAML_STR_TAG) != 0) {
						vm->ReleaseTable(temp_table);
						null_variant(var);
						return false;
					}

					yaml_node_t *value_node{yaml_document_get_node(this, pair->value)};

					script_variant_t value_var;
					if(!node_to_variant(value_node, value_var)) {
						vm->ReleaseTable(temp_table);
						null_variant(var);
						return false;
					}

					if(!vm->SetValue(temp_table, reinterpret_cast<const char *>(key_node->data.scalar.value), value_var)) {
						vm->ReleaseTable(temp_table);
						null_variant(var);
						return false;
					}
				}

				object_stack.emplace(temp_table);
				var = temp_table;
				return true;
			}
		}

		null_variant(var);
		return false;
	}

	static char __yaml_id_buff[1024];

	bool yaml::initialize(std::filesystem::path &&path_) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		struct scope_delete_parser final {
			inline scope_delete_parser(yaml_parser_t &parser_) noexcept
				: parser{parser_} {}
			inline ~scope_delete_parser() noexcept
			{ yaml_parser_delete(&parser); }
			yaml_parser_t &parser;
		};

		yaml_parser_t parser{};
		if(yaml_parser_initialize(&parser) != 1) {
			return false;
		}
		scope_delete_parser sdp{parser};

		std::size_t size;
		std::unique_ptr<unsigned char[]> data{read_file(path_, size)};

		if(size > 0) {
			yaml_parser_set_input_string(&parser, data.get(), size);

			while(true) {
				yaml_document_t temp_doc{};
				if(yaml_parser_load(&parser, &temp_doc) != 1) {
					documents.clear();
					return false;
				}

				yaml_node_t *root{yaml_document_get_root_node(&temp_doc)};
				if(!root) {
					break;
				}

				document *doc{new document{std::move(temp_doc)}};

				script_variant_t root_var;
				if(!doc->node_to_variant(root, root_var)) {
					delete doc;
					documents.clear();
					return false;
				}

				doc->root_object = root_var.m_hScript;
				documents.emplace_back(doc);
			}
		}

		instance = vm->RegisterInstance(&yaml_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: failed to register yaml instance");
			return false;
		}

		if(!vm->GenerateUniqueKey("__vmod_yaml_instance_", __yaml_id_buff, sizeof(__yaml_id_buff))) {
			vm->RaiseException("vmod: failed to generate yaml unique id");
			return false;
		}

		vm->SetInstanceUniqeId(instance, __yaml_id_buff);

		return true;
	}

	yaml::~yaml() noexcept
	{
		documents.clear();

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->RemoveInstance(instance);
		}
	}
}
