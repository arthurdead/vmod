#include "ffi.hpp"
#include "gsdk/mathlib/vector.hpp"
#include "gsdk/tier0/dbg.hpp"
#include "hacking.hpp"

static ffi_type *ffi_type_vector_elements[3]{
	&ffi_type_float, &ffi_type_float, &ffi_type_float
};
static ffi_type *ffi_type_qangle_elements[3]{
	&ffi_type_float, &ffi_type_float, &ffi_type_float
};
static ffi_type *ffi_type_color32_elements[4]{
	&ffi_type_uchar, &ffi_type_uchar, &ffi_type_uchar, &ffi_type_uchar
};
static ffi_type *ffi_type_ehandle_elements[1]{
	&ffi_type_ulong
};

ffi_type ffi_type_vector{
	sizeof(gsdk::Vector),
	alignof(gsdk::Vector),
	FFI_TYPE_STRUCT,
	ffi_type_vector_elements
};
ffi_type ffi_type_qangle{
	sizeof(gsdk::QAngle),
	alignof(gsdk::QAngle),
	FFI_TYPE_STRUCT,
	ffi_type_qangle_elements
};
ffi_type ffi_type_color32{
	sizeof(Color),
	alignof(Color),
	FFI_TYPE_STRUCT,
	ffi_type_color32_elements
};
ffi_type ffi_type_ehandle{
	sizeof(gsdk::EHANDLE),
	alignof(gsdk::EHANDLE),
	FFI_TYPE_STRUCT,
	ffi_type_ehandle_elements
};
ffi_type ffi_type_bool{
	sizeof(bool),
	alignof(bool),
	FFI_TYPE_UINT8,
	nullptr
};
ffi_type ffi_type_cstr{
	sizeof(const char *),
	alignof(const char *),
	FFI_TYPE_POINTER,
	nullptr
};

namespace vmod::ffi
{
	void script_var_to_ptr(const vscript::variant &var, void *ptr, ffi_type *type) noexcept
	{
		switch(type->type) {
			case FFI_TYPE_INT:
			*static_cast<int *>(ptr) = var.get<int>();
			break;
			case FFI_TYPE_FLOAT:
			*static_cast<float *>(ptr) = var.get<float>();
			break;
			case FFI_TYPE_DOUBLE:
			*static_cast<double *>(ptr) = var.get<double>();
			break;
			case FFI_TYPE_LONGDOUBLE:
			*static_cast<long double *>(ptr) = var.get<long double>();
			break;
			case FFI_TYPE_UINT8:
			*static_cast<unsigned char *>(ptr) = var.get<unsigned char>();
			break;
			case FFI_TYPE_SINT8:
			*static_cast<signed char *>(ptr) = var.get<signed char>();
			break;
			case FFI_TYPE_UINT16:
			*static_cast<unsigned short *>(ptr) = var.get<unsigned short>();
			break;
			case FFI_TYPE_SINT16:
			*static_cast<short *>(ptr) = var.get<short>();
			break;
			case FFI_TYPE_UINT32:
			*static_cast<unsigned int *>(ptr) = var.get<unsigned int>();
			break;
			case FFI_TYPE_SINT32:
			*static_cast<int *>(ptr) = var.get<int>();
			break;
			case FFI_TYPE_UINT64:
			*static_cast<unsigned long long *>(ptr) = var.get<unsigned long long>();
			break;
			case FFI_TYPE_SINT64:
			*static_cast<long long *>(ptr) = var.get<long long>();
			break;
			case FFI_TYPE_POINTER:
			*static_cast<void **>(ptr) = var.get<void *>();
			break;
			case FFI_TYPE_STRUCT: {
				debugtrap();
			} break;
			default: {
				debugtrap();
			} break;
		}
	}

