#include "bindings.hpp"
#include "singleton.hpp"
#include "../docs.hpp"
#include "../../filesystem.hpp"

namespace vmod::bindings::fs
{
	bool bindings() noexcept
	{
		if(!singleton::instance().bindings()) {
			return false;
		}

		return true;
	}

	void unbindings() noexcept
	{
		singleton::instance().unbindings();
	}

	void write_docs(const std::filesystem::path &dir) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		docs::gen_date(file);

		file += "namespace fs\n{\n"sv;

		docs::write(&singleton::desc, false, 1, file, false);
		file += '\n';

		docs::ident(file, 1);
		file += "string game_dir;\n"sv;

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "fs"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}
}
