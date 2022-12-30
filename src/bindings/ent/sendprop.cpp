#include "sendprop.hpp"
#include "../../main.hpp"

namespace vmod::bindings::ent
{
	ffi::cif sendprop::proxy_cif{&ffi_type_void, {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_sint, &ffi_type_sint}};

	vscript::class_desc<sendprop> sendprop::desc{"ent::sendprop"};

	bool sendprop::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!proxy_cif.initialize(FFI_SYSV)) {
			error("vmod: failed to initialize send proxy cif\n"sv);
			return false;
		}

		desc.func(&sendprop::script_hook_proxy, "script_hook_proxy"sv, "hook_proxy"sv);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register sendprop script class\n"sv);
			return false;
		}

		return true;
	}

	void sendprop::unbindings() noexcept
	{
		
	}

	bool sendprop::initialize() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		instance = vm->RegisterInstance(&desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			return false;
		}

		//TODO!! SetInstanceUniqueID

		return true;
	}

	sendprop::~sendprop() noexcept
	{
		if(closure) {
			ffi_closure_free(closure);
		}

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			main::instance().vm()->RemoveInstance(instance);
		}
	}

	bool sendprop::initialize_closure() noexcept
	{
		if(closure) {
			return true;
		}

		closure = static_cast<ffi_closure *>(ffi_closure_alloc(sizeof(ffi_closure), reinterpret_cast<void **>(&prop->m_ProxyFn)));
		if(!closure) {
			return false;
		}

		if(ffi_prep_closure_loc(closure, &proxy_cif, closure_binding, this, reinterpret_cast<void *>(prop->m_ProxyFn)) != FFI_OK) {
			return false;
		}

		return true;
	}

	void sendprop::closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept
	{
		sendprop *prop{static_cast<sendprop *>(userptr)};

		
	}

	gsdk::HSCRIPT sendprop::script_hook_proxy(gsdk::HSCRIPT callback, bool post, bool per_client) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!callback || callback == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid function");
			return nullptr;
		}

		vm->RaiseException("vmod: not implemented yet");
		return nullptr;

		if(!initialize_closure()) {
			vm->RaiseException("vmod: failed to initialize closure");
			return nullptr;
		}

		return nullptr;
	}

	ffi_type *sendprop::guess_type(const gsdk::SendProp *prop, gsdk::SendVarProxyFn proxy, const gsdk::SendTable *table) noexcept
	{
		switch(prop->m_Type) {
			case gsdk::DPT_Int: {
				if(prop->m_Flags & gsdk::SPROP_UNSIGNED) {
					if(proxy == std_proxies->m_UInt8ToInt32) {
						if(prop->m_nBits == 1) {
							return &ffi_type_bool;
						}

						return &ffi_type_uchar;
					} else if(proxy == std_proxies->m_UInt16ToInt32) {
						return &ffi_type_ushort;
					} else if(proxy == std_proxies->m_UInt32ToInt32) {
						if(table && std::strcmp(table->m_pNetTableName, "DT_BaseEntity") == 0 && std::strcmp(prop->m_pVarName, "m_clrRender") == 0) {
							return &ffi_type_color32;
						}

						return &ffi_type_uint;
					} else {
						{
							if(prop->m_nBits == 32) {
								struct dummy_t {
									unsigned int val{256};
								} dummy;

								gsdk::DVariant out{};
								proxy(prop, static_cast<const void *>(&dummy), static_cast<const void *>(&dummy.val), &out, 0, static_cast<int>(gsdk::INVALID_EHANDLE_INDEX));
								if(out.m_Int == 65536) {
									return &ffi_type_color32;
								}
							}
						}

						{
							if(prop->m_nBits == gsdk::NUM_NETWORKED_EHANDLE_BITS) {
								struct dummy_t {
									gsdk::EHANDLE val{};
								} dummy;

								gsdk::DVariant out{};
								proxy(prop, static_cast<const void *>(&dummy), static_cast<const void *>(&dummy.val), &out, 0, static_cast<int>(gsdk::INVALID_EHANDLE_INDEX));
								if(out.m_Int == gsdk::INVALID_NETWORKED_EHANDLE_VALUE) {
									return &ffi_type_ehandle;
								}
							}
						}

						return &ffi_type_uint;
					}
				} else {
					if(proxy == std_proxies->m_Int8ToInt32) {
						return &ffi_type_schar;
					} else if(proxy == std_proxies->m_Int16ToInt32) {
						return &ffi_type_sshort;
					} else if(proxy == std_proxies->m_Int32ToInt32) {
						return &ffi_type_sint;
					} else {
						{
							struct dummy_t {
								short val{SHRT_MAX-1};
							} dummy;

							gsdk::DVariant out{};
							proxy(prop, static_cast<const void *>(&dummy), static_cast<const void *>(&dummy.val), &out, 0, static_cast<int>(gsdk::INVALID_EHANDLE_INDEX));
							if(out.m_Int == dummy.val+1) {
								return &ffi_type_sshort;
							}
						}

						return &ffi_type_sint;
					}
				}
			}
			case gsdk::DPT_Float:
			return &ffi_type_float;
			case gsdk::DPT_Vector: {
				if(prop->m_fLowValue == 0.0f && prop->m_fHighValue == 360.0f) {
					return &ffi_type_qangle;
				} else {
					return &ffi_type_vector;
				}
			}
			case gsdk::DPT_VectorXY:
			return &ffi_type_vector;
			case gsdk::DPT_String: {
				return &ffi_type_cstr;
			}
			case gsdk::DPT_Array:
			return nullptr;
			case gsdk::DPT_DataTable:
			return nullptr;
			default:
			return nullptr;
		}
	}
}
