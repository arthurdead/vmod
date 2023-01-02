#include "datamap.hpp"
#include "../../main.hpp"
#include "singleton.hpp"
#include <cstddef>
#include <cstdint>

namespace vmod::bindings::ent
{
	vscript::class_desc<detail::dataprop_base> detail::dataprop_base::desc{"ent::dataprop"};
	vscript::class_desc<detail::datamap_base> detail::datamap_base::desc{"ent::datamap"};

	vscript::class_desc<allocated_datamap> allocated_datamap::desc{"ent::allocated_datamap"};

	bool dataprop::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		desc.func(&dataprop::script_type, "script_type"sv, "type"sv)
		.desc("[mem::types::type]"sv);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register dataprop script class\n"sv);
			return false;
		}

		return true;
	}

	void dataprop::unbindings() noexcept
	{
		
	}

	detail::dataprop_base::dataprop_base(gsdk::typedescription_t *prop_) noexcept
		: prop{prop_}, type_ptr{guess_type(prop_, nullptr)}
	{
		type = mem::singleton::instance().find_type(type_ptr);
	}

	bool dataprop::initialize() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		instance = vm->RegisterInstance(&desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			return false;
		}

		//TODO!! SetInstanceUniqueID

		return true;
	}

	dataprop::~dataprop() noexcept
	{
		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			main::instance().vm()->RemoveInstance(instance);
		}
	}

	ffi_type *detail::dataprop_base::guess_type(const gsdk::typedescription_t *prop, [[maybe_unused]] const gsdk::datamap_t *) noexcept
	{
		switch(prop->fieldType) {
			case gsdk::FIELD_INTERVAL:
			case gsdk::FIELD_MODELINDEX:
			case gsdk::FIELD_MATERIALINDEX:
			case gsdk::FIELD_TICK:
			case gsdk::FIELD_INTEGER: {
				return &ffi_type_sint;
			}
		#if GSDK_ENGINE == GSDK_ENGINE_L4D2
			case gsdk::FIELD_INTEGER64: {
				return &ffi_type_sint64;
			}
		#endif
			case gsdk::FIELD_FLOAT: {
				return &ffi_type_float;
			}
			case gsdk::FIELD_SHORT: {
				return &ffi_type_sshort;
			}
			case gsdk::FIELD_BOOLEAN: {
				return &ffi_type_bool;
			}
			case gsdk::FIELD_CHARACTER: {
				return &ffi_type_schar;
			}
			case gsdk::FIELD_COLOR32: {
				return &ffi_type_color32;
			}
			case gsdk::FIELD_VECTOR:
			case gsdk::FIELD_POSITION_VECTOR: {
				return &ffi_type_vector;
			}
			case gsdk::FIELD_EHANDLE: {
				return &ffi_type_ehandle;
			}
			case gsdk::FIELD_TIME: {
				return &ffi_type_float;
			}
			case gsdk::FIELD_STRING: {
				return &ffi_type_object_tstr;
			}
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME: {
				return &ffi_type_cstr;
			}
			default:
			return nullptr;
		}
	}

	bool datamap::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register datatable script class\n"sv);
			return false;
		}

		return true;
	}

	void datamap::unbindings() noexcept
	{
		
	}

	detail::datamap_base::datamap_base(gsdk::datamap_t *map_) noexcept
		: map{map_}
	{
	}

	bool datamap::initialize() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		instance = vm->RegisterInstance(&desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			return false;
		}

		//TODO!! SetInstanceUniqueID

		return true;
	}

	datamap::~datamap() noexcept
	{
		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			main::instance().vm()->RemoveInstance(instance);
		}
	}

	allocated_datamap::~allocated_datamap() noexcept
	{
		singleton &ent{singleton::instance()};
		ent.erase(map);
	}
}
