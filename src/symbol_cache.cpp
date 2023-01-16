#include "symbol_cache.hpp"

#ifndef __VMOD_COMPILING_VTABLE_DUMPER
#include "gsdk/config.hpp"
#endif

#include <string_view>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#ifndef __VMOD_COMPILING_SYMBOL_TOOL
#include "filesystem.hpp"
#include "gsdk.hpp"
#include "main.hpp"
#include <emmintrin.h>
#include <smmintrin.h>
#endif

namespace vmod
{
	symbol_cache::qualification_info::~qualification_info() noexcept {}
	symbol_cache::class_info::~class_info() noexcept {}
	symbol_cache::class_info::dtor_info::~dtor_info() noexcept {}

#ifndef __VMOD_COMPILING_SYMBOL_TOOL
	std::filesystem::path symbol_cache::yamls_dir;
#endif

#ifndef GSDK_NO_SYMBOLS
	int symbol_cache::demangle_flags{DMGL_GNU_V3|DMGL_PARAMS|DMGL_VERBOSE|DMGL_TYPES|DMGL_ANSI};
#endif

	bool symbol_cache::initialize() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(elf_version(EV_CURRENT) == EV_NONE) {
			return false;
		}

	#ifndef GSDK_NO_SYMBOLS
		cplus_demangle_set_style(gnu_v3_demangling);
	#endif

	#ifndef __VMOD_COMPILING_SYMBOL_TOOL
		yamls_dir = main::instance().root_dir();
		yamls_dir /= "syms"sv;
	#endif

		return true;
	}

	bool symbol_cache::load(const std::filesystem::path &path, unsigned char *base) noexcept
	{
		using namespace std::literals::string_view_literals;
		using namespace std::literals::string_literals;

		std::filesystem::path ext{path.extension()};

		if(ext == ".so"sv) {
		#ifndef __VMOD_COMPILING_SYMBOL_TOOL
			#ifndef GSDK_NO_SYMBOLS
			if(!symbols_available)
			#endif
			{
				if(!read_elf_info(path)) {
					return false;
				}

				std::filesystem::path yaml_dir{yamls_dir};
				yaml_dir /= path.filename();
				yaml_dir.replace_extension();

				if(!read_yamls(yaml_dir, base)) {
					return false;
				}

			#ifndef GSDK_NO_SYMBOLS
				resolve_vtables(base, true);
			#endif

				return true;
			}
			#ifndef GSDK_NO_SYMBOLS
			else {
			#endif
		#endif
			#ifndef GSDK_NO_SYMBOLS
				int fd{open(path.c_str(), O_RDONLY)};
				if(fd < 0) {
					int err{errno};
					err_str = strerror(err);
					return false;
				}

				if(!read_elf_symbols(fd, base)) {
					close(fd);
					return false;
				}

				resolve_vtables(base, true);

				close(fd);
				return true;
			#elif defined __VMOD_COMPILING_SYMBOL_TOOL
				err_str = "invalid extension"s;
				return false;
			#endif
		#if !defined __VMOD_COMPILING_SYMBOL_TOOL && !defined GSDK_NO_SYMBOLS
			}
		#endif
		}
	#ifdef __VMOD_COMPILING_VTABLE_DUMPER
		else if(ext == ".dylib"sv) {
			auto buffer{llvm::MemoryBuffer::getFile(path.native(), false, false)};
			if(!buffer || !*buffer) {
				err_str = "failed to open '"s;
				err_str += path;
				err_str += '\'';
				return false;
			}

			auto obj_expect{llvm::object::MachOObjectFile::create(*(*buffer), true, false)};
			if(!obj_expect || !*obj_expect) {
				err_str = llvm::toString(obj_expect.takeError());
				return false;
			}

			auto &&obj_ptr{*obj_expect};

			base = reinterpret_cast<unsigned char *>(const_cast<char *>((*buffer)->getBufferStart()));

			if(!read_macho(*obj_ptr, base)) {
				return false;
			}

			resolve_vtables(base, false);

			return true;
		}
	#endif
		else {
			err_str = "invalid extension"s;
			return false;
		}
	}

	symbol_cache::qualification_info::name_info &symbol_cache::qualification_info::name_info::operator=(name_info &&other) noexcept
	{
	#ifndef __VMOD_COMPILING_SYMBOL_TOOL
		#ifndef GSDK_NO_SYMBOLS
		if(other.type_ == type::offset) {
			offset_ = std::move(other.offset_);
		} else
		#endif
		{
			bytes_info = std::move(other.bytes_info);
		}
	#endif
		vindex = other.vindex;
	#ifndef __VMOD_COMPILING_SYMBOL_TOOL
		names = std::move(other.names);
		#ifndef GSDK_NO_SYMBOLS
		size_ = other.size_;
		#endif
		mfp_ = std::move(other.mfp_);
	#endif
		return *this;
	}

	symbol_cache::qualification_info::name_info::~name_info() noexcept
	{
	#if !defined __VMOD_COMPILING_SYMBOL_TOOL && !defined GSDK_NO_SYMBOLS
		if(type_ == type::bytes) {
			bytes_info.~bytes_info_t();
		}
	#endif
	}

