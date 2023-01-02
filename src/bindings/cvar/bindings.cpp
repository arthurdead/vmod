#include "bindings.hpp"
#include "singleton.hpp"
#include "convar.hpp"
#include "../docs.hpp"
#include "../../filesystem.hpp"

namespace vmod::bindings::cvar
{
	bool bindings() noexcept
	{
		if(!convar::bindings()) {
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
		convar::unbindings();

		singleton::instance().unbindings();
	}

	void write_docs(const std::filesystem::path &dir) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		docs::gen_date(file);

		file += "namespace cvar\n{\n"sv;

		docs::ident(file, 1);
		file += "enum class flags\n"sv;
		docs::ident(file, 1);
		file += "{\n"sv;
		docs::write(file, 2, singleton::instance().flags_table, docs::write_enum_how::flags);
		docs::ident(file, 1);
		file += "};\n\n"sv;

		docs::write(&convar::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&singleton::desc, false, 1, file, false);

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "cvar"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}
}
