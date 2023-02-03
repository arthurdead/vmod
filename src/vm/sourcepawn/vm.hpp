#pragma once

#include "../../gsdk/config.hpp"
#include "../../vscript/vscript.hpp"
#include "../../gsdk/tier0/dbg.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include "../vm_shared.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow-field-in-constructor"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wweak-vtables"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wimplicit-int-conversion"
#pragma GCC diagnostic ignored "-Wsigned-enum-bitfield"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Winconsistent-missing-destructor-override"
#pragma GCC diagnostic ignored "-Wsuggest-destructor-override"
#include <sp_vm_api.h>
#include <compile-context.h>
#include <compile-options.h>
#include <source-manager.h>
#include <source-file.h>
#include <lexer.h>
#include <parser.h>
#include <errors.h>
#include <semantics.h>
#include <code-generator.h>
#include <assembler.h>
#include <sc.h>
#pragma GCC diagnostic pop

#ifndef __VMOD_USING_CUSTOM_VM
	#error
#endif

namespace vmod
{
	class main;
}

namespace vmod::vm
{
	enum class sp_object_type : int
	{
		unknown,
		runtime,
		function,
	};

	static_assert(sizeof(sp_object_type) == sizeof(int));

	struct sp_object
	{
		~sp_object() noexcept;

		sp_object_type type{sp_object_type::unknown};

		static_assert(sizeof(cell_t) >= sizeof(funcid_t));

		union {
			SourcePawn::IPluginRuntime *runtime;
			SourcePawn::IPluginFunction *function;
			cell_t value;
		};

		inline sp_object(SourcePawn::IPluginRuntime *value_) noexcept
			: type{sp_object_type::runtime}, runtime{value_}
		{
		}

		inline sp_object(SourcePawn::IPluginFunction *value_) noexcept
			: type{sp_object_type::function}, function{value_}
		{
		}

	private:
		sp_object() = delete;
		sp_object(const sp_object &) = delete;
		sp_object &operator=(const sp_object &) = delete;
		sp_object(sp_object &&) = delete;
		sp_object &operator=(sp_object &&) = delete;
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class sourcepawn final : public gsdk::IScriptVM
	{
		friend class vmod::main;

	public:
		sourcepawn() noexcept = default;
		virtual ~sourcepawn() noexcept;

		bool Init() override;
		void Shutdown() override;
		bool ConnectDebugger() override;
		void DisconnectDebugger() override;
		gsdk::ScriptLanguage_t GetLanguage() override;
		const char *GetLanguageName() override;
		gsdk::HINTERNALVM GetInternalVM() __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
		void AddSearchPath(const char *) override;
		bool ForwardConsoleCommand(const gsdk::CCommandContext &, const gsdk::CCommand &) __VMOD_CUSTOM_VM_L4D2_OVERRIDE;
		bool Frame(float) override;
		gsdk::ScriptStatus_t Run(const char *, bool = true) override;
		gsdk::HSCRIPT CompileScript(const char *, const char * = nullptr) override;
		void ReleaseScript(gsdk::HSCRIPT) override;
		gsdk::ScriptStatus_t Run(gsdk::HSCRIPT, gsdk::HSCRIPT = nullptr, bool = true) override;
		gsdk::ScriptStatus_t Run(gsdk::HSCRIPT, bool) override;
		gsdk::HSCRIPT CreateScope_impl(const char *, gsdk::HSCRIPT = nullptr) override;
		gsdk::HSCRIPT ReferenceScope(gsdk::HSCRIPT) __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE;
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

		gsdk::HSCRIPT CompileScript_strict(const char *, const char * = nullptr) noexcept;

	private:
		struct compile_res
		{
			std::filesystem::path bin_path;
			bool success;

			inline bool operator!() const noexcept
			{ return success; }
			inline operator bool() const noexcept
			{ return success; }

			inline operator std::filesystem::path &&() && noexcept
			{ return std::move(bin_path); }
			inline operator const std::filesystem::path &() const noexcept
			{ return bin_path; }

			inline operator const char *() const noexcept
			{ return bin_path.c_str(); }
		};

		bool push(const gsdk::ScriptVariant_t &value, SourcePawn::IPluginFunction *func) noexcept;

		compile_res compile_code(const char *code, bool strict) noexcept;
		compile_res cleanup_compile(bool success, std::string &&hash, ParseTree *tree) noexcept;

		gsdk::HSCRIPT load(compile_res &&res) noexcept;

		SourcePawn::ISourcePawnEnvironment *env{nullptr};
		SourcePawn::ISourcePawnEngine2 *api{nullptr};

		CompileContext compiler_ctx;

		std::filesystem::path src_dir;
		std::filesystem::path bin_dir;

	private:
		sourcepawn(const sourcepawn &) = delete;
		sourcepawn &operator=(const sourcepawn &) = delete;
		sourcepawn(sourcepawn &&) = delete;
		sourcepawn &operator=(sourcepawn &&) = delete;
	};
	#pragma GCC diagnostic pop
}