#if !defined __VMOD_COMPILING_SYMBOL_TOOL
	void symbol_cache::qualification_info::name_info::resolve_from_base(unsigned char *base) noexcept
	{
	#ifndef GSDK_NO_SYMBOLS
		if(type_ == type::offset) {
			resolve_absolute(base + offset_);
		} else
	#endif
		{
			debugtrap();
		}
	}

	void symbol_cache::qualification_info::name_info::resolve_absolute(unsigned char *addr) noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		mfp_.addr = reinterpret_cast<generic_plain_mfp_t>(addr);
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
		mfp_.adjustor = 0;
	}

	void symbol_cache::class_info::ctor_info::resolve_absolute(unsigned char *addr) noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		generic_plain_mfp_t temp{reinterpret_cast<generic_plain_mfp_t>(addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
		mfp_ = mfp_from_func<void, generic_object_t>(temp);
	}

	void symbol_cache::class_info::dtor_info::resolve_absolute(unsigned char *addr) noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		generic_plain_mfp_t temp{reinterpret_cast<generic_plain_mfp_t>(addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
		mfp_ = mfp_from_func<void, generic_object_t>(temp);
	}
#endif

#ifndef GSDK_NO_SYMBOLS
	void symbol_cache::class_info::vtable_info::resolve_from_base(unsigned char *base) noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-align"
	#endif
		prefix = reinterpret_cast<__cxxabiv1::vtable_prefix *>(base + offset);
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
	}

	void symbol_cache::resolve_vtables(unsigned char *base, bool elf) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(offset_map.empty()) {
			return;
		}

		for(auto &it : qualifications) {
			class_info *info{dynamic_cast<class_info *>(it.second.get())};
			if(!info) {
				continue;
			}

			std::size_t vtable_size{info->vtable_.size_};
			if(vtable_size == 0) {
				continue;
			}

			info->vtable_.funcs_.resize(vtable_size, {qualifications.end(),info->names.end()});

			generic_vtable_t vtable{vtable_from_prefix(info->vtable_.prefix)};
			for(std::size_t i{0}; i < vtable_size; ++i) {
				generic_plain_mfp_t func{vtable[i]};
				std::uint64_t off{elf ? (reinterpret_cast<std::uint64_t>(func) - reinterpret_cast<std::uint64_t>(base)) : reinterpret_cast<std::uint64_t>(func)};
				auto map_it{offset_map.find(off)};
				if(map_it != offset_map.end()) {
					map_it->second.func->second->vindex = i;
					info->vtable_.funcs_[i] = map_it->second;
				}
			}
		}

		offset_map.clear();
	}
#endif

#ifdef __VMOD_COMPILING_VTABLE_DUMPER
	bool symbol_cache::read_macho(llvm::object::MachOObjectFile &obj, unsigned char *base) noexcept
	{
		using namespace std::literals::string_view_literals;

		for(auto it : obj.symbols()) {
			auto name_mangled{it.getName()};
			if(!name_mangled || name_mangled->empty()) {
				continue;
			}

			using Type_t = llvm::object::SymbolRef::Type;

			auto type{it.getType()};
			if(!type || (*type != Type_t::ST_Data && *type != Type_t::ST_Debug)) {
				continue;
			}

			auto value{it.getValue()};
			if(!value || *value == 0) {
				continue;
			}

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

			auto name_mangled_str{name_mangled->str()};
			if(name_mangled_str[0] == '_') {
				name_mangled_str.erase(name_mangled_str.begin());
			}

			void *component_mem{nullptr};
			demangle_component *component{cplus_demangle_v3_components(name_mangled_str.c_str(), demangle_flags, &component_mem)};
			scope_free sfcm{component_mem};

			std::size_t size{0};

			if(it != obj.symbol_end()) {
				auto idx{obj.getSymbolIndex(it.getRawDataRefImpl())};
				auto next{obj.getSymbolByIndex(static_cast<unsigned int>(idx)+1)};
				if(next != obj.symbol_end()) {
					auto next_value{next->getValue()};
					if(next_value && *next_value != 0 && *next_value > *value) {
						size = static_cast<std::size_t>(*next_value - *value);
					}
				}
			}

			basic_sym_t basic_sym{static_cast<std::uint64_t>(*value), size};

			qualifications_t::iterator tmp_qual_it;
			qualification_info::names_t::iterator tmp_name_it;
			handle_component(name_mangled_str, component, tmp_qual_it, tmp_name_it, std::move(basic_sym), base, false);
		}

		return true;
	}
