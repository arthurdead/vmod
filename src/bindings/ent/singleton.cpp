#include "singleton.hpp"
#include "../../main.hpp"
#include "sendprop.hpp"
#include "factory.hpp"

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

		desc.func(&singleton::script_lookup_sendprop, "script_lookup_sendprop"sv, "lookup_sendprop"sv);
		desc.func(&singleton::script_from_ptr, "script_from_ptr"sv, "from_ptr"sv);
		desc.func(&singleton::script_find_factory, "script_find_factory"sv, "find_factory"sv);
		desc.func(&singleton::script_create_factory, "script_create_factory"sv, "create_factory"sv);

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
			vm->RaiseException("vmod: invalid name");
			return nullptr;
		}

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size");
			return nullptr;
		}

		if(!callback || callback == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: null function");
			return nullptr;
		}

		gsdk::IEntityFactory *factory{entityfactorydict->FindFactory(facname.data())};
		if(factory) {
			vm->RaiseException("vmod: name already in use");
			return nullptr;
		}

		factory_impl *sfac{new factory_impl{}};
		if(!sfac->initialize(facname, callback)) {
			delete sfac;
			return nullptr;
		}

		return sfac->instance;
	}

	gsdk::HSCRIPT singleton::script_find_factory(std::string_view facname) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(facname.empty()) {
			vm->RaiseException("vmod: invalid name");
			return nullptr;
		}

		gsdk::IEntityFactory *factory{entityfactorydict->FindFactory(facname.data())};
		if(!factory) {
			return nullptr;
		}

		factory_impl *impl{dynamic_cast<factory_impl *>(factory)};
		if(impl) {
			return impl->instance;
		}

		factory_ref *sfac{new factory_ref{factory}};
		if(!sfac->initialize()) {
			delete sfac;
			return nullptr;
		}

		return sfac->instance;
	}

	gsdk::HSCRIPT singleton::script_from_ptr(gsdk::CBaseEntity *ptr) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return nullptr;
		}

		return ptr->GetScriptInstance();
	}

	gsdk::HSCRIPT singleton::script_lookup_sendprop(std::string_view path) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(path.empty()) {
			vm->RaiseException("vmod: invalid path");
			return nullptr;
		}

		prop_result res;

		prop_tree_flags flags{
			prop_tree_flags::only_prop|
			prop_tree_flags::send|
			prop_tree_flags::ignore_exclude|
			prop_tree_flags::lazy
		};
		if(!walk_prop_tree(path, flags, res)) {
			return nullptr;
		}

		auto it{sendprops.find(res.sendprop)};
		if(it == sendprops.end()) {
			std::unique_ptr<sendprop> prop{new sendprop{res.sendprop}};
			if(!prop->initialize()) {
				return nullptr;
			}

			it = sendprops.emplace(res.sendprop, std::move(prop)).first;
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
				std::string_view name{path.substr(name_start, name_end-name_start)};

				std::size_t subscript_start{name.find('[')};
				if(subscript_start != std::string_view::npos) {
					std::size_t subscript_end{name.find(']', subscript_start+1)};
					if(subscript_end == std::string_view::npos) {
						error("vmod: subscript started but not ended\n"sv);
						return false;
					} else if(subscript_end != name.length()) {
						error("vmod: subscript ending must be the last character\n"sv);
						return false;
					}

					std::string_view subscript_num{name.substr(subscript_start, subscript_end-subscript_start)};

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

					unsigned char found{0};

					if(flags & prop_tree_flags::send) {
						bool send_found{false};

						gsdk::SendTable *temp_table{curr_sendtable};

						std::function<void()> loop_props{
							[flags,&full_path,&send_found,&temp_table,&curr_sendprop,name,&loop_props]() noexcept -> void {
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

									if(std::strncmp(prop.m_pVarName, name.data(), name.length()) == 0) {
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
							error("vmod: member '%.*s' was not found in '%.*s'\n"sv, name.length(), name.data(), last_send_name.length(), last_send_name.data());
							return false;
						}
					}

					if(flags & prop_tree_flags::data) {
						bool data_found{false};

						gsdk::datamap_t *temp_table{curr_datamap};

						std::function<void()> loop_props{
							[flags,&full_path,&data_found,&temp_table,&curr_dataprop,&curr_datamap,name,&loop_props]() noexcept -> void {
								gsdk::datamap_t *baseclass{temp_table->baseMap};

								if(name == "baseclass"sv) {
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
									if(std::strncmp(prop.fieldName, name.data(), name.length()) == 0) {
										curr_dataprop = &prop;
										data_found = true;
										return;
									} else if(flags & prop_tree_flags::lazy) {
										if(prop.td) {
											check_later.emplace_back(std::pair<const char *, gsdk::datamap_t *>{prop.fieldName, prop.td});
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
							error("vmod: member '%.*s' was not found in '%.*s'\n"sv, name.length(), name.data(), last_data_name.length(), last_data_name.data());
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

				last_send_name = name;
				last_data_name = name;

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

		return true;
	}
}