	void ptr_to_script_var(void *ptr, ffi_type *type, gsdk::ScriptVariant_t &var) noexcept
	{
		std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));
		std::memcpy(var.m_data, ptr, type->size);
		var.m_type = to_script_type(type);
	}

	short to_script_type(ffi_type *type)
	{
		switch(type->type) {
			case FFI_TYPE_INT:
			return gsdk::FIELD_INTEGER;
			case FFI_TYPE_FLOAT:
			return gsdk::FIELD_FLOAT;
			case FFI_TYPE_DOUBLE:
			return gsdk::FIELD_DOUBLE;
			case FFI_TYPE_LONGDOUBLE:
			return gsdk::FIELD_DOUBLE;
			case FFI_TYPE_UINT8:
			return gsdk::FIELD_SHORT;
			case FFI_TYPE_SINT8:
			return gsdk::FIELD_SHORT;
			case FFI_TYPE_UINT16:
			return gsdk::FIELD_SHORT;
			case FFI_TYPE_SINT16:
			return gsdk::FIELD_SHORT;
			case FFI_TYPE_UINT32:
			return gsdk::FIELD_UINT;
			case FFI_TYPE_SINT32:
			return gsdk::FIELD_INTEGER;
			case FFI_TYPE_UINT64:
			return gsdk::FIELD_UINT64;
			case FFI_TYPE_SINT64:
			return gsdk::FIELD_INTEGER64;
			case FFI_TYPE_POINTER:
		#if __SIZEOF_POINTER__ == __SIZEOF_INT__
			return gsdk::FIELD_UINT;
		#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
			return gsdk::FIELD_UINT64;
		#else
			#error
		#endif
			case FFI_TYPE_STRUCT: {
				debugtrap();
				return gsdk::FIELD_TYPEUNKNOWN;
			}
			default: {
				debugtrap();
				return gsdk::FIELD_TYPEUNKNOWN;
			}
		}
	}

	ffi_type *type_id_to_ptr(int id) noexcept
	{
		switch(id) {
			case FFI_TYPE_VOID: return &ffi_type_void;
			case FFI_TYPE_INT: return &ffi_type_sint;
			case FFI_TYPE_FLOAT: return &ffi_type_float;
			case FFI_TYPE_DOUBLE: return &ffi_type_double;
			case FFI_TYPE_LONGDOUBLE: return &ffi_type_longdouble;
			case FFI_TYPE_UINT8: return &ffi_type_uint8;
			case FFI_TYPE_SINT8: return &ffi_type_sint8;
			case FFI_TYPE_UINT16: return &ffi_type_uint16;
			case FFI_TYPE_SINT16: return &ffi_type_sint16;
			case FFI_TYPE_UINT32: return &ffi_type_uint32;
			case FFI_TYPE_SINT32: return &ffi_type_sint32;
			case FFI_TYPE_UINT64: return &ffi_type_uint64;
			case FFI_TYPE_SINT64: return &ffi_type_sint64;
			case FFI_TYPE_POINTER: return &ffi_type_pointer;
			case FFI_TYPE_STRUCT: {
				debugtrap();
				return nullptr;
			}
			default: {
				debugtrap();
				return nullptr;
			}
		}
	}

	bool cif::initialize(ffi_abi abi) noexcept
	{
		if(ffi_prep_cif(&impl, abi, args_types.size(), ret_type, args_types.data()) != FFI_OK) {
			return false;
		}

		for(ffi_type *type : args_types) {
			auto &arg_ptr{args_storage.emplace_back()};
			arg_ptr.reset(static_cast<unsigned char *>(std::aligned_alloc(type->alignment, type->size)));
		}

		for(auto &ptr : args_storage) {
			args_ptrs.emplace_back(ptr.get());
		}

		if(ret_type != &ffi_type_void) {
			ret_storage.reset(static_cast<unsigned char *>(std::aligned_alloc(ret_type->alignment, ret_type->size)));
		}

		return true;
	}

	void cif::call(void(*func)()) noexcept
	{
		ffi_call(&impl, func, static_cast<void *>(ret_storage.get()), args_ptrs.data());
	}
}
