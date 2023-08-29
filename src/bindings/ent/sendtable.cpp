#include "sendtable.hpp"
#include "../../main.hpp"

namespace vmod::bindings::ent
{
	ffi::cif sendprop::proxy_cif{&ffi_type_void, {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_sint, &ffi_type_sint}};

	vscript::class_desc<sendprop> sendprop::desc{"ent::sendprop"};
	vscript::class_desc<sendtable> sendtable::desc{"ent::sendtable"};

	sendtable::~sendtable() noexcept {}

	bool sendprop::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!proxy_cif.initialize(FFI_SYSV)) {
			error("vmod: failed to initialize send proxy cif\n"sv);
			return false;
		}

		desc.func(&sendprop::script_hook_proxy, "script_hook_proxy"sv, "hook_proxy"sv)
		.desc("[callback_instance](sendproxy_callback|callback, post, per_client)"sv);

		desc.func(&sendprop::script_type, "script_type"sv, "type"sv)
		.desc("[mem::types::type]"sv);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register sendprop script class\n"sv);
			return false;
		}

		return true;
	}

	void sendprop::unbindings() noexcept
	{
		
	}

	sendprop::sendprop(gsdk::SendProp *prop_) noexcept
		: prop{prop_}, old_proxy{prop_->m_ProxyFn}, type_ptr{guess_type(prop_, prop_->m_ProxyFn, nullptr)}
	{
		type = mem::singleton::instance().find_type(type_ptr);
	}

	sendprop::~sendprop() noexcept
	{
		remove_closure();
	}

	void sendprop::on_sleep() noexcept
	{
		remove_closure();
	}

	void sendprop::on_wake() noexcept
	{
		initialize_closure();
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

	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		if(ffi_prep_closure_loc(closure, &proxy_cif, closure_binding, this, reinterpret_cast<void *>(prop->m_ProxyFn)) != FFI_OK) {
			return false;
		}
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif

		return true;
	}

	void sendprop::remove_closure() noexcept
	{
		if(closure) {
			ffi_closure_free(closure);
			closure = nullptr;
		}

		prop->m_ProxyFn = old_proxy;
	}

	void sendprop::closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept
	{
		sendprop *prop{static_cast<sendprop *>(userptr)};

		if(prop->empty()) {
			ffi_call(closure_cif, reinterpret_cast<void(*)()>(prop->old_proxy), ret, args);
			return;
		}

		gsdk::DVariant *dvar{*static_cast<gsdk::DVariant **>(args[3])};

		vscript::variant vargs[]{
			prop->instance,
			*static_cast<void **>            (args[1]),
			*static_cast<void **>            (args[2]),
			&dvar->m_data,
			*static_cast<int *>              (args[4]),
			*static_cast<int *>              (args[5]),
			nullptr,
		};

		std::memset(dvar->m_data, 0, sizeof(gsdk::DVariant::m_data));
		dvar->m_Type = prop->prop->m_Type;

		if(prop->call_pre(vargs, std::size(vargs))) {
			ffi_call(closure_cif, reinterpret_cast<void(*)()>(prop->old_proxy), ret, args);
		}

		prop->call_post(vargs, std::size(vargs));
	}

	gsdk::HSCRIPT sendprop::script_hook_proxy(gsdk::HSCRIPT callback, bool post, [[maybe_unused]] bool per_client) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!callback || callback == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid callback");
			return gsdk::INVALID_HSCRIPT;
		}

		callback = vm->ReferenceObject(callback);
		if(!callback || callback == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: failed to get callback reference");
			return gsdk::INVALID_HSCRIPT;
		}

		plugin::callback_instance *clbk_instance{new plugin::callback_instance{this, callback, post}};
		if(!clbk_instance->initialize()) {
			delete clbk_instance;
			return gsdk::INVALID_HSCRIPT;
		}

		if(!initialize_closure()) {
			delete clbk_instance;
			vm->RaiseException("vmod: failed to initialize closure");
			return gsdk::INVALID_HSCRIPT;
		}

		return clbk_instance->instance;
	}

	ffi_type *sendprop::guess_type(const gsdk::SendProp *prop, gsdk::SendVarProxyFn proxy, const gsdk::SendTable *table) noexcept
	{
		if(prop->m_Flags & gsdk::SPROP_EXCLUDE) {
			return nullptr;
		}

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

						return &ffi_type_uint32;
					} else if(gsdk::SendProxy_EHandleToInt && proxy == gsdk::SendProxy_EHandleToInt) {
						return &ffi_type_ehandle;
					} else if(gsdk::SendProxy_Color32ToInt && proxy == gsdk::SendProxy_Color32ToInt) {
						return &ffi_type_color32;
					} else {
						auto target_proxy{proxy};
						if(gsdk::SendProxy_UtlVectorElement && proxy == gsdk::SendProxy_UtlVectorElement) {
							target_proxy = static_cast<const gsdk::CSendPropExtra_UtlVector *>(prop->m_pExtraData)->m_ProxyFn;
						}

						{
							if(prop->m_nBits == 32) {
								struct dummy_t {
									unsigned int val{256};
								} dummy;

								gsdk::DVariant out{};
								target_proxy(prop, static_cast<const void *>(&dummy), static_cast<const void *>(&dummy.val), &out, 0, static_cast<int>(gsdk::INVALID_EHANDLE_INDEX));
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
								target_proxy(prop, static_cast<const void *>(&dummy), static_cast<const void *>(&dummy.val), &out, 0, static_cast<int>(gsdk::INVALID_EHANDLE_INDEX));
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
						return &ffi_type_sint32;
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
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wfloat-equal"
				if(prop->m_fLowValue == 0.0f && prop->m_fHighValue == 360.0f) {
					return &ffi_type_qangle;
				} else {
					return &ffi_type_vector;
				}
				#pragma GCC diagnostic pop
			}
			case gsdk::DPT_VectorXY:
			return &ffi_type_vector;
			case gsdk::DPT_String: {
				return &ffi_type_cstr;
			}
			case gsdk::DPT_Array: {
				auto before{prop-1};
				return guess_type(before, before->m_ProxyFn, table);
			}
			case gsdk::DPT_DataTable: {
				if(prop->m_pArrayProp) {
					return guess_type(prop->m_pArrayProp, prop->m_pArrayProp->m_ProxyFn, table);
				}

				return nullptr;
			}
			default:
			return nullptr;
		}
	}

	bool sendtable::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register sendtable script class\n"sv);
			return false;
		}

		return true;
	}

	void sendtable::unbindings() noexcept
	{
		
	}
}
