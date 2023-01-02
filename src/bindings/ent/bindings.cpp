#include "bindings.hpp"
#include "singleton.hpp"
#include "sendtable.hpp"
#include "datamap.hpp"
#include "factory.hpp"
#include "../docs.hpp"
#include "../../filesystem.hpp"

namespace vmod::bindings::ent
{
	bool bindings() noexcept
	{
		if(!singleton::instance().bindings()) {
			return false;
		}

		if(!sendprop::bindings()) {
			return false;
		}

		if(!sendtable::bindings()) {
			return false;
		}

		if(!dataprop::bindings()) {
			return false;
		}

		if(!datamap::bindings()) {
			return false;
		}

		if(!factory_ref::bindings()) {
			return false;
		}

		if(!factory_impl::bindings()) {
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
		factory_impl::unbindings();

		factory_ref::unbindings();

		sendprop::unbindings();
		sendtable::unbindings();

		dataprop::unbindings();
		datamap::unbindings();

		singleton::instance().unbindings();
	}

	void write_docs(const std::filesystem::path &dir) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		docs::gen_date(file);

		file += "namespace ent\n{\n"sv;

		docs::write(&factory_ref::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&factory_impl::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&sendtable::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&sendprop::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&datamap::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::write(&dataprop::desc, true, 1, file, false);
		file += "\n\n"sv;

		docs::ident(file, 1);
		file += "struct dataprop_description\n"sv;
		docs::ident(file, 1);
		file += "{\n"sv;
		docs::ident(file, 2);
		file += "char[] name;\n"sv;
		docs::ident(file, 2);
		file += "mem::types::type type;\n"sv;
		docs::ident(file, 2);
		file += "optional<char[]> external_name;\n"sv;
		docs::ident(file, 2);
		file += "optional<int> num;\n"sv;
		docs::ident(file, 1);
		file += "};\n\n"sv;

		docs::write(&singleton::desc, false, 1, file, false);

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "ent"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}
}
