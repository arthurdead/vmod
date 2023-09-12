#include "instance.hpp"
#include "../vscript/class_desc.hpp"
#include "docs.hpp"

namespace vmod::bindings
{
	bool instance_base::register_instance(gsdk::ScriptClassDesc_t *target_desc, void *pthis) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		const char *desc_name{bindings::docs::get_class_desc_name(target_desc).data()};

		instance_ = vm->RegisterInstance(target_desc, pthis);
		if(!instance_) {
			vm->RaiseException("vmod: failed to register '%s' instance", desc_name);
			return false;
		}

		std::string_view obfuscated_name;

		if(target_desc->m_pNextDesc == reinterpret_cast<const gsdk::ScriptClassDesc_t *>(uninitialized_memory)) {
			const vscript::extra_class_desc &extra{static_cast<const vscript::detail::base_class_desc<generic_class> *>(target_desc)->extra()};
			obfuscated_name = extra.obfuscated_name();
		} else {
			vm->RaiseException("vmod: class desc '%s' was not created by vmod", desc_name);
			return false;
		}

		{
			std::string id_root{obfuscated_name};
			id_root += "_instance"sv;

			if(!vm->SetInstanceUniqeId2(*instance_, id_root.data())) {
				vm->RaiseException("vmod: failed to generate unique id for '%s'", desc_name);
				return false;
			}
		}

		return true;
	}
}
