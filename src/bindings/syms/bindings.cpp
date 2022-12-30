#include "bindings.hpp"
#include "singleton.hpp"
#include "../docs.hpp"
#include "../../filesystem.hpp"

namespace vmod::bindings::syms
{
	bool bindings() noexcept
	{
		if(!singleton::bindings()) {
			return false;
		}

		if(!singleton::qualification_it::bindings()) {
			return false;
		}

		if(!singleton::name_it::bindings()) {
			return false;
		}

		return true;
	}

	void unbindings() noexcept
	{
		singleton::qualification_it::unbindings();

		singleton::name_it::unbindings();

		singleton::unbindings();
	}

	void write_docs(const std::filesystem::path &dir) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		docs::gen_date(file);

		file += "namespace syms\n{\n"sv;

		docs::write(&singleton::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&singleton::qualification_it::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&singleton::name_it::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::ident(file, 1);
		file += docs::get_class_desc_name(&singleton::desc);
		file += " sv;"sv;

		file += "\n}"sv;

		std::filesystem::path doc_path{dir};
		doc_path /= "syms"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}
}
