#include "gsdk_library.hpp"
#include "symbol_cache.hpp"
#include "filesystem.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <charconv>

int main(int argc, char *argv[], [[maybe_unused]] char *[])
{
	using namespace std::literals::string_view_literals;

	if(argc != 3) {
		std::cout << "vmod: usage: <file> <dir>\n"sv;
		return EXIT_FAILURE;
	}

	std::filesystem::path binary{argv[1]};
	std::filesystem::path dir{argv[2]};

	if(!vmod::symbol_cache::initialize()) {
		return EXIT_FAILURE;
	}

	std::filesystem::path ext{binary.extension()};

	void *base{nullptr};

	vmod::gsdk_library lib;
	if(ext == ".so"sv) {
		if(!lib.load(binary)) {
			std::cout << "vmod: failed to load '"sv << binary << "': '"sv << lib.error_string() << "'\n"sv;
			return EXIT_FAILURE;
		}

		base = lib.base();
	}

	vmod::symbol_cache syms;
	if(!syms.load(binary, base)) {
		std::cout << "vmod: failed to read '"sv << binary << "': '"sv << syms.error_string() << "'\n"sv;
		return EXIT_FAILURE;
	}

	for(const auto &it : syms) {
		auto class_info{dynamic_cast<vmod::symbol_cache::class_info *>(it.second.get())};
		if(!class_info) {
			continue;
		}

		const auto &vtable{class_info->vtable()};

		std::size_t vtable_size{vtable.size()};
		if(vtable_size == 0) {
			continue;
		}

		std::string file;

		file += "class "sv;
		file += it.first;

		file += "\n{\n"sv;

		for(std::size_t i{0}; i < vtable_size; ++i) {
			const auto &func_it{vtable[i]};

			file += "\t//"sv;

			std::size_t len{6 + 6};

			std::string idx_buffer;
			idx_buffer.resize(len);

			char *begin{idx_buffer.data()};
			char *end{begin + len};

			std::to_chars_result tc_res{std::to_chars(begin, end, i)};
			tc_res.ptr[0] = '\0';

			file += begin;

			file += '\n';

			file += '\t';

			if(func_it.qual != syms.end()) {
				if(func_it.qual->first != it.first) {
					file += func_it.qual->first;
					file += "::"sv;
				}
			} else {
				file += "<<unknown>>::"sv;
			}

			if(func_it.func != class_info->end()) {
				file += func_it.func->first;
				file += ";\n"sv;
			} else {
				file += "<<unknown>>\n"sv;
			}

			file += '\n';
		}
		file.pop_back();

		file += "};"sv;

		std::filesystem::path path{dir};
		path /= it.first;
		path.replace_extension(".txt"sv);

		vmod::write_file(path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	return EXIT_SUCCESS;
}
