#include "bindings.hpp"
#include "string_table.hpp"
#include "../../main.hpp"
#include "../docs.hpp"
#include "../../filesystem.hpp"

namespace vmod::bindings::strtables
{
	bool bindings() noexcept
	{
		if(!string_table::bindings()) {
			return false;
		}

		return true;
	}

	void unbindings() noexcept
	{
		string_table::unbindings();
	}

	void write_docs(const std::filesystem::path &dir) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		docs::gen_date(file);

		file += "namespace strtables\n{\n"sv;

		docs::write(&string_table::desc, true, 1, file, false);
		file += "\n\n"sv;

		for(const auto &it : main::instance().script_stringtables) {
			docs::ident(file, 1);
			file += docs::get_class_desc_name(&string_table::desc);
			file += ' ';
			file += it.first;
			file += ";\n"sv;
		}

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "strtables"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}
}
