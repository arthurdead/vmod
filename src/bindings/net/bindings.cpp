#include "bindings.hpp"
#include "singleton.hpp"
#include "../docs.hpp"
#include "../../filesystem.hpp"

namespace vmod::bindings::net
{
	bool bindings() noexcept
	{
		if(!mysql::bindings()) {
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
		mysql::unbindings();

		singleton::instance().unbindings();
	}

	void write_docs(const std::filesystem::path &dir) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		docs::gen_date(file);

		file += "namespace net\n{\n"sv;

		docs::write(&mysql::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&singleton::desc, false, 1, file, false);
		file += '\n';

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "net"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}
}
