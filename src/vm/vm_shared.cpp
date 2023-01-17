#include "vm_shared.hpp"
#include "squirrel/vm.hpp"
#include "../filesystem.hpp"
#include "../gsdk.hpp"

namespace vmod
{
	template <typename ...Args>
	static inline auto CompileScript_strict(gsdk::IScriptVM *vm, Args &&...args) noexcept -> decltype(vm->CompileScript(std::forward<Args>(args)...))
	{
	#ifdef __VMOD_USING_CUSTOM_VM
		if(vm->GetLanguage() == gsdk::SL_SQUIRREL) {
			return static_cast<vm::squirrel *>(vm)->CompileScript_strict(std::forward<Args>(args)...);
		} else
	#endif
		{
			return vm->CompileScript(std::forward<Args>(args)...);
		}
	}

	bool compile_internal_script(gsdk::IScriptVM *vm, std::filesystem::path path, const unsigned char *data, gsdk::HSCRIPT &object, bool &from_file) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::filesystem::path filename{path.filename()};

		if(std::filesystem::exists(path)) {
			std::unique_ptr<unsigned char[]> script_data{read_file(path)};

			object = CompileScript_strict(vm, reinterpret_cast<const char *>(script_data.get()), path.c_str());
			if(!object || object == gsdk::INVALID_HSCRIPT) {
				if(data) {
					error("vmod: failed to compile '%s' from file '%s' re-trying with embedded data\n"sv, filename.c_str(), path.c_str());
					object = CompileScript_strict(vm, reinterpret_cast<const char *>(data), filename.c_str());
					if(!object || object == gsdk::INVALID_HSCRIPT) {
						error("vmod: failed to compile '%s' from embedded data\n"sv, data);
						return false;
					}
				} else {
					error("vmod: failed to compile '%s' from file '%s'\n"sv, filename.c_str(), path.c_str());
					return false;
				}
			} else {
				from_file = true;
			}
		} else {
			if(data) {
				warning("vmod: missing '%s' file trying embedded data\n"sv, filename.c_str());
				object = CompileScript_strict(vm, reinterpret_cast<const char *>(data), filename.c_str());
				if(!object || object == gsdk::INVALID_HSCRIPT) {
					error("vmod: failed to compile '%s' from embedded data\n"sv, filename.c_str());
					return false;
				}
			} else {
				error("vmod: missing '%s' file '%s'\n"sv, filename.c_str(), path.c_str());
				return false;
			}
		}

		return true;
	}
}
