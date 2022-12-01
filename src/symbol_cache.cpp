#include "symbol_cache.hpp"
#include "gsdk.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include <cstring>

#pragma push_macro("HAVE_DECL_BASENAME")
#undef HAVE_DECL_BASENAME
#define HAVE_DECL_BASENAME 1
#include <libiberty/demangle.h>
#pragma pop_macro("HAVE_DECL_BASENAME")

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
	{ addr_ = (static_cast<unsigned char *>(base) + offset); }

	void symbol_cache::class_info::ctor_info::resolve(void *base) noexcept
	{
		generic_func_t temp{reinterpret_cast<generic_func_t>(static_cast<unsigned char *>(base) + offset)};
		mfp_ = mfp_from_func<void, empty_class>(temp);
	}

	void symbol_cache::class_info::dtor_info::resolve(void *base) noexcept
	{
		generic_func_t temp{reinterpret_cast<generic_func_t>(static_cast<unsigned char *>(base) + offset)};
		mfp_ = mfp_from_func<void, empty_class>(temp);
	}

	void symbol_cache::class_info::vtable_info::resolve(void *base) noexcept
	{ vtable = vtable_from_addr<empty_class>(static_cast<unsigned char *>(base) + offset); }

	void symbol_cache::qualification_info::resolve(void *base) noexcept
	{
		for(auto &it : names) {
			it.second.resolve(base);
		}
	}

	void symbol_cache::class_info::resolve(void *base) noexcept
	{
		qualification_info::resolve(base);

		vtable.resolve(base);

		for(ctor_info &info : ctors) {
			info.resolve(base);
		}

		for(dtor_info &info : dtors) {
			info.resolve(base);
		}
	}

	void symbol_cache::resolve(void *base) noexcept
	{
		for(auto &it : qualifications) {
			it.second.resolve(base);
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

				switch(GELF_ST_BIND(sym.st_info)) {
					case STB_LOCAL: break;
					default: continue;
				}

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

				constexpr int base_demangle_flags{DMGL_GNU_V3|DMGL_PARAMS};

				struct scope_free final {
					inline scope_free(void *mem_) noexcept
						: mem{mem_} {}
					inline ~scope_free() noexcept {
						if(mem) {
							free(mem);
						}
					}
				private:
					void *mem;
				};

				void *component_mem{nullptr};
				demangle_component *component{cplus_demangle_v3_components(name_mangled.data(), base_demangle_flags, &component_mem)};
				scope_free sfcm{component_mem};
				if(!component) {
					qualification_info::name_info info;
					info.offset = static_cast<std::ptrdiff_t>(sym.st_value);
					info.size_ = static_cast<std::size_t>(sym.st_size);
					info.resolve(base);
					global_qual.names.emplace(name_mangled, std::move(info));
					continue;
				}

				switch(component->type) {
					case DEMANGLE_COMPONENT_NAME:
					case DEMANGLE_COMPONENT_TYPED_NAME: break;
					default: continue;
				}

				std::size_t allocated;
				char *unmangled_buffer;

				constexpr std::size_t guessed_name_length{128};

				switch(component->type) {
					case DEMANGLE_COMPONENT_NAME: {
						unmangled_buffer = cplus_demangle_print(base_demangle_flags, component, guessed_name_length, &allocated);
						if(!unmangled_buffer) {
							continue;
						}

						std::string name_unmangled{unmangled_buffer};
						free(unmangled_buffer);

						qualification_info::name_info info;
						info.offset = static_cast<std::ptrdiff_t>(sym.st_value);
						info.size_ = static_cast<std::size_t>(sym.st_size);
						info.resolve(base);

						global_qual.names.emplace(std::move(name_unmangled), std::move(info));
					} break;
					case DEMANGLE_COMPONENT_TYPED_NAME: {
						switch(component->u.s_binary.right->type) {
							case DEMANGLE_COMPONENT_FUNCTION_TYPE: {
								switch(component->u.s_binary.left->type) {
									case DEMANGLE_COMPONENT_QUAL_NAME: {
										unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.left->u.s_binary.left, guessed_name_length, &allocated);
										if(!unmangled_buffer) {
											continue;
										}

										std::string qual_name{unmangled_buffer};
										free(unmangled_buffer);

										unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.left->u.s_binary.right, guessed_name_length, &allocated);
										if(!unmangled_buffer) {
											continue;
										}

										std::string func_name{unmangled_buffer};
										free(unmangled_buffer);

										unmangled_buffer = cplus_demangle_print(base_demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
										if(!unmangled_buffer) {
											continue;
										}

										func_name += unmangled_buffer;
										free(unmangled_buffer);

										auto qual_it{qualifications.find(qual_name)};
										if(qual_it == qualifications.end()) {
											qual_it = qualifications.emplace(std::move(qual_name), qualification_info{}).first;
										}

										auto func_it{qual_it->second.names.find(func_name)};
										if(func_it != qual_it->second.names.end()) {
											continue;
										}

										qualification_info::name_info info;
										info.offset = static_cast<std::ptrdiff_t>(sym.st_value);
										info.size_ = static_cast<std::size_t>(sym.st_size);
										info.resolve(base);

										qual_it->second.names.emplace(std::move(func_name), std::move(info));
									} break;
									case DEMANGLE_COMPONENT_NAME: {
										unmangled_buffer = cplus_demangle_print(base_demangle_flags, component, guessed_name_length, &allocated);
										if(!unmangled_buffer) {
											continue;
										}

										std::string func_name{unmangled_buffer};
										free(unmangled_buffer);

										qualification_info::name_info info;
										info.offset = static_cast<std::ptrdiff_t>(sym.st_value);
										info.size_ = static_cast<std::size_t>(sym.st_size);
										info.resolve(base);

										global_qual.names.emplace(std::move(func_name), std::move(info));
									} break;
									default: continue;
								}
							} break;
							default: continue;
						}
					} break;
					default: continue;
				}
			}
		}

		elf_end(elf);

		return true;
	}
}
