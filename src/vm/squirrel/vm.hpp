#pragma once

#include "../../gsdk/config.hpp"
#include "../../vscript/vscript.hpp"
#include "../../gsdk/tier0/dbg.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include "../vm_shared.hpp"

#ifdef __VMOD_USING_QUIRREL
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wdocumentation"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <sqmodules.h>
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#endif

#ifndef __VMOD_USING_CUSTOM_VM
	#error
#endif

namespace vmod
{
	class main;
}

namespace vmod::vm
{
	static_assert(sizeof(SQObjectType) == sizeof(int));

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class squirrel final : public IScriptVM
	{
		friend class vmod::main;

	public:
		squirrel() noexcept = default;
		virtual ~squirrel() noexcept;

		bool Init() override;
		void Shutdown() override;
		bool ConnectDebugger() override;
		void DisconnectDebugger() override;
		gsdk::ScriptLanguage_t GetLanguage() const override;
		const char *GetLanguageName() const override;
		gsdk::HINTERNALVM GetInternalVM() __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		void AddSearchPath(const char *) override;
		bool ForwardConsoleCommand(const gsdk::CCommandContext &, const gsdk::CCommand &) __VMOD_CUSTOM_VM_L4D2_OVERRIDE;
		bool Frame(float) override;
		gsdk::ScriptStatus_t Run(const char *, bool = true) override;
		gsdk::HSCRIPT CompileScript_impl(const char *, const char * = nullptr) override;
		void ReleaseScript(gsdk::HSCRIPT) override;
		gsdk::ScriptStatus_t Run(gsdk::HSCRIPT, gsdk::HSCRIPT = nullptr, bool = true) override;
		gsdk::ScriptStatus_t Run(gsdk::HSCRIPT, bool) override;
		gsdk::HSCRIPT CreateScope_impl(const char *, gsdk::HSCRIPT = nullptr) override;
		gsdk::HSCRIPT ReferenceScope_impl(gsdk::HSCRIPT) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		void ReleaseScope(gsdk::HSCRIPT) override;
		gsdk::HSCRIPT LookupFunction_impl(const char *, gsdk::HSCRIPT = nullptr) override;
		void ReleaseFunction(gsdk::HSCRIPT) override;
		gsdk::ScriptStatus_t ExecuteFunction_impl(gsdk::HSCRIPT, const gsdk::ScriptVariant_t *, int, gsdk::ScriptVariant_t *, gsdk::HSCRIPT, bool) override;
		void RegisterFunction(gsdk::ScriptFunctionBinding_t *) override;
		bool RegisterClass(gsdk::ScriptClassDesc_t *) override;
		gsdk::HSCRIPT RegisterInstance_impl(gsdk::ScriptClassDesc_t *, void *) override;
		void SetInstanceUniqeId(gsdk::HSCRIPT, const char *) override;
		void RemoveInstance(gsdk::HSCRIPT) override;
		void *GetInstanceValue_impl(gsdk::HSCRIPT, gsdk::ScriptClassDesc_t * = nullptr) override;
		bool GenerateUniqueKey(const char *, char *, int) override;
		bool ValueExists(gsdk::HSCRIPT, const char *) override;
		bool SetValue(gsdk::HSCRIPT, const char *, const char *) override;
		bool SetValue_impl(gsdk::HSCRIPT, const char *, const gsdk::ScriptVariant_t &) override;
		bool SetValue_impl(gsdk::HSCRIPT, int, const gsdk::ScriptVariant_t &) __VMOD_CUSTOM_VM_L4D2_OVERRIDE;
		void CreateTable_impl(gsdk::ScriptVariant_t &) override;
		bool IsTable(gsdk::HSCRIPT) __VMOD_CUSTOM_VM_L4D2_OVERRIDE;
		int GetNumTableEntries(gsdk::HSCRIPT) const override;
		int GetKeyValue(gsdk::HSCRIPT, int, gsdk::ScriptVariant_t *, gsdk::ScriptVariant_t *) override;
		bool GetValue_impl(gsdk::HSCRIPT, const char *, gsdk::ScriptVariant_t *) override;
		bool GetValue_impl(gsdk::HSCRIPT, int, gsdk::ScriptVariant_t *) __VMOD_CUSTOM_VM_L4D2_OVERRIDE;
		bool GetScalarValue(gsdk::HSCRIPT, gsdk::ScriptVariant_t *) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		void ReleaseValue(gsdk::ScriptVariant_t &) override;
		bool ClearValue(gsdk::HSCRIPT, const char *) override;
		void CreateArray_impl(gsdk::ScriptVariant_t &) __VMOD_CUSTOM_VM_L4D2_OVERRIDE;
		bool IsArray(gsdk::HSCRIPT) __VMOD_CUSTOM_VM_L4D2_OVERRIDE;
		int GetArrayCount(gsdk::HSCRIPT) __VMOD_CUSTOM_VM_L4D2_OVERRIDE;
		void ArrayAddToTail(gsdk::HSCRIPT, const gsdk::ScriptVariant_t &) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		void WriteState(gsdk::CUtlBuffer *) override;
		void ReadState(gsdk::CUtlBuffer *) override;
		void CollectGarbage(const char *, bool) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		void RemoveOrphanInstances() __VMOD_CUSTOM_VM_NOT_L4D2_OVERRIDE;
		void DumpState() override;
		void SetOutputCallback(gsdk::ScriptOutputFunc_t) override;
		void SetErrorCallback(gsdk::ScriptErrorFunc_t) override;
		bool RaiseException_impl(const char *) override;
		gsdk::HSCRIPT GetRootTable() __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		gsdk::HSCRIPT CopyHandle(gsdk::HSCRIPT) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		gsdk::HIDENTITY GetIdentity(gsdk::HSCRIPT) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		gsdk::CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get_impl(gsdk::HSCRIPT &, const char *, gsdk::ISquirrelMetamethodDelegate *, bool) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		void DestroySquirrelMetamethod_Get(gsdk::CSquirrelMetamethodDelegateImpl *) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		int GetKeyValue2(gsdk::HSCRIPT, int, gsdk::ScriptVariant_t *, gsdk::ScriptVariant_t *) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;

		void RegisterFunction_nonvirtual(gsdk::ScriptFunctionBinding_t *) noexcept;
		bool RegisterClass_nonvirtual(gsdk::ScriptClassDesc_t *) noexcept;
		gsdk::HSCRIPT RegisterInstance_impl_nonvirtual(gsdk::ScriptClassDesc_t *, void *) noexcept;
		bool SetValue_nonvirtual(gsdk::HSCRIPT, const char *, const char *) noexcept;
		bool SetValue_impl_nonvirtual(gsdk::HSCRIPT, const char *, const gsdk::ScriptVariant_t &) noexcept;
		void CreateArray_impl_nonvirtual(gsdk::ScriptVariant_t &) noexcept;
		int GetArrayCount_nonvirtual(gsdk::HSCRIPT) noexcept;
		bool IsArray_nonvirtual(gsdk::HSCRIPT) noexcept;
		bool IsTable_nonvirtual(gsdk::HSCRIPT) noexcept;

		gsdk::ScriptHandleWrapper_t CompileScript_strict(const char *, const char * = nullptr) noexcept;

		HSQOBJECT vector_class;
		HSQOBJECT qangle_class;

	private:
		static char err_buff[gsdk::MAXPRINTMSG];
		static void error_func(HSQUIRRELVM vm, const SQChar *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));
		static char print_buff[gsdk::MAXPRINTMSG];
		static void print_func(HSQUIRRELVM vm, const SQChar *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));

		static SQInteger static_func_call(HSQUIRRELVM vm);
		static SQInteger member_func_call(HSQUIRRELVM vm);

		static SQInteger metamethod_get_call(HSQUIRRELVM vm);

		static SQInteger instance_external_ctor(HSQUIRRELVM vm);

		static char instance_str_buff[gsdk::MAXPRINTMSG];
		static SQInteger instance_str(HSQUIRRELVM vm);

		static SQInteger instance_valid(HSQUIRRELVM vm);
		static SQInteger instance_release_generic(SQUserPointer userptr, SQInteger size);
		static SQInteger instance_release_external(SQUserPointer userptr, SQInteger size);

		SQInteger func_call_impl(const gsdk::ScriptFunctionBinding_t *info, void *obj, std::vector<gsdk::ScriptVariant_t> &args) noexcept;

		bool push(const gsdk::ScriptVariant_t &var) noexcept;
		bool get(HSQOBJECT obj, gsdk::ScriptVariant_t &var, bool scalar=false) noexcept;
		bool get(SQInteger idx, gsdk::ScriptVariant_t &var) noexcept;

		static bool typemask_for_type(std::string &typemask, gsdk::ScriptDataType_t type) noexcept;

		bool register_func(const gsdk::ScriptClassDesc_t *classinfo, const gsdk::ScriptFunctionBinding_t *info, std::string_view name_str) noexcept;

		bool register_class(const gsdk::ScriptClassDesc_t *info, std::string &&classname_str, HSQOBJECT **obj) noexcept;

		gsdk::HSCRIPT compile_script() noexcept;

		void get_obj(gsdk::ScriptVariant_t &value) noexcept;

		bool debug_vm{false};

		gsdk::ScriptOutputFunc_t output_callback{nullptr};
		gsdk::ScriptErrorFunc_t err_callback{nullptr};

		HSQUIRRELVM impl{nullptr};

	#ifdef __VMOD_USING_QUIRREL
		std::unique_ptr<SqModules> modules;
	#endif

		std::size_t unique_ids{0};

		bool vector_registered{false};

		bool qangle_registered{false};

		HSQOBJECT create_scope_func;
		bool got_create_scope{false};

		HSQOBJECT release_scope_func;
		bool got_release_scope{false};

		HSQOBJECT register_func_desc;
		bool got_reg_func_desc{false};

		HSQOBJECT root_table;
		bool got_root_table{false};

		HSQOBJECT last_exception;
		bool got_last_exception{false};

		struct instance_info_t
		{
			instance_info_t(instance_info_t &&) noexcept = default;
			instance_info_t &operator=(instance_info_t &&) noexcept = default;

			inline instance_info_t(const gsdk::ScriptClassDesc_t *info_, void *ptr_) noexcept
				: classinfo{info_}, ptr{ptr_}
			{
			}

			const gsdk::ScriptClassDesc_t *classinfo{nullptr};
			void *ptr{nullptr};
			std::string id;

		private:
			instance_info_t() = delete;
			instance_info_t(const instance_info_t &) = delete;
			instance_info_t &operator=(const instance_info_t &) = delete;
		};

		struct func_info_t
		{
			func_info_t(func_info_t &&) noexcept = default;
			func_info_t &operator=(func_info_t &&) noexcept = default;

			inline func_info_t(const gsdk::ScriptFunctionBinding_t *ptr_) noexcept
				: ptr{ptr_}
			{
			}

			const gsdk::ScriptFunctionBinding_t *ptr{nullptr};

		private:
			func_info_t() = delete;
			func_info_t(const func_info_t &) = delete;
			func_info_t &operator=(const func_info_t &) = delete;
		};

		using registered_funcs_t = std::unordered_map<std::string, func_info_t>;

		registered_funcs_t registered_funcs;

		struct class_info_t
		{
			class_info_t(class_info_t &&) noexcept = default;
			class_info_t &operator=(class_info_t &&) noexcept = default;

			inline class_info_t(const gsdk::ScriptClassDesc_t *ptr_) noexcept
				: ptr{ptr_}
			{
			}

			const gsdk::ScriptClassDesc_t *ptr{nullptr};
			HSQOBJECT obj;

			registered_funcs_t registered_funcs;

		private:
			class_info_t() = delete;
			class_info_t(const class_info_t &) = delete;
			class_info_t &operator=(const class_info_t &) = delete;
		};

		using registered_classes_t = std::unordered_map<std::string, std::unique_ptr<class_info_t>>;

		registered_classes_t registered_classes;

		registered_classes_t::iterator last_registered_class;

	private:
		squirrel(const squirrel &) = delete;
		squirrel &operator=(const squirrel &) = delete;
		squirrel(squirrel &&) = delete;
		squirrel &operator=(squirrel &&) = delete;
	};
	#pragma GCC diagnostic pop
}
