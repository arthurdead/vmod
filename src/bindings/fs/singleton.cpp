#include "singleton.hpp"
#include "../../main.hpp"
#include <glob.h>

namespace vmod::bindings::fs
{
	vscript::singleton_class_desc<singleton> singleton::desc{"fs"};

	static singleton fs_;

	singleton &singleton::instance() noexcept
	{ return fs_; }

	singleton::~singleton() noexcept {}

	bool singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		desc.func(&singleton::script_join_paths, "script_join_paths"sv, "join_paths"sv);

		desc.func(&singleton::script_glob, "script_glob"sv, "glob"sv)
		.desc("[array<path>](pattern)"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		if(!vm->SetValue(scope, "game_dir", vscript::variant{main::instance().game_dir()})) {
			error("vmod: failed to set game dir value\n"sv);
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(vm->ValueExists(scope, "game_dir")) {
			vm->ClearValue(scope, "game_dir");
		}

		singleton_base::unbindings();
	}

	int singleton::script_globerr([[maybe_unused]] const char *, [[maybe_unused]] int) noexcept
	{
		//TODO!!!
		//vmod.vm()->RaiseException("vmod: glob error %i on %s:", eerrno, epath);
		return 0;
	}

	gsdk::HSCRIPT singleton::script_glob(const std::filesystem::path &pattern) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(pattern.empty()) {
			vm->RaiseException("vmod: invalid pattern: '%s'", pattern.c_str());
			return nullptr;
		}

		glob_t glob;
		if(::glob(pattern.c_str(), GLOB_ERR|GLOB_NOSORT, script_globerr, &glob) != 0) {
			globfree(&glob);
			return nullptr;
		}

		gsdk::HSCRIPT arr{vm->CreateArray()};
		if(!arr || arr == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: failed to create array");
			return nullptr;
		}

		for(std::size_t i{0}; i < glob.gl_pathc; ++i) {
			std::string temp{glob.gl_pathv[i]};

			vscript::variant var;
			var.assign<std::string>(std::move(temp));
			vm->ArrayAddToTail(arr, std::move(var));
		}

		globfree(&glob);

		return arr;
	}

	std::filesystem::path singleton::script_join_paths(const vscript::variant *args, std::size_t num_args, ...) noexcept
	{
		std::filesystem::path final_path;

		for(std::size_t i{0}; i < num_args; ++i) {
			final_path /= args[i].get<std::filesystem::path>();
		}

		return final_path;
	}
}
