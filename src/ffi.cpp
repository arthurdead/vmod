#include "ffi.hpp"
#include "gsdk/mathlib/vector.hpp"
#include "gsdk/string_t.hpp"
#include "gsdk/tier0/dbg.hpp"
#include "hacking.hpp"
#include "gsdk/server/datamap.hpp"

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
static ffi_type *ffi_type_tstr_object_elements[1]{
	&ffi_type_cstr
};

#define __VMOD_IMPLEMENT_FFI_TYPE_WITH_PTR(name, type) \
	ffi_type ffi_type_##name{ \
		sizeof(type), \
		alignof(type), \
		FFI_TYPE_STRUCT, \
		ffi_type_##name##_elements \
	}; \
	ffi_type ffi_type_##name##_ptr{ \
		sizeof(type *), \
		alignof(type *), \
		FFI_TYPE_POINTER, \
		nullptr \
	};

__VMOD_IMPLEMENT_FFI_TYPE_WITH_PTR(vector, gsdk::Vector)
__VMOD_IMPLEMENT_FFI_TYPE_WITH_PTR(qangle, gsdk::QAngle)
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
ffi_type ffi_type_object_tstr{
	sizeof(gsdk::detail::string_t::object::string_t),
	alignof(gsdk::detail::string_t::object::string_t),
	FFI_TYPE_STRUCT,
	ffi_type_tstr_object_elements
};
ffi_type ffi_type_ent_ptr{
	sizeof(gsdk::CBaseEntity *),
	alignof(gsdk::CBaseEntity *),
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
			case FFI_TYPE_POINTER: {
				if(type == &ffi_type_ent_ptr) {
					*static_cast<gsdk::CBaseEntity **>(ptr) = var.get<gsdk::CBaseEntity *>();
				} else if(type == &ffi_type_vector_ptr) {
					**static_cast<gsdk::Vector **>(ptr) = var.get<gsdk::Vector>();
				} else if(type == &ffi_type_qangle_ptr) {
					**static_cast<gsdk::QAngle **>(ptr) = var.get<gsdk::QAngle>();
				} else if(type == &ffi_type_cstr) {
					*static_cast<const char **>(ptr) = var.get<const char *>();
				} else {
					*static_cast<void **>(ptr) = var.get<void *>();
				}
			} break;
			case FFI_TYPE_STRUCT: {
				if(type == &ffi_type_vector) {
					*static_cast<gsdk::Vector *>(ptr) = var.get<gsdk::Vector>();
				} else if(type == &ffi_type_qangle) {
					*static_cast<gsdk::QAngle *>(ptr) = var.get<gsdk::QAngle>();
				} else {
					debugtrap();
				}
			} break;
			default: {
				debugtrap();
			} break;
		}
	}

	void ptr_to_script_var(void *ptr, ffi_type *type, gsdk::ScriptVariant_t &var) noexcept
	{
		std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		if(type == &ffi_type_ent_ptr) {
			var.m_object = (*static_cast<gsdk::CBaseEntity **>(ptr))->GetScriptInstance();
			var.m_type = gsdk::FIELD_HSCRIPT;
		} else if(type == &ffi_type_vector_ptr) {
			var.m_vector = new gsdk::Vector{**static_cast<gsdk::Vector **>(ptr)};
			var.m_type = gsdk::FIELD_VECTOR;
			var.m_flags |= gsdk::SV_FREE;
		} else if(type == &ffi_type_qangle_ptr) {
			var.m_qangle = new gsdk::QAngle{**static_cast<gsdk::QAngle **>(ptr)};
			var.m_type = gsdk::FIELD_QANGLE;
			var.m_flags |= gsdk::SV_FREE;
		} else if(type == &ffi_type_vector) {
			var.m_vector = new gsdk::Vector{*static_cast<gsdk::Vector *>(ptr)};
			var.m_type = gsdk::FIELD_VECTOR;
			var.m_flags |= gsdk::SV_FREE;
		} else if(type == &ffi_type_qangle) {
			var.m_qangle = new gsdk::QAngle{*static_cast<gsdk::QAngle *>(ptr)};
			var.m_type = gsdk::FIELD_QANGLE;
			var.m_flags |= gsdk::SV_FREE;
		} else if(type == &ffi_type_cstr) {
			var.m_ccstr = *static_cast<const char **>(ptr);
			var.m_type = gsdk::FIELD_CSTRING;
		} else {
			std::memcpy(var.m_data, ptr, type->size);
			var.m_type = static_cast<short>(to_field_type(type));
		}
	}

	void init_ptr(void *ptr, ffi_type *type) noexcept
	{
		std::memset(ptr, 0, type->size);
	}

	int to_field_type(ffi_type *type)
	{
		switch(type->type) {
			case FFI_TYPE_INT:
			return gsdk::FIELD_INTEGER;
			case FFI_TYPE_FLOAT:
			return gsdk::FIELD_FLOAT;
			case FFI_TYPE_DOUBLE:
			return gsdk::FIELD_FLOAT64;
			case FFI_TYPE_LONGDOUBLE:
			return gsdk::FIELD_FLOAT64;
			case FFI_TYPE_UINT8:
			return gsdk::FIELD_CHARACTER;
			case FFI_TYPE_SINT8:
			return gsdk::FIELD_CHARACTER;
			case FFI_TYPE_UINT16:
			return gsdk::FIELD_SHORT;
			case FFI_TYPE_SINT16:
			return gsdk::FIELD_SHORT;
			case FFI_TYPE_UINT32:
			return gsdk::FIELD_UINT32;
			case FFI_TYPE_SINT32:
			return gsdk::FIELD_INTEGER;
			case FFI_TYPE_UINT64:
			return gsdk::FIELD_UINT64;
			case FFI_TYPE_SINT64:
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			return gsdk::FIELD_INTEGER64;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
			return gsdk::FIELD_INTEGER;
		#else
			#error
		#endif
			case FFI_TYPE_POINTER: {
				if(type == &ffi_type_ent_ptr) {
					return gsdk::FIELD_HSCRIPT;
				} else if(type == &ffi_type_vector_ptr) {
					return gsdk::FIELD_VECTOR;
				} else if(type == &ffi_type_qangle_ptr) {
					return gsdk::FIELD_QANGLE;
				} else if(type == &ffi_type_cstr) {
					return gsdk::FIELD_CSTRING;
				}

				return gsdk::FIELD_CLASSPTR;
			}
			case FFI_TYPE_STRUCT: {
				if(type == &ffi_type_vector) {
					return gsdk::FIELD_VECTOR;
				} else if(type == &ffi_type_qangle) {
					return gsdk::FIELD_QANGLE;
				} else {
					debugtrap();
					return gsdk::FIELD_TYPEUNKNOWN;
				}
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

	cif::~cif() noexcept {}

	bool cif::initialize(ffi_abi abi) noexcept
	{
		if(ffi_prep_cif(&cif_impl, abi, args_types.size(), ret_type, args_types.data()) != FFI_OK) {
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

	void cif::call(void(*func)(), void *ret, void **args) noexcept
	{
		ffi_call(&cif_impl, func, ret, args);
	}

	void cif::call(void(*func)(), void *ret) noexcept
	{
		ffi_call(&cif_impl, func, ret, args_ptrs.data());
	}

	void cif::call(void(*func)(), void **args) noexcept
	{
		ffi_call(&cif_impl, func, static_cast<void *>(ret_storage.get()), args);
	}

	bool closure::initialize_impl(ffi_abi abi, void **func, binding_func binding, void *userptr) noexcept
	{
		if(!cif::initialize(abi)) {
			return false;
		}

		closure_impl = static_cast<ffi_closure *>(ffi_closure_alloc(sizeof(ffi_closure), func));
		if(!closure_impl) {
			return false;
		}

		if(ffi_prep_closure_loc(closure_impl, &cif_impl, binding, userptr, *func) != FFI_OK) {
			return false;
		}

		return true;
	}

	closure::~closure() noexcept
	{
		if(closure_impl) {
			ffi_closure_free(closure_impl);
		}
	}
}
