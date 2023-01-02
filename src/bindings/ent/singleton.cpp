#include "singleton.hpp"
#include "../../main.hpp"
#include "sendtable.hpp"
#include "datamap.hpp"
#include "factory.hpp"
#include "../../bindings/mem/singleton.hpp"
#include "../../hacking.hpp"
#include <cstddef>
#include <ffi.h>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

namespace vmod::bindings::ent
{
	vscript::singleton_class_desc<singleton> singleton::desc{"ent"};

	static singleton ent_;

	singleton &singleton::instance() noexcept
	{ return ent_; }

	singleton::~singleton() noexcept {}

	bool singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&singleton::script_lookup_sendprop, "script_lookup_sendprop"sv, "lookup_sendprop"sv)
		.desc("[sendprop](name)"sv);
		desc.func(&singleton::script_lookup_sendtable, "script_lookup_sendtable"sv, "lookup_sendtable"sv)
		.desc("[sendtable](name)"sv);

		desc.func(&singleton::script_lookup_dataprop, "script_lookup_dataprop"sv, "lookup_dataprop"sv)
		.desc("[dataprop](name)"sv);
		desc.func(&singleton::script_lookup_datatable, "script_lookup_datatable"sv, "lookup_datatable"sv)
		.desc("[datatable](name)"sv);

		desc.func(&singleton::script_create_datamap, "script_create_datamap"sv, "create_datatable"sv)
		.desc("[datatable](name, datatable|base, array<dataprop_description>|props)"sv);

		desc.func(&singleton::script_from_ptr, "script_from_ptr"sv, "from_ptr"sv)
		.desc("(ptr)"sv);

		desc.func(&singleton::script_find_factory, "script_find_factory"sv, "find_factory"sv)
		.desc("[factory_ref](name)"sv);

		desc.func(&singleton::script_create_factory, "script_create_factory"sv, "create_factory"sv)
		.desc("[factory_impl](name, function|callback, size)"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		singleton_base::unbindings();
	}

	gsdk::HSCRIPT singleton::script_create_factory(std::string_view facname, gsdk::HSCRIPT callback, std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(facname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", facname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size: %zu", size);
			return gsdk::INVALID_HSCRIPT;
		}

		if(!callback || callback == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid callback");
			return gsdk::INVALID_HSCRIPT;
		}

		gsdk::IEntityFactory *factory{entityfactorydict->FindFactory(facname.data())};
		if(factory) {
			vm->RaiseException("vmod: name already in use: '%s'", facname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		factory_impl *sfac{new factory_impl{}};
		if(!sfac->initialize(facname, callback)) {
			delete sfac;
			return gsdk::INVALID_HSCRIPT;
		}

		return sfac->instance;
	}

	gsdk::HSCRIPT singleton::script_find_factory(std::string_view facname) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(facname.empty()) {
			vm->RaiseException("vmod: invalid name: '%s'", facname.data());
			return gsdk::INVALID_HSCRIPT;
		}

		gsdk::IEntityFactory *factory{entityfactorydict->FindFactory(facname.data())};
		if(!factory) {
			return gsdk::INVALID_HSCRIPT;
		}

		factory_impl *impl{dynamic_cast<factory_impl *>(factory)};
		if(impl) {
			return impl->instance;
		}

		factory_ref *sfac{new factory_ref{factory}};
		if(!sfac->initialize()) {
			delete sfac;
			return gsdk::INVALID_HSCRIPT;
		}

		return sfac->instance;
	}

	gsdk::HSCRIPT singleton::script_from_ptr(gsdk::CBaseEntity *ptr) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return gsdk::INVALID_HSCRIPT;
		}

		return ptr->GetScriptInstance();
	}

	gsdk::HSCRIPT singleton::script_lookup_sendprop(std::string_view path) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(path.empty()) {
			vm->RaiseException("vmod: invalid path: '%s'", path.data());
			return gsdk::INVALID_HSCRIPT;
		}

		prop_result res;

		prop_tree_flags flags{
			prop_tree_flags::only_prop|
			prop_tree_flags::send|
			prop_tree_flags::ignore_exclude|
			prop_tree_flags::lazy
		};
		if(!walk_prop_tree(path, flags, res)) {
			return gsdk::INVALID_HSCRIPT;
		}

		auto it{sendprops.find(res.sendprop)};
		if(it == sendprops.end()) {
			std::unique_ptr<sendprop> prop{new sendprop{res.sendprop}};
			if(!prop->initialize()) {
				return gsdk::INVALID_HSCRIPT;
			}

			it = sendprops.emplace(res.sendprop, std::move(prop)).first;
		}

		return it->second->instance;
	}

	gsdk::HSCRIPT singleton::script_lookup_sendtable(std::string_view path) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(path.empty()) {
			vm->RaiseException("vmod: invalid path: '%s'", path.data());
			return gsdk::INVALID_HSCRIPT;
		}

		prop_result res;

		prop_tree_flags flags{
			prop_tree_flags::only_table|
			prop_tree_flags::send|
			prop_tree_flags::ignore_exclude|
			prop_tree_flags::lazy
		};
		if(!walk_prop_tree(path, flags, res)) {
			return gsdk::INVALID_HSCRIPT;
		}

		auto it{sendtables.find(res.sendtable)};
		if(it == sendtables.end()) {
			std::unique_ptr<sendtable> prop{new sendtable{res.sendtable}};
			if(!prop->initialize()) {
				return gsdk::INVALID_HSCRIPT;
			}

			it = sendtables.emplace(res.sendtable, std::move(prop)).first;
		}

		return it->second->instance;
	}

	gsdk::HSCRIPT singleton::script_lookup_dataprop(std::string_view path) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(path.empty()) {
			vm->RaiseException("vmod: invalid path: '%s'", path.data());
			return gsdk::INVALID_HSCRIPT;
		}

		prop_result res;

		prop_tree_flags flags{
			prop_tree_flags::only_prop|
			prop_tree_flags::data|
			prop_tree_flags::ignore_exclude|
			prop_tree_flags::lazy
		};
		if(!walk_prop_tree(path, flags, res)) {
			return gsdk::INVALID_HSCRIPT;
		}

		auto it{dataprops.find(res.dataprop)};
		if(it == dataprops.end()) {
			std::unique_ptr<dataprop> prop{new dataprop{res.dataprop}};
			if(!prop->initialize()) {
				return gsdk::INVALID_HSCRIPT;
			}

			it = dataprops.emplace(res.dataprop, std::move(prop)).first;
		}

		return it->second->instance;
	}

	gsdk::HSCRIPT singleton::script_lookup_datatable(std::string_view path) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(path.empty()) {
			vm->RaiseException("vmod: invalid path: '%s'", path.data());
			return gsdk::INVALID_HSCRIPT;
		}

		prop_result res;

		prop_tree_flags flags{
			prop_tree_flags::only_table|
			prop_tree_flags::data|
			prop_tree_flags::ignore_exclude|
			prop_tree_flags::lazy
		};
		if(!walk_prop_tree(path, flags, res)) {
			return gsdk::INVALID_HSCRIPT;
		}

		auto it{datatables.find(res.datatable)};
		if(it == datatables.end()) {
			std::unique_ptr<datamap> prop{new datamap{res.datatable}};
			if(!prop->initialize()) {
				return gsdk::INVALID_HSCRIPT;
			}

			it = datatables.emplace(res.datatable, std::move(prop)).first;
		}

		return it->second->instance;
	}

	singleton::prop_result &singleton::prop_result::operator+=(const prop_data_result &other) noexcept
	{
		switch(which) {
			case which::none:
			which = which::data; break;
			case which::send:
			which = which::both; break;
			default: break;
		}

		switch(other.type) {
			case prop_result_type::prop:
			dataprop = other.prop; break;
			case prop_result_type::table:
			datatable = other.table; break;
			default: break;
		}

		return *this;
	}

	singleton::prop_result &singleton::prop_result::operator+=(const prop_send_result &other) noexcept
	{
		switch(which) {
			case which::none:
			which = which::send; break;
			case which::data:
			which = which::both; break;
			default: break;
		}

		switch(other.type) {
			case prop_result_type::prop:
			sendprop = other.prop; break;
			case prop_result_type::table:
			sendtable = other.table; break;
			default: break;
		}

		return *this;
	}

	bool singleton::walk_prop_tree(std::string_view path, prop_tree_flags flags, prop_result &result) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(!(flags & prop_tree_flags::send) && !(flags & prop_tree_flags::data)) {
			error("vmod: no tree type specified\n"sv);
			return false;
		}

		if((flags & prop_tree_flags::only_prop) && (flags & prop_tree_flags::only_table)) {
			error("vmod: cannot exclude both prop and table\n"sv);
			return false;
		}

		if((flags & prop_tree_flags::send) && (flags & prop_tree_flags::data)) {
			error("vmod: walking both send and data is not supported yet\n"sv);
			return false;
		}

		if(flags & prop_tree_flags::lazy) {
			auto full_it{prop_tree_cache.lazy_to_full.find(std::string{path})};
			if(full_it != prop_tree_cache.lazy_to_full.end()) {
				flags &= ~prop_tree_flags::lazy;
				path = full_it->second;
			}
		}

		if(flags & prop_tree_flags::data) {
			auto datares_it{prop_tree_cache.data.find(std::string{path})};
			if(datares_it != prop_tree_cache.data.end()) {
				flags &= ~prop_tree_flags::data;
				result += datares_it->second;
			}
		}

		if(flags & prop_tree_flags::send) {
			auto sendres_it{prop_tree_cache.send.find(std::string{path})};
			if(sendres_it != prop_tree_cache.send.end()) {
				flags &= ~prop_tree_flags::send;
				result += sendres_it->second;
			}
		}

		if((result.which == prop_result::which::both) ||
			(!(flags & prop_tree_flags::send) && !(flags & prop_tree_flags::data))) {
			return true;
		}

		std::size_t path_len{path.length()};

		std::size_t name_start{path.find('.')};
		if(name_start == std::string_view::npos) {
			name_start = path_len;
		}

		std::string_view classname{path.substr(0, name_start)};

		auto sv_class_it{sv_ent_class_info.find(std::string{classname})};
		if(sv_class_it == sv_ent_class_info.end()) {
			error("vmod: invalid class '%.*s'\n"sv, classname.length(), classname.data());
			return false;
		}

		entity_class_info &class_info{sv_class_it->second};

		gsdk::ServerClass *sv_class{class_info.sv_class};

		gsdk::datamap_t *curr_datamap{nullptr};
		gsdk::typedescription_t *curr_dataprop{nullptr};

		gsdk::SendTable *curr_sendtable{nullptr};
		gsdk::SendProp *curr_sendprop{nullptr};

		std::string_view last_send_name{classname};
		std::string_view last_data_name{classname};

		if(flags & prop_tree_flags::send) {
			curr_sendtable = sv_class->m_pTable;
		}

		if(flags & prop_tree_flags::data) {
			curr_datamap = class_info.datamap;
		}

		std::string full_path;

		if(flags & prop_tree_flags::lazy) {
			full_path = classname;
		}

		if(name_start < path_len) {
			while(true) {
				std::size_t name_end{path.find('.', name_start+1)};
				bool done{name_end == std::string_view::npos};
				if(done) {
					name_end = path_len;
				}

				++name_start;
				std::string_view membername{path.substr(name_start, name_end-name_start)};

				std::size_t subscript_start{membername.find('[')};
				if(subscript_start != std::string_view::npos) {
					std::size_t subscript_end{membername.find(']', subscript_start+1)};
					if(subscript_end == std::string_view::npos) {
						error("vmod: subscript started but not ended\n"sv);
						return false;
					} else if(subscript_end != membername.length()) {
						error("vmod: subscript ending must be the last character\n"sv);
						return false;
					}

					std::string_view subscript_num{membername.substr(subscript_start, subscript_end-subscript_start)};

					const char *num_begin{subscript_num.data()};
					const char *num_end{num_begin + subscript_num.length()};

					std::size_t num;
					std::from_chars(num_begin, num_end, num);

					//TODO!!!!
					error("vmod: subscripts are not supported yet\n"sv);
					return false;
				} else {
					if((flags & prop_tree_flags::send) && !curr_sendtable) {
						error("vmod: '%.*s' cannot contain members\n"sv, last_send_name.length(), last_send_name.data());
						return false;
					}

					if((flags & prop_tree_flags::data) && !curr_datamap) {
						error("vmod: '%.*s' cannot contain members\n"sv, last_data_name.length(), last_data_name.data());
						return false;
					}

					[[maybe_unused]] unsigned char found{0};

					if(flags & prop_tree_flags::send) {
						bool send_found{false};

						gsdk::SendTable *temp_table{curr_sendtable};

						std::function<void()> loop_props{
							[flags,&full_path,&send_found,&temp_table,&curr_sendprop,membername,&loop_props]() noexcept -> void {
								gsdk::SendTable *baseclass{nullptr};
								std::vector<std::pair<const char *, gsdk::SendTable *>> check_later;

								std::size_t num_props{static_cast<std::size_t>(temp_table->m_nProps)};
								for(std::size_t i{0}; i < num_props; ++i) {
									gsdk::SendProp &prop{temp_table->m_pProps[i]};
									if(flags & prop_tree_flags::ignore_exclude) {
										if(prop.m_Flags & gsdk::SPROP_EXCLUDE) {
											continue;
										}
									}

									if(std::strncmp(prop.m_pVarName, membername.data(), membername.length()) == 0) {
										curr_sendprop = &prop;
										send_found = true;
										return;
									} else if(flags & prop_tree_flags::lazy) {
										if(std::strcmp(prop.m_pVarName, "baseclass") == 0) {
											baseclass = prop.m_pDataTable;
										} else if(prop.m_Type == gsdk::DPT_DataTable) {
											check_later.emplace_back(std::pair<const char *, gsdk::SendTable *>{prop.m_pVarName, prop.m_pDataTable});
										}
									}
								}

								if(flags & prop_tree_flags::lazy) {
									if(!check_later.empty()) {
										for(const auto &it : check_later) {
											temp_table = it.second;
											loop_props();
											if(send_found) {
												full_path += '.';
												full_path += it.first;
												return;
											}
										}
									}

									if(baseclass) {
										temp_table = baseclass;
										full_path += '.';
										full_path += "baseclass"sv;
										loop_props();
										if(send_found) {
											return;
										}
									}
								}
							}
						};

						loop_props();

						if(flags & prop_tree_flags::lazy) {
							full_path += '.';
							full_path += curr_sendprop->m_pVarName;
						}

						if(send_found) {
							found |= (1 << 0);
						}

						if(!send_found) {
							error("vmod: member '%.*s' was not found in '%.*s'\n"sv, membername.length(), membername.data(), last_send_name.length(), last_send_name.data());
							return false;
						}
					}

					if(flags & prop_tree_flags::data) {
						bool data_found{false};

						gsdk::datamap_t *temp_table{curr_datamap};

						std::function<void()> loop_props{
							[flags,&full_path,&data_found,&temp_table,&curr_dataprop,&curr_datamap,membername,&loop_props]() noexcept -> void {
								gsdk::datamap_t *baseclass{temp_table->baseMap};

								if(membername == "baseclass"sv) {
									if(baseclass) {
										curr_datamap = baseclass;
										data_found = true;
									} else {
										data_found = false;
									}
									return;
								}

								std::vector<std::pair<const char *, gsdk::datamap_t *>> check_later;

								std::size_t num_props{static_cast<std::size_t>(temp_table->dataNumFields)};
								for(std::size_t i{0}; i < num_props; ++i) {
									gsdk::typedescription_t &prop{temp_table->dataDesc[i]};
									if(std::strncmp(prop.fieldName, membername.data(), membername.length()) == 0) {
										curr_dataprop = &prop;
										data_found = true;
										return;
									} else if(flags & prop_tree_flags::lazy) {
										if(prop.fieldType == gsdk::FIELD_EMBEDDED) {
											check_later.emplace_back(std::pair<const char *, gsdk::datamap_t *>{prop.fieldName, prop.td});
										} else if(prop.flags & gsdk::FTYPEDESC_KEY) {
											if(std::strncmp(prop.externalName, membername.data(), membername.length()) == 0) {
												curr_dataprop = &prop;
												data_found = true;
												return;
											}
										}
									}
								}

								if(flags & prop_tree_flags::lazy) {
									full_path += '.';
									full_path += temp_table->dataClassName;

									if(!check_later.empty()) {
										for(const auto &it : check_later) {
											temp_table = it.second;
											loop_props();
											if(data_found) {
												full_path += '.';
												full_path += it.first;
												return;
											}
										}
									}

									if(baseclass) {
										temp_table = baseclass;
										full_path += '.';
										full_path += "baseclass"sv;
										loop_props();
										if(data_found) {
											return;
										}
									}
								}
							}
						};

						loop_props();

						if(flags & prop_tree_flags::lazy) {
							full_path += '.';
							full_path += curr_dataprop->fieldName;
						}

						if(data_found) {
							found |= (1 << 1);
						}

						if(!data_found) {
							error("vmod: member '%.*s' was not found in '%.*s'\n"sv, membername.length(), membername.data(), last_data_name.length(), last_data_name.data());
							return false;
						}
					}
				}

				if(flags & prop_tree_flags::send) {
					switch(curr_sendprop->m_Type) {
						case gsdk::DPT_DataTable: {
							curr_sendtable = curr_sendprop->m_pDataTable;
							curr_sendprop = nullptr;
						} break;
						default: {
							curr_sendtable = nullptr;
						} break;
					}
				}

				if(flags & prop_tree_flags::data) {
					if(curr_dataprop) {
						if(curr_dataprop->td) {
							curr_datamap = curr_dataprop->td;
							curr_dataprop = nullptr;
						} else {
							curr_datamap = nullptr;
						}
					} else if(curr_datamap) {
						curr_dataprop = nullptr;
					}
				}

				last_send_name = membername;
				last_data_name = membername;

				name_start = name_end;

				if(done) {
					break;
				}
			}
		}

		if(flags & prop_tree_flags::lazy) {
			prop_tree_cache.lazy_to_full.emplace(path, std::move(full_path));
		}

		if(flags & prop_tree_flags::send) {
			if(flags & prop_tree_flags::only_prop) {
				if(!curr_sendprop) {
					error("vmod: '%.*s' is not a prop\n"sv, last_send_name.length(), last_send_name.data());
					return false;
				}
			} else if(flags & prop_tree_flags::only_table) {
				if(!curr_sendtable) {
					error("vmod: '%.*s' is not a table\n"sv, last_send_name.length(), last_send_name.data());
					return false;
				}
			}
		}

		if(flags & prop_tree_flags::data) {
			if(flags & prop_tree_flags::only_prop) {
				if(!curr_dataprop) {
					error("vmod: '%.*s' is not a prop\n"sv, last_data_name.length(), last_data_name.data());
					return false;
				}
			} else if(flags & prop_tree_flags::only_table) {
				if(!curr_datamap) {
					error("vmod: '%.*s' is not a table\n"sv, last_data_name.length(), last_data_name.data());
					return false;
				}
			}
		}

		if(flags & prop_tree_flags::send) {
			if(curr_sendtable) {
				result.type = prop_result_type::table;
				result.sendtable = curr_sendtable;
			} else if(curr_sendprop) {
				result.type = prop_result_type::prop;
				result.sendprop = curr_sendprop;
			}

			switch(result.which) {
				case prop_result::which::none:
				result.which = prop_result::which::send; break;
				case prop_result::which::data:
				result.which = prop_result::which::both; break;
				default: break;
			}
		}

		if(flags & prop_tree_flags::data) {
			if(curr_datamap) {
				result.type = prop_result_type::table;
				result.datatable = curr_datamap;
			} else if(curr_dataprop) {
				result.type = prop_result_type::prop;
				result.dataprop = curr_dataprop;
			}

			switch(result.which) {
				case prop_result::which::none:
				result.which = prop_result::which::data; break;
				case prop_result::which::send:
				result.which = prop_result::which::both; break;
				default: break;
			}
		}

		if(flags & prop_tree_flags::send) {
			prop_tree_cache.send.emplace(path, static_cast<prop_send_result>(result));
		}

		if(flags & prop_tree_flags::data) {
			prop_tree_cache.data.emplace(path, static_cast<prop_data_result>(result));
		}

		prop_tree_cache.ptr_to_path.emplace(result.value, path);

		return true;
	}

	void singleton::erase(std::uintptr_t value) noexcept
	{
		auto path_it{prop_tree_cache.ptr_to_path.find(value)};
		if(path_it != prop_tree_cache.ptr_to_path.end()) {
			auto cache_it{prop_tree_cache.data.find(path_it->second)};
			if(cache_it != prop_tree_cache.data.end()) {
				prop_tree_cache.data.erase(cache_it);
			}
			prop_tree_cache.ptr_to_path.erase(path_it);
		}
	}

	void singleton::erase(gsdk::datamap_t *map) noexcept
	{
		auto table_it{datatables.find(map)};
		if(table_it != datatables.end()) {
			datatables.erase(table_it);
		}

		std::uintptr_t value{reinterpret_cast<std::uintptr_t>(map)};

		erase(value);
	}

	void singleton::erase(gsdk::typedescription_t *prop) noexcept
	{
		std::uintptr_t value{reinterpret_cast<std::uintptr_t>(prop)};

		erase(value);
	}

	gsdk::HSCRIPT singleton::script_create_datamap(std::string &&name, gsdk::HSCRIPT base, gsdk::HSCRIPT props_array) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!props_array || props_array == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid props");
			return gsdk::INVALID_HSCRIPT;
		}

		if(!vm->IsArray(props_array)) {
			vm->RaiseException("vmod: props is not a array");
			return gsdk::INVALID_HSCRIPT;
		}

		std::vector<gsdk::typedescription_t> props;
		std::vector<std::unique_ptr<allocated_datamap::prop_storage>> props_storage;

		std::vector<std::unique_ptr<gsdk::datamap_t>> maps;
		std::vector<std::unique_ptr<allocated_datamap::map_storage>> maps_storage;

		std::function<bool(gsdk::HSCRIPT)> read_props{
			[&read_props,vm,&props,&props_storage](gsdk::HSCRIPT var) noexcept -> bool {
				int array_num{vm->GetArrayCount(var)};
				for(int i{0}, it{0}; it != -1 && i < array_num; ++i) {
					vscript::variant value;
					it = vm->GetArrayValue(var, it, &value);

					gsdk::HSCRIPT prop_table{value.get<gsdk::HSCRIPT>()};

					if(!prop_table || prop_table == gsdk::INVALID_HSCRIPT) {
						vm->RaiseException("vmod: prop %i is invalid", i);
						return false;
					}

					if(vm->IsTable(prop_table)) {
						if(!vm->GetValue(prop_table, "name", &value)) {
							vm->RaiseException("vmod: prop %i is missing a name", i);
							return false;
						}

						gsdk::typedescription_t &prop{props.emplace_back()};
						auto &storage{props_storage.emplace_back(new allocated_datamap::prop_storage)};

						prop.flags = static_cast<short>(gsdk::FTYPEDESC_PRIVATE|gsdk::FTYPEDESC_VIEW_NEVER|gsdk::FTYPEDESC_NOERRORCHECK);

						std::string prop_name{value.get<std::string>()};
						if(prop_name.empty()) {
							vm->RaiseException("vmod: prop %i has empty name", i);
							return false;
						}

						storage->name = std::move(prop_name);
						prop.fieldName = storage->name.c_str();

						if(!vm->GetValue(prop_table, "type", &value)) {
							vm->RaiseException("vmod: prop '%s' is missing its type", prop.fieldName);
							return false;
						}

						ffi_type *type{mem::singleton::read_type(value.get<gsdk::HSCRIPT>())};
						if(!type) {
							vm->RaiseException("vmod: prop '%s' type is invalid", prop.fieldName);
							return false;
						}

						std::size_t num{1};

						if(vm->GetValue(prop_table, "num", &value)) {
							num = value.get<std::size_t>();

							if(num == 0 || num == static_cast<std::size_t>(-1)) {
								vm->RaiseException("vmod: prop '%s' has invalid num: %zu", prop.fieldName, num);
								return false;
							}
						}

						prop.fieldType = static_cast<gsdk::fieldtype_t>(ffi::to_field_type(type));
						prop.fieldSize = static_cast<unsigned short>(num);
						prop.fieldSizeInBytes = static_cast<int>(type->size);

						if(vm->GetValue(prop_table, "external_name", &value)) {
							std::string external_name{value.get<std::string>()};
							if(external_name.empty()) {
								vm->RaiseException("vmod: prop has '%s' invalid external name: '%s'", prop.fieldName, external_name.c_str());
								return gsdk::INVALID_HSCRIPT;
							}

							storage->external_name = std::move(external_name);
							prop.externalName = storage->external_name.c_str();

							prop.flags |= gsdk::FTYPEDESC_KEY;
						}
					} else if(vm->IsArray(prop_table)) {
						vm->RaiseException("vmod: prop %i is invalid", i);
						return false;
					} else {
						vm->RaiseException("vmod: prop %i is invalid", i);
						return false;
					}
				}

				return true;
			}
		};

		if(!read_props(props_array)) {
			return gsdk::INVALID_HSCRIPT;
		}

		allocated_datamap *map{new allocated_datamap};

		if(base && base != gsdk::INVALID_HSCRIPT) {
			auto base_map{vm->GetInstanceValue<detail::datamap_base>(base, &detail::datamap_base::desc)};
			if(base_map) {
				map->maps[0]->baseMap = base_map->map;
			}
		}

		map->maps_storage[0]->name = std::move(name);
		map->maps[0]->dataClassName = map->maps_storage[0]->name.c_str();

		map->maps_storage[0]->props = std::move(props);

		map->maps[0]->dataNumFields = static_cast<int>(map->maps_storage[0]->props.size());
		map->maps[0]->dataDesc = map->maps_storage[0]->props.data();

		return map->instance;
	}
}
