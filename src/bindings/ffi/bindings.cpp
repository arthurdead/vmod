#include "bindings.hpp"
#include "singleton.hpp"
#include "cif.hpp"
#include "detour.hpp"
#include "../docs.hpp"
#include "../../filesystem.hpp"

namespace vmod::bindings::ffi
{
	bool bindings() noexcept
	{
		if(!singleton::instance().bindings()) {
			return false;
		}

		if(!caller::bindings()) {
			return false;
		}

		if(!detour::bindings()) {
			return false;
		}

		return true;
	}

	void unbindings() noexcept
	{
		detour::unbindings();

		caller::unbindings();

		singleton::instance().unbindings();
	}

	void write_docs(const std::filesystem::path &dir) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		docs::gen_date(file);

		file += "namespace ffi\n{\n"sv;

		docs::write(&detour::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&caller::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&singleton::desc, false, 1, file, false);

		file += '\n';

		docs::ident(file, 1);
		file += "enum class types\n"sv;
		docs::ident(file, 1);
		file += "{\n"sv;
		docs::write(file, 2, singleton::instance().types_table, docs::write_enum_how::name);
		docs::ident(file, 1);
		file += "};\n\n"sv;

		docs::ident(file, 1);
		file += "enum class abi\n"sv;
		docs::ident(file, 1);
		file += "{\n"sv;
		docs::write(file, 2, singleton::instance().abi_table, docs::write_enum_how::normal);
		docs::ident(file, 1);
		file += "};\n}"sv;

		std::filesystem::path doc_path{dir};
		doc_path /= "ffi"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}
}