#endif

#ifndef __VMOD_COMPILING_SYMBOL_TOOL
	bool symbol_cache::read_elf_info(const std::filesystem::path &path) noexcept
	{
		int fd{open(path.c_str(), O_RDONLY)};
		if(fd < 0) {
			int err{errno};
			err_str = strerror(err);
			return false;
		}

		if(!read_elf_info(fd)) {
			close(fd);
			return false;
		}

		close(fd);
		return true;
	}

	bool symbol_cache::read_elf_info(int fd) noexcept
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

		std::size_t num_phdrs{0};
		elf_getphdrnum(elf, &num_phdrs);

		std::size_t pagesize{static_cast<std::size_t>(sysconf(_SC_PAGESIZE))};

		GElf_Phdr phdr;

		for(std::size_t i{0}; i < num_phdrs; ++i) {
			if(!gelf_getphdr(elf, static_cast<int>(i), &phdr)) {
				continue;
			}

			if(phdr.p_type != PT_LOAD) {
				continue;
			}

			if(phdr.p_flags != (PF_X|PF_R)) {
				continue;
			}

			memory_size = align_up(phdr.p_filesz, pagesize);
			break;
		}

		GElf_Shdr scn_hdr;

		std::size_t strndx{0};
		elf_getshdrstrndx(elf, &strndx);

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
				case SHT_PROGBITS: break;
				default: continue;
			}

			std::string_view name{elf_strptr(elf, strndx, scn_hdr.sh_name)};
			if(name.empty()) {
				continue;
			}

			if(name != ".data"sv) {
				continue;
			}

			data_offset = scn_hdr.sh_offset;
			data_size = scn_hdr.sh_size;
			break;
		}

		elf_end(elf);

		return true;
	}
#endif

#ifndef GSDK_NO_SYMBOLS
	bool symbol_cache::read_elf_symbols(int fd, unsigned char *base) noexcept
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

				basic_sym_t basic_sym{static_cast<std::uint64_t>(sym.st_value), static_cast<std::size_t>(sym.st_size)};

				if(bind == STB_GLOBAL) {
					if(name_mangled == "__cxa_pure_virtual"sv ||
						name_mangled == "__cxa_deleted_virtual"sv) {
						std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
						std::uint64_t offset{basic_sym.off};
					#ifndef __VMOD_COMPILING_SYMBOL_TOOL
						info->set_offset(offset);
						info->size_ = basic_sym.size;
						info->resolve_from_base(base);
					#endif
						auto name_it{global_qual.names.emplace(name_mangled, std::move(info)).first};
						offset_map.emplace(offset, pair_t{qualifications.end(),name_it});
					}
					continue;
				}

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
				demangle_component *component{cplus_demangle_v3_components(name_mangled.data(), demangle_flags, &component_mem)};
				scope_free sfcm{component_mem};

				qualifications_t::iterator tmp_qual_it;
				qualification_info::names_t::iterator tmp_name_it;
				handle_component(name_mangled, component, tmp_qual_it, tmp_name_it, std::move(basic_sym), base, true);
			}

			break;
		}

		elf_end(elf);

		return true;
	}
#endif

