#include "bindings.hpp"
#include "singleton.hpp"
#include "container.hpp"
#include "../docs.hpp"
#include "../../filesystem.hpp"

namespace vmod::bindings::mem
{
	bool bindings() noexcept
	{
		if(!container::bindings()) {
			return false;
		}

		if(!singleton::instance().bindings()) {
			return false;
		}

		return true;
	}

	bool create_get() noexcept
	{
		if(!singleton::instance().create_get()) {
			return false;
		}

		return true;
	}

	void unbindings() noexcept
	{
		container::unbindings();

		singleton::instance().unbindings();
	}

	void write_docs(const std::filesystem::path &dir) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		docs::gen_date(file);

		file += "namespace mem\n{\n"sv;

		docs::ident(file, 1);
		file += "namespace types\n"sv;
		docs::ident(file, 1);
		file += "{\n"sv;
		docs::ident(file, 2);
		file += "struct type\n"sv;
		docs::ident(file, 2);
		file += "{\n"sv;
		docs::ident(file, 3);
		file += "int size;\n"sv;
		docs::ident(file, 3);
		file += "int alignment;\n"sv;
		docs::ident(file, 2);
		file += "};\n\n"sv;
		for(const auto &it : singleton::instance().types) {
			docs::ident(file, 2);
			file += "type "sv;
			file += it->name();
			file += ";\n"sv;
		}
		docs::ident(file, 1);
		file += "}\n\n"sv;

		docs::ident(file, 1);
		file += "using free_callback = void(container mem);\n\n"sv;

		docs::write(&container::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&singleton::desc, false, 1, file, false);

		file += "}\n"sv;

		std::filesystem::path doc_path{dir};
		doc_path /= "mem"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}
}
