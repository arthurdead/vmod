#include "symbol_cache.hpp"
#include "gsdk.hpp"
#include <string_view>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace vmod
{
	bool symbol_cache::initialize() noexcept
	{
		if(elf_version(EV_CURRENT) == EV_NONE) {
			return false;
		}

		cplus_demangle_set_style(gnu_v3_demangling);

		return true;
	}

	bool symbol_cache::load(const std::filesystem::path &path, void *base) noexcept
	{
		int fd{open(path.c_str(), O_RDONLY)};
		if(fd < 0) {
			int err{errno};
			err_str = strerror(err);
			return false;
		}

		if(!read_elf(fd, base)) {
			close(fd);
			return false;
		}

		close(fd);

		return true;
	}

	void symbol_cache::qualification_info::name_info::resolve(void *base) noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		mfp_.addr = reinterpret_cast<generic_plain_mfp_t>(static_cast<unsigned char *>(base) + offset_);
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
		mfp_.adjustor = 0;
	}

	void symbol_cache::class_info::ctor_info::resolve(void *base) noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		generic_plain_mfp_t temp{reinterpret_cast<generic_plain_mfp_t>(static_cast<unsigned char *>(base) + offset_)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
		mfp_ = mfp_from_func<void, generic_object_t>(temp);
	}

	void symbol_cache::class_info::dtor_info::resolve(void *base) noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		generic_plain_mfp_t temp{reinterpret_cast<generic_plain_mfp_t>(static_cast<unsigned char *>(base) + offset_)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
		mfp_ = mfp_from_func<void, generic_object_t>(temp);
	}

	void symbol_cache::class_info::vtable_info::resolve(void *base) noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-align"
	#endif
		prefix = reinterpret_cast<__cxxabiv1::vtable_prefix *>(static_cast<unsigned char *>(base) + offset);
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
	}

	void symbol_cache::qualification_info::resolve(void *base) noexcept
	{
		for(auto &it : names) {
			it.second->resolve(base);
		}
	}

	void symbol_cache::class_info::resolve(void *base) noexcept
	{
		qualification_info::resolve(base);

		vtable_.resolve(base);
	}

	void symbol_cache::resolve(void *base) noexcept
	{
		for(auto &it : qualifications) {
			it.second->resolve(base);
		}
	}

	bool symbol_cache::read_elf(int fd, void *base) noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		Elf *elf{elf_begin(fd, ELF_C_READ, nullptr)};
		if(!elf) {
			int err{elf_errno()};
			err_str = elf_errmsg(err);
			return false;
		}

		if(elf_kind(elf) != ELF_K_ELF || gelf_getclass(elf) == ELFCLASSNONE) {
			err_str = ""s;
			elf_end(elf);
			return false;
		}

		GElf_Shdr scn_hdr;
		GElf_Sym sym;

		Elf_Scn *scn{elf_nextscn(elf, nullptr)};
		while(scn) {
			struct scope_nextscn final {
				inline scope_nextscn(Elf *elf_, Elf_Scn *&scn_) noexcept
					: elf{elf_}, scn{scn_} {}
				inline ~scope_nextscn() noexcept
				{ scn = elf_nextscn(elf, scn); }
			private:
				Elf *elf;
				Elf_Scn *&scn;
			};

			scope_nextscn snscn{elf, scn};

			if(!gelf_getshdr(scn, &scn_hdr)) {
				continue;
			}

			switch(scn_hdr.sh_type) {
				case SHT_SYMTAB: break;
				default: continue;
			}

			Elf_Data *scn_data{elf_getdata(scn, nullptr)};

			std::size_t count{static_cast<std::size_t>(scn_hdr.sh_size) / static_cast<std::size_t>(scn_hdr.sh_entsize)};
			for(std::size_t i{0}; i < count; ++i) {
				gelf_getsym(scn_data, static_cast<int>(i), &sym);

				switch(GELF_ST_TYPE(sym.st_info)) {
					case STT_OBJECT:
					case STT_FUNC: break;
					default: continue;
				}

				switch(GELF_ST_VISIBILITY(sym.st_other)) {
					case STV_DEFAULT: break;
					default: continue;
				}

				std::string_view name_mangled{elf_strptr(elf, scn_hdr.sh_link, sym.st_name)};
				if(name_mangled.empty()) {
					continue;
				}

				int bind{GELF_ST_BIND(sym.st_info)};
				switch(bind) {
					case STB_LOCAL:
					case STB_GLOBAL: break;
					default: continue;
				}

				if(bind == STB_GLOBAL) {
					if(name_mangled == "__cxa_pure_virtual"sv) {
						std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
						std::ptrdiff_t offset{static_cast<std::ptrdiff_t>(sym.st_value)};
						info->offset_ = offset;
						info->size_ = static_cast<std::size_t>(sym.st_size);
						info->resolve(base);
						auto name_it{global_qual.names.emplace(name_mangled, std::move(info)).first};
						offset_map.emplace(offset, pair_t{qualifications.end(),name_it});
					}
					continue;
				}

				constexpr int base_demangle_flags{DMGL_GNU_V3|DMGL_PARAMS|DMGL_VERBOSE|DMGL_TYPES|DMGL_ANSI};

				struct scope_free final {
					inline scope_free(void *mem_) noexcept
						: mem{mem_} {}
					inline ~scope_free() noexcept {
						if(mem) {
							std::free(mem);
						}
					}
				private:
					void *mem;
				};

				void *component_mem{nullptr};
				demangle_component *component{cplus_demangle_v3_components(name_mangled.data(), base_demangle_flags, &component_mem)};
				scope_free sfcm{component_mem};

				qualifications_t::iterator tmp_qual_it;
				qualification_info::names_t::iterator tmp_name_it;
				handle_component(name_mangled, base_demangle_flags, component, tmp_qual_it, tmp_name_it, std::move(sym), base);
			}

			break;
		}

		elf_end(elf);

		for(auto &it : qualifications) {
			class_info *info{dynamic_cast<class_info *>(it.second.get())};
			if(info) {
				std::size_t vtable_size{info->vtable_.size_};
				if(vtable_size == 0) {
					continue;
				}

				info->vtable_.funcs_.resize(vtable_size, {qualifications.end(),info->names.end()});

				generic_vtable_t vtable{reinterpret_cast<generic_vtable_t>(const_cast<void **>(&info->vtable_.prefix->origin))};
				for(std::size_t i{0}; i < vtable_size; ++i) {
					generic_plain_mfp_t func{vtable[i]};
					std::ptrdiff_t off{reinterpret_cast<std::ptrdiff_t>(func) - reinterpret_cast<std::ptrdiff_t>(base)};
					auto map_it{offset_map.find(off)};
					if(map_it != offset_map.end()) {
						map_it->second.func->second->vindex = i;
						info->vtable_.funcs_[i] = map_it->second;
					}
				}
			}
		}

		offset_map.clear();

		return true;
	}

	bool symbol_cache::handle_component(std::string_view name_mangled, int base_demangle_flags, demangle_component *component, qualifications_t::iterator &qual_it, qualification_info::names_t::iterator &name_it, GElf_Sym &&sym, void *base) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(!component) {
			std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
			info->offset_ = static_cast<std::ptrdiff_t>(sym.st_value);
			info->size_ = static_cast<std::size_t>(sym.st_size);
			info->resolve(base);
			name_it = global_qual.names.emplace(name_mangled, std::move(info)).first;
			qual_it = qualifications.end();
			return true;
		}

		switch(component->type) {
			case DEMANGLE_COMPONENT_QUAL_NAME:
			case DEMANGLE_COMPONENT_VTABLE:
			case DEMANGLE_COMPONENT_LOCAL_NAME:
			case DEMANGLE_COMPONENT_NAME:
			case DEMANGLE_COMPONENT_TYPED_NAME: break;
			default: {
				name_it = global_qual.names.end();
				qual_it = qualifications.end();
				return false;
			}
		}

		std::size_t allocated;
		char *unmangled_buffer;

		constexpr std::size_t guessed_name_length{256};

		//TODO!!!!!! move this elsewhere
		static constexpr auto ignored_qual{
			[](std::string_view name) noexcept -> bool {
				if(name.starts_with("GCSDK"sv) ||
					name == "CRTime"sv ||
					name.starts_with("CryptoPP"sv) ||
					name == "CCrypto"sv ||
					name.starts_with("google::protobuf"sv) ||
					name.starts_with("CMsgGC_"sv) ||
					name.starts_with("CMsg"sv)) {
					return true;
				}
				return false;
			}
		};

		switch(component->type) {
			case DEMANGLE_COMPONENT_VTABLE: {
				unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.left, guessed_name_length, &allocated);
				if(!unmangled_buffer) {
					name_it = global_qual.names.end();
					qual_it = qualifications.end();
					return false;
				}

				std::string qual_name{unmangled_buffer};
				std::free(unmangled_buffer);

				if(ignored_qual(qual_name)) {
					name_it = global_qual.names.end();
					qual_it = qualifications.end();
					return false;
				}

				auto this_qual_it{qualifications.find(qual_name)};
				if(this_qual_it == qualifications.end()) {
					this_qual_it = qualifications.emplace(std::move(qual_name), new class_info{}).first;
				}

				class_info *info{dynamic_cast<class_info *>(this_qual_it->second.get())};
				if(info) {
					info->vtable_.offset = static_cast<std::ptrdiff_t>(sym.st_value);
					info->vtable_.size_ = ((static_cast<std::size_t>(sym.st_size) - (sizeof(__cxxabiv1::vtable_prefix) - sizeof(__cxxabiv1::vtable_prefix::origin))) / sizeof(generic_plain_mfp_t));
					info->vtable_.resolve(base);
				}

				name_it = global_qual.names.end();
				qual_it = this_qual_it;
				return false;
			}
			case DEMANGLE_COMPONENT_LOCAL_NAME: {
				switch(component->u.s_binary.left->type) {
					case DEMANGLE_COMPONENT_TYPED_NAME: {
						qualifications_t::iterator tmp_qual_it;
						qualification_info::names_t::iterator tmp_name_it;
						if(handle_component(name_mangled, base_demangle_flags, component->u.s_binary.left, tmp_qual_it, tmp_name_it, GElf_Sym{}, base) &&
							tmp_name_it != global_qual.names.end()) {
							unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
							if(!unmangled_buffer) {
								name_it = global_qual.names.end();
								qual_it = qualifications.end();
								return false;
							}

							std::string local_name{unmangled_buffer};
							std::free(unmangled_buffer);

							//TODO!!!!!! move this elsewhere
							if(local_name == "tm_fmt"sv ||
								tmp_name_it->first.starts_with("protobuf_AssignDesc_"sv) ||
								tmp_name_it->first.starts_with("protobuf_AddDesc_"sv)) {
								name_it = global_qual.names.end();
								qual_it = qualifications.end();
								return false;
							}

							std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
							info->offset_ = static_cast<std::ptrdiff_t>(sym.st_value);
							info->size_ = static_cast<std::size_t>(sym.st_size);
							info->resolve(base);

							name_it = tmp_name_it->second->names.emplace(std::move(local_name), std::move(info)).first;
							qual_it = tmp_qual_it;
							return true;
						} else {
							name_it = global_qual.names.end();
							qual_it = qualifications.end();
							return false;
						}
					}
					default: {
						name_it = global_qual.names.end();
						qual_it = qualifications.end();
						return false;
					}
				}
			}
			case DEMANGLE_COMPONENT_NAME: {
				unmangled_buffer = cplus_demangle_print(base_demangle_flags, component, guessed_name_length, &allocated);
				if(!unmangled_buffer) {
					name_it = global_qual.names.end();
					qual_it = qualifications.end();
					return false;
				}

				std::string name_unmangled{unmangled_buffer};
				std::free(unmangled_buffer);

				std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
				info->offset_ = static_cast<std::ptrdiff_t>(sym.st_value);
				info->size_ = static_cast<std::size_t>(sym.st_size);
				info->resolve(base);

				name_it = global_qual.names.emplace(std::move(name_unmangled), std::move(info)).first;
				qual_it = qualifications.end();
				return true;
			}
			case DEMANGLE_COMPONENT_QUAL_NAME: {
				switch(component->u.s_binary.right->type) {
					case DEMANGLE_COMPONENT_NAME: {
						switch(component->u.s_binary.left->type) {
							case DEMANGLE_COMPONENT_NAME: {
								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.left, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string qual_name{unmangled_buffer};
								std::free(unmangled_buffer);

								if(ignored_qual(qual_name)) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string name{unmangled_buffer};
								std::free(unmangled_buffer);

								std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};

								auto this_qual_it{qualifications.find(qual_name)};
								if(this_qual_it == qualifications.end()) {
									this_qual_it = qualifications.emplace(std::move(qual_name), new class_info{}).first;
								}

								std::ptrdiff_t offset{static_cast<std::ptrdiff_t>(sym.st_value)};
								info->offset_ = offset;
								info->size_ = static_cast<std::size_t>(sym.st_size);
								info->resolve(base);

								name_it = this_qual_it->second->names.insert_or_assign(std::move(name), std::move(info)).first;
								qual_it = this_qual_it;
								return true;
							}
							default: {
								name_it = global_qual.names.end();
								qual_it = qualifications.end();
								return false;
							}
						}
					}
					default: {
						name_it = global_qual.names.end();
						qual_it = qualifications.end();
						return false;
					}
				}
			}
			case DEMANGLE_COMPONENT_TYPED_NAME: {
				switch(component->u.s_binary.right->type) {
					case DEMANGLE_COMPONENT_FUNCTION_TYPE: {
						switch(component->u.s_binary.left->type) {
							case DEMANGLE_COMPONENT_RESTRICT_THIS:
							case DEMANGLE_COMPONENT_VOLATILE_THIS:
							case DEMANGLE_COMPONENT_CONST_THIS:
							case DEMANGLE_COMPONENT_REFERENCE_THIS:
							case DEMANGLE_COMPONENT_RVALUE_REFERENCE_THIS: {
								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.left->u.s_binary.left->u.s_binary.left, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string qual_name{unmangled_buffer};
								std::free(unmangled_buffer);

								if(ignored_qual(qual_name)) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.left->u.s_binary.left->u.s_binary.right, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string func_name{unmangled_buffer};
								std::free(unmangled_buffer);

								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								func_name += unmangled_buffer;
								std::free(unmangled_buffer);

								switch(component->u.s_binary.left->type) {
									case DEMANGLE_COMPONENT_RESTRICT_THIS:
									func_name += " restrict"sv;
									break;
									case DEMANGLE_COMPONENT_VOLATILE_THIS:
									func_name += " volatile"sv;
									break;
									case DEMANGLE_COMPONENT_CONST_THIS:
									func_name += " const"sv;
									break;
									case DEMANGLE_COMPONENT_REFERENCE_THIS:
									func_name += " &"sv;
									break;
									case DEMANGLE_COMPONENT_RVALUE_REFERENCE_THIS:
									func_name += " &&"sv;
									break;
									default: break;
								}

								auto this_qual_it{qualifications.find(qual_name)};
								if(this_qual_it == qualifications.end()) {
									this_qual_it = qualifications.emplace(std::move(qual_name), new class_info{}).first;
								}

								std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};

								std::ptrdiff_t offset{static_cast<std::ptrdiff_t>(sym.st_value)};
								info->offset_ = offset;
								info->size_ = static_cast<std::size_t>(sym.st_size);
								info->resolve(base);

								name_it = this_qual_it->second->names.insert_or_assign(std::move(func_name), std::move(info)).first;
								qual_it = this_qual_it;

								offset_map.emplace(offset, pair_t{qual_it,name_it});

								return true;
							}
							case DEMANGLE_COMPONENT_QUAL_NAME: {
								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.left->u.s_binary.left, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string qual_name{unmangled_buffer};
								std::free(unmangled_buffer);

								if(ignored_qual(qual_name)) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.left->u.s_binary.right, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string func_name{unmangled_buffer};
								std::free(unmangled_buffer);

								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								func_name += unmangled_buffer;
								std::free(unmangled_buffer);

								std::unique_ptr<qualification_info::name_info> info;

								auto this_qual_it{qualifications.find(qual_name)};
								if(this_qual_it == qualifications.end()) {
									this_qual_it = qualifications.emplace(std::move(qual_name), new class_info{}).first;
								}

								switch(component->u.s_binary.left->u.s_binary.right->type) {
									case DEMANGLE_COMPONENT_CTOR: {
										gnu_v3_ctor_kinds kind{component->u.s_binary.left->u.s_binary.right->u.s_ctor.kind};
										switch(kind) {
											case gnu_v3_complete_object_ctor: {
												func_name += " (complete)"sv;
											} break;
											case gnu_v3_base_object_ctor: {
												func_name += " (base)"sv;
											} break;
											case gnu_v3_complete_object_allocating_ctor: {
												func_name += " (complete allocating)"sv;
											} break;
											case gnu_v3_unified_ctor: {
												func_name += " (unified)"sv;
											} break;
											case gnu_v3_object_ctor_group: {
												func_name += " (group)"sv;
											} break;
										#ifndef __clang__
											default: break;
										#endif
										}

										class_info::ctor_info *ctor_info{new class_info::ctor_info};
										ctor_info->kind = kind;

										info.reset(ctor_info);
									} break;
									case DEMANGLE_COMPONENT_DTOR: {
										gnu_v3_dtor_kinds kind{component->u.s_binary.left->u.s_binary.right->u.s_dtor.kind};
										switch(kind) {
											case gnu_v3_deleting_dtor: {
												func_name += " (deleting)"sv;
											} break;
											case gnu_v3_complete_object_dtor: {
												func_name += " (complete)"sv;
											} break;
											case gnu_v3_base_object_dtor: {
												func_name += " (base)"sv;
											} break;
											case gnu_v3_unified_dtor: {
												func_name += " (unified)"sv;
											} break;
											case gnu_v3_object_dtor_group: {
												func_name += " (group)"sv;
											} break;
										#ifndef __clang__
											default: break;
										#endif
										}

										class_info::dtor_info *dtor_info{new class_info::dtor_info};
										dtor_info->kind = kind;

										info.reset(dtor_info);
									} break;
									default: {
										qualification_info::name_info *name_info{new qualification_info::name_info};

										info.reset(name_info);
									} break;
								}

								std::ptrdiff_t offset{static_cast<std::ptrdiff_t>(sym.st_value)};
								info->offset_ = offset;
								info->size_ = static_cast<std::size_t>(sym.st_size);
								info->resolve(base);

								name_it = this_qual_it->second->names.insert_or_assign(std::move(func_name), std::move(info)).first;
								qual_it = this_qual_it;

								if(dynamic_cast<class_info::ctor_info *>(info.get()) == nullptr) {
									offset_map.emplace(offset, pair_t{qual_it,name_it});
								}

								return true;
							}
							case DEMANGLE_COMPONENT_TEMPLATE: {
								switch(component->u.s_binary.left->u.s_binary.left->type) {
									case DEMANGLE_COMPONENT_NAME: break;
									default: {
										name_it = global_qual.names.end();
										qual_it = qualifications.end();
										return false;
									}
								}
								[[fallthrough]];
							}
							case DEMANGLE_COMPONENT_NAME: {
								unmangled_buffer = cplus_demangle_print(base_demangle_flags, component, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string func_name{unmangled_buffer};
								std::free(unmangled_buffer);

								std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
								info->offset_ = static_cast<std::ptrdiff_t>(sym.st_value);
								info->size_ = static_cast<std::size_t>(sym.st_size);
								info->resolve(base);

								name_it = global_qual.names.insert_or_assign(std::move(func_name), std::move(info)).first;
								qual_it = qualifications.end();
								return true;
							}
							default: {
								name_it = global_qual.names.end();
								qual_it = qualifications.end();
								return false;
							}
						}
					}
					default: {
						name_it = global_qual.names.end();
						qual_it = qualifications.end();
						return false;
					}
				}
			}
			default: {
				name_it = global_qual.names.end();
				qual_it = qualifications.end();
				return false;
			}
		}
	}

	std::ptrdiff_t symbol_cache::uncached_find_mangled_func(const std::filesystem::path &path, std::string_view search) noexcept
	{
		int fd{open(path.c_str(), O_RDONLY)};
		if(fd < 0) {
			return 0;
		}

		Elf *elf{elf_begin(fd, ELF_C_READ, nullptr)};
		if(!elf) {
			return 0;
		}

		if(elf_kind(elf) != ELF_K_ELF || gelf_getclass(elf) == ELFCLASSNONE) {
			elf_end(elf);
			return 0;
		}

		std::ptrdiff_t offset{0};

		GElf_Shdr scn_hdr;
		GElf_Sym sym;

		Elf_Scn *scn{elf_nextscn(elf, nullptr)};
		while(scn) {
			struct scope_nextscn final {
				inline scope_nextscn(Elf *elf_, Elf_Scn *&scn_) noexcept
					: elf{elf_}, scn{scn_} {}
				inline ~scope_nextscn() noexcept
				{ scn = elf_nextscn(elf, scn); }
			private:
				Elf *elf;
				Elf_Scn *&scn;
			};

			scope_nextscn snscn{elf, scn};

			if(!gelf_getshdr(scn, &scn_hdr)) {
				continue;
			}

			switch(scn_hdr.sh_type) {
				case SHT_SYMTAB: break;
				default: continue;
			}

			Elf_Data *scn_data{elf_getdata(scn, nullptr)};

			std::size_t count{static_cast<std::size_t>(scn_hdr.sh_size) / static_cast<std::size_t>(scn_hdr.sh_entsize)};
			for(std::size_t i{0}; i < count; ++i) {
				gelf_getsym(scn_data, static_cast<int>(i), &sym);

				switch(GELF_ST_BIND(sym.st_info)) {
					case STB_LOCAL: break;
					default: continue;
				}

				switch(GELF_ST_TYPE(sym.st_info)) {
					case STT_FUNC: break;
					default: continue;
				}

				switch(GELF_ST_VISIBILITY(sym.st_other)) {
					case STV_DEFAULT: break;
					default: continue;
				}

				std::string_view name_mangled{elf_strptr(elf, scn_hdr.sh_link, sym.st_name)};
				if(name_mangled.empty()) {
					continue;
				}

				if(search == name_mangled) {
					offset = static_cast<std::ptrdiff_t>(sym.st_value);
					break;
				}
			}

			if(offset != 0) {
				break;
			}
		}

		elf_end(elf);

		close(fd);

		return offset;
	}
}