#ifndef __VMOD_COMPILING_SYMBOL_TOOL
	bool symbol_cache::read_yaml(const std::filesystem::path &path, unsigned char *base) noexcept
	{
		using namespace std::literals::string_view_literals;
		using namespace std::literals::string_literals;

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

		std::size_t size{0};
		std::unique_ptr<unsigned char[]> yaml_data{read_file(path, size)};
		if(size == 0) {
			return false;
		}

		yaml_parser_set_input_string(&parser, yaml_data.get(), size);

		while(true) {
			yaml_document_t temp_doc{};
			if(yaml_parser_load(&parser, &temp_doc) != 1) {
				return false;
			}

			yaml_node_t *root{yaml_document_get_root_node(&temp_doc)};
			if(!root) {
				break;
			}

			if(root->type != YAML_MAPPING_NODE ||
				std::strcmp(reinterpret_cast<const char *>(root->tag), YAML_MAP_TAG) != 0) {
				err_str = "root is not a map"s;
				return false;
			}

			if(!read_map_node(temp_doc, root, {}, base)) {
				return false;
			}
		}

		return true;
	}

	namespace detail
	{
		static bool is_string_node(yaml_node_t *node) noexcept
		{
			if(node->type != YAML_SCALAR_NODE) {
				return false;
			}

			switch(node->data.scalar.style) {
				case YAML_PLAIN_SCALAR_STYLE:
				case YAML_SINGLE_QUOTED_SCALAR_STYLE:
				case YAML_DOUBLE_QUOTED_SCALAR_STYLE: {
					if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_STR_TAG) == 0) {
						return true;
					} else {
						return false;
					}
				}
				default:
				return false;
			}
		}

		static bool is_null_node(yaml_node_t *node) noexcept
		{
			if(node->type != YAML_SCALAR_NODE) {
				return false;
			}

			switch(node->data.scalar.style) {
				case YAML_PLAIN_SCALAR_STYLE:{
					if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_STR_TAG) == 0) {
						if(std::strcmp(reinterpret_cast<const char *>(node->data.scalar.value), "null") == 0) {
							return true;
						} else {
							return false;
						}
					} else {
						return false;
					}
				}
				case YAML_LITERAL_SCALAR_STYLE: {
					if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_NULL_TAG) == 0) {
						return true;
					} else {
						return false;
					}
				}
				default:
				return false;
			}
		}

		template <typename T>
		static bool read_int_node(yaml_node_t *node, T &value, int base = 10) noexcept
		{
			if(node->type != YAML_SCALAR_NODE) {
				return false;
			}

			switch(node->data.scalar.style) {
				case YAML_PLAIN_SCALAR_STYLE: {
					if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_STR_TAG) == 0) {
						if(std::strncmp(reinterpret_cast<const char *>(node->data.scalar.value), "null", node->data.scalar.length) == 0) {
							value = static_cast<T>(static_cast<std::make_unsigned_t<T>>(-1));
							return true;
						} else {
							const char *begin{reinterpret_cast<const char *>(node->data.scalar.value)};

							std::size_t len{node->data.scalar.length};

							if(base == 16) {
								if(len > 2) {
									if(begin[0] == '0' &&
										(begin[1] == 'x' || begin[1] == 'X')) {
										begin += 2;
										len -= 2;
									}
								}
							}

							const char *end{begin + len};

							std::from_chars_result fc_res{std::from_chars(begin, end, value, base)};

							if(fc_res.ec != std::error_code{} ||
								fc_res.ptr != end) {
								return false;
							}

							return true;
						}
					} else {
						return false;
					}
				}
				case YAML_LITERAL_SCALAR_STYLE: {
					if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_NULL_TAG) == 0) {
						value = 0;
						return true;
					} else if(std::strcmp(reinterpret_cast<const char *>(node->tag), YAML_INT_TAG) == 0) {
						switch(node->data.scalar.length) {
							case sizeof(char):
							value = static_cast<T>(*reinterpret_cast<char *>(node->data.scalar.value));
							return true;
							case sizeof(short):
							value = static_cast<T>(*reinterpret_cast<short *>(node->data.scalar.value));
							return true;
							case sizeof(int):
							value = static_cast<T>(*reinterpret_cast<int *>(node->data.scalar.value));
							return true;
							case sizeof(long long):
							value = static_cast<T>(*reinterpret_cast<long long *>(node->data.scalar.value));
							return true;
							default:
							return false;
						}
					} else {
						return false;
					}
				}
				default:
				return false;
			}
		}

		using bytes_info_t = symbol_cache::qualification_info::name_info::bytes_info_t;
		using null_or_byte_t = symbol_cache::qualification_info::name_info::null_or_byte_t;

		static unsigned char *resolve_bytes(unsigned char *base, std::uint64_t memory_size, const bytes_info_t &bytes) noexcept
		{
			std::size_t num_bytes{bytes.size()};

			unsigned char *begin{base};
			unsigned char *end{base + (memory_size - num_bytes)};

			const unsigned char *bytes_begin{reinterpret_cast<const unsigned char *>(bytes.data())};

			if(!bytes.has_null()) {
				for(unsigned char *it{begin}; it != end; ++it) {
					if(__builtin_memcmp(it, bytes_begin, num_bytes) == 0) {
						return it;
					}
				}

				return nullptr;
			} else {
				for(unsigned char *it{begin}; it != end; ++it) {
					bool found{true};

					std::size_t last_null_pos{0};
					for(std::size_t null_pos : bytes.null_positions) {
						std::size_t diff{null_pos-last_null_pos};
						if(__builtin_memcmp(it+last_null_pos, bytes_begin+last_null_pos, diff) != 0) {
							found = false;
							break;
						}
						last_null_pos = null_pos;
					}

					if(found) {
						if(__builtin_memcmp(it+last_null_pos, bytes_begin+last_null_pos, num_bytes-last_null_pos) == 0) {
							return it;
						}
					}
				}

				return nullptr;
			}
		}
	}

	bool symbol_cache::read_final_map_node(yaml_document_t &doc, yaml_node_t *node, std::string &&qual, std::string &&name, unsigned char *base) noexcept
	{
		using namespace std::literals::string_view_literals;
		using namespace std::literals::string_literals;

		std::unordered_map<std::string, yaml_node_t *> members;

		auto build_err_str{
			[this,&qual,name](std::string_view pre, std::string_view post) noexcept -> void {
				if(!pre.empty()) {
					err_str += pre;
					err_str += ' ';
				}
				err_str += '\'';
				if(!qual.empty()) {
					err_str += qual;
					err_str += "::"sv;
				}
				err_str += name;
				err_str += "' "sv;
				err_str += post;
			}
		};

		for(yaml_node_pair_t *pair{node->data.mapping.pairs.start}; pair != node->data.mapping.pairs.top; ++pair) {
			yaml_node_t *key_node{yaml_document_get_node(&doc, pair->key)};
			if(!detail::is_string_node(key_node)) {
				build_err_str("key in"sv, "is not a string"sv);
				return false;
			}

			yaml_node_t *value_node{yaml_document_get_node(&doc, pair->value)};

			std::string keystr{reinterpret_cast<const char *>(key_node->data.scalar.value), key_node->data.scalar.length};

			members.emplace(std::move(keystr), value_node);
		}

		auto method_node_it{members.find("method"s)};
		if(method_node_it == members.end()) {
			build_err_str({}, "is missing method"sv);
			return false;
		}

		if(!detail::is_string_node(method_node_it->second)) {
			build_err_str({}, "method is not a string"sv);
			return false;
		}

		std::string_view methodstr{reinterpret_cast<const char *>(method_node_it->second->data.scalar.value), method_node_it->second->data.scalar.length};

		if(methodstr.compare(0, 9, "byte_scan"sv) == 0) {
			auto bytes_node_it{members.find("bytes"s)};
			if(bytes_node_it == members.end()) {
				build_err_str({}, "is missing bytes"sv);
				return false;
			}

			detail::bytes_info_t bytes;

			switch(bytes_node_it->second->type) {
				case YAML_SEQUENCE_NODE: {
					if(std::strcmp(reinterpret_cast<const char *>(bytes_node_it->second->tag), YAML_SEQ_TAG) != 0) {
						build_err_str({}, "bytes is not a array"sv);
						return false;
					}

					for(yaml_node_item_t *item{bytes_node_it->second->data.sequence.items.start}; item != bytes_node_it->second->data.sequence.items.top; ++item) {
						yaml_node_t *value_node{yaml_document_get_node(&doc, *item)};

						if(detail::is_null_node(value_node)) {
							bytes.emplace_back(nullptr);
						} else {
							unsigned char byte;
							if(!detail::read_int_node<unsigned char>(value_node, byte, 16)) {
								build_err_str({}, "bytes has a non byte value"sv);
								return false;
							}

							bytes.emplace_back(byte);
						}
					}
				} break;
				default: {
					build_err_str({}, "bytes is not a array"sv);
					return false;
				}
			}

			unsigned char *addr{nullptr};

			if(methodstr.ends_with("_mem"sv)) {
				addr = detail::resolve_bytes(base, memory_size, bytes);
			} else if(methodstr.ends_with("_data"sv)) {
				addr = detail::resolve_bytes(base + data_offset, data_size, bytes);
			} else {
				build_err_str({}, "unknown method '"sv);
				err_str += methodstr;
				err_str += '\'';
				return false;
			}

			auto offset_node_it{members.find("offset"s)};
			if(offset_node_it != members.end()) {
				std::uint64_t off;
				if(!detail::read_int_node<std::uint64_t>(offset_node_it->second, off, 16)) {
					build_err_str({}, "invalid offset"sv);
					return false;
				}

				addr += off;
			}

			if(!addr) {
				debugtrap();
				build_err_str({}, "address not found"sv);
				return false;
			}

			if(qual.empty()) {
				std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
				info->set_bytes(std::move(bytes));
				info->resolve_absolute(addr);
				global_qual.names.emplace(std::move(name), std::move(info));
			} else {
				auto this_qual_it{qualifications.find(qual)};
				if(this_qual_it == qualifications.end()) {
					this_qual_it = qualifications.emplace(std::move(qual), new class_info{}).first;
				}

				std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
				info->set_bytes(std::move(bytes));
				info->resolve_absolute(addr);
				this_qual_it->second->names.emplace(std::move(name), std::move(info));
			}

			return true;
		} else {
			build_err_str({}, "unknown method '"sv);
			err_str += methodstr;
			err_str += '\'';
			return false;
		}
	}

	bool symbol_cache::read_array_node(yaml_document_t &doc, yaml_node_t *node, std::string_view name, unsigned char *base) noexcept
	{
		using namespace std::literals::string_view_literals;
		using namespace std::literals::string_literals;

		auto build_err_str{
			[this,name](std::string_view post) noexcept -> void {
				err_str = "value in '"s;
				err_str += name;
				err_str += "' "sv;
				err_str += post;
			}
		};

		for(yaml_node_item_t *item{node->data.sequence.items.start}; item != node->data.sequence.items.top; ++item) {
			yaml_node_t *value_node{yaml_document_get_node(&doc, *item)};

			switch(value_node->type) {
				case YAML_MAPPING_NODE: {
					if(std::strcmp(reinterpret_cast<const char *>(value_node->tag), YAML_MAP_TAG) != 0) {
						build_err_str("is not a map"sv);
						return false;
					}

					if(!read_map_node(doc, value_node, name, base)) {
						return false;
					}
				} break;
				default: {
					build_err_str("is not a map"sv);
					return false;
				}
			}
		}

		return true;
	}

	bool symbol_cache::read_map_node(yaml_document_t &doc, yaml_node_t *node, std::string_view name, unsigned char *base) noexcept
	{
		using namespace std::literals::string_view_literals;
		using namespace std::literals::string_literals;

		for(yaml_node_pair_t *pair{node->data.mapping.pairs.start}; pair != node->data.mapping.pairs.top; ++pair) {
			yaml_node_t *key_node{yaml_document_get_node(&doc, pair->key)};
			if(!detail::is_string_node(key_node)) {
				debugtrap();
				err_str = "value in '"s;
				err_str += name.empty() ? "root"sv : name;
				err_str += "' is not a string"sv;
				return false;
			}

			std::string_view keystr{reinterpret_cast<const char *>(key_node->data.scalar.value), key_node->data.scalar.length};

			auto build_err_str{
				[this,keystr,name](std::string_view post) noexcept -> void {
					err_str = "'"s;
					err_str += keystr;
					err_str += "' in '"sv;
					err_str += name.empty() ? "root"sv : name;
					err_str += "' "sv;
					err_str += post;
				}
			};

			yaml_node_t *value_node{yaml_document_get_node(&doc, pair->value)};
			switch(value_node->type) {
				case YAML_MAPPING_NODE: {
					if(std::strcmp(reinterpret_cast<const char *>(value_node->tag), YAML_MAP_TAG) != 0) {
						build_err_str("is not a map"sv);
						return false;
					}

					std::string tmpqual{name};
					std::string tmpname{keystr};

					if(!read_final_map_node(doc, value_node, std::move(tmpqual), std::move(tmpname), base)) {
						return false;
					}
				} break;
				case YAML_SEQUENCE_NODE: {
					if(std::strcmp(reinterpret_cast<const char *>(value_node->tag), YAML_SEQ_TAG) != 0) {
						build_err_str("is not a array"sv);
						return false;
					}

					if(!read_array_node(doc, value_node, keystr, base)) {
						return false;
					}
				} break;
				default: {
					build_err_str("is not a array or map"sv);
					return false;
				}
			}
		}

		return true;
	}

	bool symbol_cache::read_yamls(const std::filesystem::path &dir, unsigned char *base) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::error_code ec;
		for(const auto &file : std::filesystem::directory_iterator{dir, ec}) {
			if(!file.is_regular_file()) {
				continue;
			}

			std::filesystem::path path{file.path()};
			std::filesystem::path filename{path.filename()};

			if(filename.native()[0] == '.') {
				continue;
			}

			if(filename.extension() != ".yaml"sv) {
				continue;
			}

			if(!read_yaml(path, base)) {
				return false;
			}
		}

		return true;
	}
#endif

#ifndef GSDK_NO_SYMBOLS
	bool symbol_cache::handle_component([[maybe_unused]] std::string_view name_mangled, demangle_component *component, qualifications_t::iterator &qual_it, qualification_info::names_t::iterator &name_it, basic_sym_t &&sym, unsigned char *base, [[maybe_unused]] bool elf) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(!component) {
		#ifndef __VMOD_COMPILING_SYMBOL_TOOL
			if(elf) {
				std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
				info->set_offset(sym.off);
				info->size_ = sym.size;
				info->resolve_from_base(base);
				name_it = global_qual.names.emplace(name_mangled, std::move(info)).first;
				qual_it = qualifications.end();
				return true;
			} else {
		#endif
				name_it = global_qual.names.end();
				qual_it = qualifications.end();
				return false;
		#ifndef __VMOD_COMPILING_SYMBOL_TOOL
			}
		#endif
		}

		switch(component->type) {
			case DEMANGLE_COMPONENT_VTABLE:
			case DEMANGLE_COMPONENT_QUAL_NAME:
			case DEMANGLE_COMPONENT_TYPED_NAME:
			break;
		#ifndef __VMOD_COMPILING_SYMBOL_TOOL
			case DEMANGLE_COMPONENT_LOCAL_NAME:
			case DEMANGLE_COMPONENT_NAME: {
				if(elf) {
					break;
				} else {
					[[fallthrough]];
				}
			}
		#endif
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
					name.starts_with("CMsg"sv) ||
					name.starts_with("CCallback"sv) ||
					name.starts_with("CMemberFunctor"sv) ||
					name.starts_with("CNonMemberScriptBinding"sv) ||
					name.starts_with("CSharedVarBase"sv) ||
					name.starts_with("CSharedUtlVectorBase"sv) ||
					name.starts_with("CFunctor"sv) ||
					name.starts_with("IGameStatTracker::CGameStatList"sv) ||
					name.starts_with("CDataManager"sv) ||
					name.starts_with("std::"sv) ||
					name.starts_with("CFmtStr"sv) ||
					name.starts_with("CDefOps"sv) ||
					name.starts_with("CManagedDataCacheClient"sv) ||
					name.starts_with("CCallResult"sv) ||
					name.starts_with("CUtl"sv)) {
					return true;
				}
				return false;
			}
		};

		switch(component->type) {
			case DEMANGLE_COMPONENT_VTABLE: {
				unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.left, guessed_name_length, &allocated);
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

				std::size_t vtable_size{0};
				if(sym.size > 0) {
					vtable_size = ((sym.size - (sizeof(__cxxabiv1::vtable_prefix) - sizeof(__cxxabiv1::vtable_prefix::origin))) / sizeof(generic_plain_mfp_t));
				}

				vtable_sizes.emplace(qual_name, vtable_size);

				auto this_qual_it{qualifications.find(qual_name)};
				if(this_qual_it == qualifications.end()) {
					this_qual_it = qualifications.emplace(std::move(qual_name), new class_info{}).first;
				}

				class_info *info{dynamic_cast<class_info *>(this_qual_it->second.get())};
				if(info) {
					info->vtable_.size_ = vtable_size;
					info->vtable_.offset = sym.off;
					info->vtable_.resolve_from_base(base);
				}

				name_it = global_qual.names.end();
				qual_it = this_qual_it;
				return true;
			}
		#ifndef __VMOD_COMPILING_SYMBOL_TOOL
			case DEMANGLE_COMPONENT_LOCAL_NAME: {
				if(elf) {
					switch(component->u.s_binary.left->type) {
						case DEMANGLE_COMPONENT_TYPED_NAME: {
							qualifications_t::iterator tmp_qual_it;
							qualification_info::names_t::iterator tmp_name_it;
							if(handle_component(name_mangled, component->u.s_binary.left, tmp_qual_it, tmp_name_it, basic_sym_t{}, base, elf) &&
								tmp_name_it != global_qual.names.end()) {
								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
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
								info->set_offset(sym.off);
								info->size_ = sym.size;
								info->resolve_from_base(base);

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
				} else {
					name_it = global_qual.names.end();
					qual_it = qualifications.end();
					return false;
				}
			}
			case DEMANGLE_COMPONENT_NAME: {
				if(elf) {
					unmangled_buffer = cplus_demangle_print(demangle_flags, component, guessed_name_length, &allocated);
					if(!unmangled_buffer) {
						name_it = global_qual.names.end();
						qual_it = qualifications.end();
						return false;
					}

					std::string name_unmangled{unmangled_buffer};
					std::free(unmangled_buffer);

					std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
					info->set_offset(sym.off);
					info->size_ = sym.size;
					info->resolve_from_base(base);

					name_it = global_qual.names.emplace(std::move(name_unmangled), std::move(info)).first;
					qual_it = qualifications.end();
					return true;
				} else {
					name_it = global_qual.names.end();
					qual_it = qualifications.end();
					return false;
				}
			}
		#endif
			case DEMANGLE_COMPONENT_QUAL_NAME: {
				switch(component->u.s_binary.right->type) {
					case DEMANGLE_COMPONENT_NAME: {
						switch(component->u.s_binary.left->type) {
							case DEMANGLE_COMPONENT_NAME: {
								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.left, guessed_name_length, &allocated);
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

								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
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

								std::uint64_t offset{sym.off};
							#ifndef __VMOD_COMPILING_SYMBOL_TOOL
								if(elf) {
									info->set_offset(offset);
									info->size_ = sym.size;
									info->resolve_from_base(base);
								}
							#endif

								name_it = this_qual_it->second->names.insert_or_assign(std::move(name), std::move(info)).first;
								qual_it = this_qual_it;

								offset_map.emplace(offset, pair_t{qual_it,name_it});

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
								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.left->u.s_binary.left->u.s_binary.left, guessed_name_length, &allocated);
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

								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.left->u.s_binary.left->u.s_binary.right, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string func_name{unmangled_buffer};
								std::free(unmangled_buffer);

								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
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

								std::uint64_t offset{sym.off};
							#ifndef __VMOD_COMPILING_SYMBOL_TOOL
								if(elf) {
									info->set_offset(offset);
									info->size_ = sym.size;
									info->resolve_from_base(base);
								}
							#endif

								name_it = this_qual_it->second->names.insert_or_assign(std::move(func_name), std::move(info)).first;
								qual_it = this_qual_it;

								offset_map.emplace(offset, pair_t{qual_it,name_it});

								return true;
							}
							case DEMANGLE_COMPONENT_QUAL_NAME: {
								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.left->u.s_binary.left, guessed_name_length, &allocated);
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

								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.left->u.s_binary.right, guessed_name_length, &allocated);
								if(!unmangled_buffer) {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}

								std::string func_name{unmangled_buffer};
								std::free(unmangled_buffer);

								unmangled_buffer = cplus_demangle_print(demangle_flags, component->u.s_binary.right, guessed_name_length, &allocated);
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
									#ifndef __VMOD_COMPILING_SYMBOL_TOOL
										if(elf) {
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
											break;
										} else {
									#endif
											name_it = global_qual.names.end();
											qual_it = this_qual_it;
											return false;
									#ifndef __VMOD_COMPILING_SYMBOL_TOOL
										}
									#endif
									}
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

								std::uint64_t offset{sym.off};
							#ifndef __VMOD_COMPILING_SYMBOL_TOOL
								if(elf) {
									info->set_offset(offset);
									info->size_ = sym.size;
									info->resolve_from_base(base);
								}
							#endif

								name_it = this_qual_it->second->names.insert_or_assign(std::move(func_name), std::move(info)).first;
								qual_it = this_qual_it;

							#ifndef __VMOD_COMPILING_SYMBOL_TOOL
								if(dynamic_cast<class_info::ctor_info *>(info.get()) == nullptr)
							#endif
								{
									offset_map.emplace(offset, pair_t{qual_it,name_it});
								}

								return true;
							}
						#ifndef __VMOD_COMPILING_SYMBOL_TOOL
							case DEMANGLE_COMPONENT_TEMPLATE: {
								if(elf) {
									switch(component->u.s_binary.left->u.s_binary.left->type) {
										case DEMANGLE_COMPONENT_NAME: break;
										default: {
											name_it = global_qual.names.end();
											qual_it = qualifications.end();
											return false;
										}
									}
									[[fallthrough]];
								} else {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}
							}
							case DEMANGLE_COMPONENT_NAME: {
								if(elf) {
									unmangled_buffer = cplus_demangle_print(demangle_flags, component, guessed_name_length, &allocated);
									if(!unmangled_buffer) {
										name_it = global_qual.names.end();
										qual_it = qualifications.end();
										return false;
									}

									std::string func_name{unmangled_buffer};
									std::free(unmangled_buffer);

									std::unique_ptr<qualification_info::name_info> info{new qualification_info::name_info};
									info->set_offset(sym.off);
									info->size_ = sym.size;
									info->resolve_from_base(base);

									name_it = global_qual.names.insert_or_assign(std::move(func_name), std::move(info)).first;
									qual_it = qualifications.end();
									return true;
								} else {
									name_it = global_qual.names.end();
									qual_it = qualifications.end();
									return false;
								}
							}
						#endif
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
#endif

	std::size_t symbol_cache::vtable_size(const std::string &name) const noexcept
	{
		auto it{vtable_sizes.find(name)};
		if(it == vtable_sizes.end()) {
			return static_cast<std::size_t>(-1);
		}

		return it->second;
	}

#if !defined __VMOD_COMPILING_SYMBOL_TOOL && !defined GSDK_NO_SYMBOLS
	std::uint64_t symbol_cache::uncached_find_mangled_func(const std::filesystem::path &path, std::string_view search) noexcept
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

		std::uint64_t offset{0};

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
					offset = static_cast<std::uint64_t>(sym.st_value);
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
#endif
}
