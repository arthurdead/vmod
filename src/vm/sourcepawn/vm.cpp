#define __VMOD_COMPILING_SOURCEPAWN_VM

#include "vm.hpp"
#include "../../main.hpp"
#include "../../filesystem.hpp"

#include <zlib.h>

namespace vmod::vm
{
	sourcepawn::~sourcepawn() noexcept {}

	static inline sp_object *vs_cast(gsdk::HSCRIPT obj) noexcept
	{ return __builtin_bit_cast(sp_object *, obj); }
	static inline gsdk::HSCRIPT vs_cast(sp_object *obj) noexcept
	{ return __builtin_bit_cast(gsdk::HSCRIPT, obj); }

	bool sourcepawn::Init()
	{
		using namespace std::literals::string_view_literals;
		using namespace std::literals::string_literals;

		env = SourcePawn::ISourcePawnEnvironment::New();
		if(!env) {
			return false;
		}

		api = env->APIv2();

		api->SetJitEnabled(true);

		const std::filesystem::path &root_dir{main::instance().root_dir()};

		src_dir = root_dir;
		src_dir /= "tmp/sourcepawn"sv;

		bin_dir = src_dir;

		CompileOptions *opts{compiler_ctx.options()};
		opts->need_semicolon = true;
		opts->require_newdecls = true;
		opts->verbosity = 0;

		compiler_ctx.set_verify_output(false);

		compiler_ctx.CreateGlobalScope();
		compiler_ctx.InitLexer();

		DefineConstant(compiler_ctx, compiler_ctx.atom("INVALID_FUNCTION"s), -1, compiler_ctx.types()->tag_nullfunc());
		DefineConstant(compiler_ctx, compiler_ctx.atom("cellmax"s), INT_MAX, 0);
		DefineConstant(compiler_ctx, compiler_ctx.atom("cellmin"s), INT_MIN, 0);

		return true;
	}

	void sourcepawn::Shutdown()
	{
		env->Shutdown();
		api = nullptr;
		env = nullptr;
	}

	sp_object::~sp_object() noexcept
	{
		switch(type) {
			case sp_object_type::runtime:
			delete runtime;
			break;
			default:
			break;
		}
	}

	bool sourcepawn::ConnectDebugger()
	{
		return true;
	}

	void sourcepawn::DisconnectDebugger()
	{
		
	}

	gsdk::ScriptLanguage_t sourcepawn::GetLanguage() const
	{
		return gsdk::SL_SOURCEPAWN;
	}

	const char *sourcepawn::GetLanguageName() const
	{
		return "sourcepawn";
	}

	gsdk::HINTERNALVM sourcepawn::GetInternalVM()
	{
		return nullptr;
	}

	void sourcepawn::AddSearchPath(const char *)
	{
		
	}

	bool sourcepawn::ForwardConsoleCommand(const gsdk::CCommandContext &, const gsdk::CCommand &)
	{
		return false;
	}

	bool sourcepawn::Frame(float)
	{
		return true;
	}

	sourcepawn::compile_res sourcepawn::compile_code(const char *code, bool strict) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::size_t hash{std::hash<const char *>{}(code)};

		std::string buff;
		buff.resize(12);

		char *begin{buff.data()};
		char *end{begin + 12};

		std::to_chars(begin, end, hash);

		std::filesystem::path path{src_dir};
		path /= begin;
		path.replace_extension(".sp"sv);

		write_file(path, reinterpret_cast<const unsigned char *>(code), std::strlen(code));

		auto source{compiler_ctx.sources()->Open(path, {})};
		source->set_is_main_file();

		compiler_ctx.set_inpf_org(source);

		compiler_ctx.lexer()->Init(source);

		Parser parser{compiler_ctx};

		AutoCountErrors errors;
		ParseTree *tree{parser.Parse()};
		if(!tree || !errors.ok()) {
			return cleanup_compile(false, begin, tree);
		}

		errors.Reset();

		Semantics sema{compiler_ctx, tree};
		{
			SemaContext sc{&sema};
			sema.set_context(&sc);

			StmtList *stmts{tree->stmts()};

			if(!stmts->EnterNames(sc) || !errors.ok()) {
				return cleanup_compile(false, begin, tree);
			}

			errors.Reset();
			if(!stmts->Bind(sc) || !errors.ok()) {
				return cleanup_compile(false, begin, tree);
			}

			sema.set_context(nullptr);

			errors.Reset();
			if(!sema.Analyze() || !errors.ok()) {
				return cleanup_compile(false, begin, tree);
			}

			stmts->ProcessUses(sc);
		}

		return cleanup_compile(true, begin, tree);
	}

	sourcepawn::compile_res sourcepawn::cleanup_compile(bool success, std::string &&hash, ParseTree *tree) noexcept
	{
		using namespace std::literals::string_view_literals;

		ReportManager *reports{compiler_ctx.reports()};

		unsigned int numerr{reports->NumErrorMessages()};

		if(!success && numerr == 0) {
			::error(423);
			++numerr;
		}

		success = (numerr == 0);

		compiler_ctx.set_shutting_down();
		reports->DumpErrorReport(true);

		CodeGenerator cg{compiler_ctx, tree};
		if(tree && success) {
			success = cg.Generate();
		}

		compile_res res;

		if(success) {
			res.bin_path = bin_dir;
			res.bin_path /= hash;
			res.bin_path.replace_extension(".smx"sv);

			success &= assemble(compiler_ctx, cg, res.bin_path.c_str(), Z_BEST_COMPRESSION);
		}

		reports->DumpErrorReport(true);

		compiler_ctx.set_inpf_org(nullptr);

		res.success = success;

		return res;
	}

	gsdk::ScriptStatus_t sourcepawn::Run(const char *code, bool wait)
	{
		auto compile_res{compile_code(code, false)};
		if(!compile_res.success) {
			return gsdk::SCRIPT_ERROR;
		}

		SourcePawn::IPluginRuntime *runtime{api->LoadBinaryFromFile(compile_res.bin_path.c_str(), nullptr, 0)};
		if(!runtime) {
			return gsdk::SCRIPT_ERROR;
		}

		SourcePawn::IPluginFunction *main{runtime->GetFunctionByName("main")};
		if(!main) {
			delete runtime;
			return gsdk::SCRIPT_ERROR;
		}

		if(main->Execute(nullptr) != SP_ERROR_NONE) {
			delete runtime;
			return gsdk::SCRIPT_ERROR;
		}

		delete runtime;

		if(wait) {
			return gsdk::SCRIPT_DONE;
		} else {
			return gsdk::SCRIPT_RUNNING;
		}
	}

	gsdk::HSCRIPT sourcepawn::load(compile_res &&res) noexcept
	{
		SourcePawn::IPluginRuntime *runtime{api->LoadBinaryFromFile(res.bin_path.c_str(), nullptr, 0)};
		if(!runtime) {
			return gsdk::INVALID_HSCRIPT;
		}

		return vs_cast(new sp_object{runtime});
	}

	gsdk::HSCRIPT sourcepawn::CompileScript_strict(const char *code, const char *name) noexcept
	{
		auto res{compile_code(code, true)};
		if(!res.success) {
			return gsdk::INVALID_HSCRIPT;
		}

		return load(std::move(res));
	}

	gsdk::HSCRIPT sourcepawn::CompileScript(const char *code, const char *name)
	{
		auto res{compile_code(code, false)};
		if(!res.success) {
			return gsdk::INVALID_HSCRIPT;
		}

		return load(std::move(res));
	}

	void sourcepawn::ReleaseScript(gsdk::HSCRIPT object)
	{
		delete vs_cast(object);
	}

	gsdk::ScriptStatus_t sourcepawn::Run(gsdk::HSCRIPT object, gsdk::HSCRIPT scope, bool wait)
	{
		return Run(object, wait);
	}

	gsdk::ScriptStatus_t sourcepawn::Run(gsdk::HSCRIPT object, bool wait)
	{
		SourcePawn::IPluginFunction *main{vs_cast(object)->runtime->GetFunctionByName("main")};
		if(!main) {
			return gsdk::SCRIPT_ERROR;
		}

		if(main->Execute(nullptr) != SP_ERROR_NONE) {
			return gsdk::SCRIPT_ERROR;
		}

		if(wait) {
			return gsdk::SCRIPT_DONE;
		} else {
			return gsdk::SCRIPT_RUNNING;
		}
	}

	gsdk::HSCRIPT sourcepawn::CreateScope_impl(const char *name, gsdk::HSCRIPT parent)
	{
		return gsdk::INVALID_HSCRIPT;
	}

	gsdk::HSCRIPT sourcepawn::ReferenceScope(gsdk::HSCRIPT scope)
	{
		return gsdk::INVALID_HSCRIPT;
	}

	void sourcepawn::ReleaseScope(gsdk::HSCRIPT scope)
	{
		
	}

	gsdk::HSCRIPT sourcepawn::LookupFunction_impl(const char *name, gsdk::HSCRIPT scope)
	{
		return gsdk::INVALID_HSCRIPT;
	}

	void sourcepawn::ReleaseFunction(gsdk::HSCRIPT object)
	{
		
	}

	bool sourcepawn::push(const gsdk::ScriptVariant_t &var, SourcePawn::IPluginFunction *func) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_VOID: {
				func->PushCell(0);
				return true;
			}
			case gsdk::FIELD_TIME:
			case gsdk::FIELD_FLOAT: {
				func->PushFloat(var.m_float);
				return true;
			}
			case gsdk::FIELD_FLOAT64: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				func->PushFloat(static_cast<float>(var.m_double));
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				func->PushFloat(var.m_float);
			#else
				#error
			#endif
				return true;
			}
			case gsdk::FIELD_STRING: {
				const char *ccstr{gsdk::vscript::STRING(var.m_tstr)};
				func->PushStringEx(const_cast<char *>(ccstr), std::strlen(ccstr), SM_PARAM_STRING_UTF8|SM_PARAM_STRING_COPY, 0);
				return true;
			}
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME:
			case gsdk::FIELD_CSTRING: {
				func->PushStringEx(const_cast<char *>(var.m_ccstr), std::strlen(var.m_ccstr), SM_PARAM_STRING_UTF8|SM_PARAM_STRING_COPY, 0);
				return true;
			}
			case gsdk::FIELD_CHARACTER: {
				char buff[2]{var.m_char, '\0'};
				func->PushStringEx(buff, 2, SM_PARAM_STRING_UTF8|SM_PARAM_STRING_COPY, 0);
				return true;
			}
			case gsdk::FIELD_SHORT: {
				func->PushCell(var.m_short);
				return true;
			}
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL: {
				if(var.m_int > 0) {
					func->PushCell(var.m_int);
				} else {
					func->PushCell(0);
				}
				return true;
			}
			case gsdk::FIELD_MODELINDEX:
			case gsdk::FIELD_MATERIALINDEX:
			case gsdk::FIELD_TICK:
			case gsdk::FIELD_INTEGER: {
				func->PushCell(var.m_int);
				return true;
			}
			case gsdk::FIELD_UINT32: {
				func->PushCell(static_cast<cell_t>(var.m_uint));
				return true;
			}
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			case gsdk::FIELD_INTEGER64: {
			#if GSDK_ENGINE == GSDK_ENGINE_L4D2
				func->PushCell(static_cast<cell_t>(var.m_longlong));
			#else
				func->PushCell(static_cast<cell_t>(var.m_long));
			#endif
				return true;
			}
		#endif
			case gsdk::FIELD_UINT64: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				func->PushCell(static_cast<cell_t>(var.m_ulonglong));
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				func->PushCell(static_cast<cell_t>(var.m_ulong));
			#else
				#error
			#endif
				return true;
			} 
			case gsdk::FIELD_BOOLEAN: {
				func->PushCell(var.m_bool ? 1 : 0);
				return true;
			}
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				if(var.m_object && var.m_object != gsdk::INVALID_HSCRIPT) {
					switch(var.m_object->type) {
						case sp_object_type::function:
						func->PushCell(static_cast<cell_t>(vs_cast(var.m_object)->function->GetFunctionID()));
						break;
						default:
						return false;
					}
				} else {
					func->PushCell(0);
				}
				return true;
			}
			case gsdk::FIELD_QANGLE:
			return false;
			case gsdk::FIELD_POSITION_VECTOR:
			case gsdk::FIELD_VECTOR:
			return false;
			case gsdk::FIELD_CLASSPTR:
			case gsdk::FIELD_FUNCTION: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				func->PushCell(static_cast<cell_t>(var.m_ulonglong));
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				func->PushCell(static_cast<cell_t>(var.m_ulong));
			#else
				#error
			#endif
				return true;
			}
			case gsdk::FIELD_EHANDLE: {
				return false;
			}
			case gsdk::FIELD_EDICT: {
				return false;
			}
			case gsdk::FIELD_VARIANT: {
				return false;
			}
			case gsdk::FIELD_TYPEUNKNOWN:
			default: {
				return false;
			}
		}
	}

	gsdk::ScriptStatus_t sourcepawn::ExecuteFunction_impl(gsdk::HSCRIPT object, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret, gsdk::HSCRIPT scope, bool wait)
	{
		std::size_t num_args_siz{static_cast<size_t>(num_args)};
		for(std::size_t i{0}; i < num_args_siz; ++i) {
			if(!push(args[i], vs_cast(object)->function)) {
				object->function->Cancel();
				return gsdk::SCRIPT_ERROR;
			}
		}

		cell_t ret_cell{0};
		if(vs_cast(object)->function->Execute(ret ? &ret_cell : nullptr) != SP_ERROR_NONE) {
			return gsdk::SCRIPT_ERROR;
		}

		if(ret) {
			ret->m_type = gsdk::FIELD_INTEGER;
			ret->m_int = ret_cell;
			ret->m_flags = gsdk::SV_NOFLAGS;
		}

		if(wait) {
			return gsdk::SCRIPT_DONE;
		} else {
			return gsdk::SCRIPT_RUNNING;
		}
	}

	void sourcepawn::RegisterFunction_nonvirtual(gsdk::ScriptFunctionBinding_t *binding) noexcept
	{
		
	}

	void sourcepawn::RegisterFunction(gsdk::ScriptFunctionBinding_t *binding)
	{
		RegisterFunction_nonvirtual(binding);
	}

	bool sourcepawn::RegisterClass_nonvirtual(gsdk::ScriptClassDesc_t *desc) noexcept
	{
		return false;
	}

	bool sourcepawn::RegisterClass(gsdk::ScriptClassDesc_t *desc)
	{
		return RegisterClass_nonvirtual(desc);
	}

	gsdk::HSCRIPT sourcepawn::RegisterInstance_impl_nonvirtual(gsdk::ScriptClassDesc_t *desc, void *ptr) noexcept
	{
		return gsdk::INVALID_HSCRIPT;
	}

	gsdk::HSCRIPT sourcepawn::RegisterInstance_impl(gsdk::ScriptClassDesc_t *desc, void *ptr)
	{
		return RegisterInstance_impl_nonvirtual(desc, ptr);
	}

	void sourcepawn::SetInstanceUniqeId(gsdk::HSCRIPT object, const char *name)
	{
		
	}

	void sourcepawn::RemoveInstance(gsdk::HSCRIPT object)
	{
		
	}

	void *sourcepawn::GetInstanceValue_impl(gsdk::HSCRIPT object, gsdk::ScriptClassDesc_t *desc)
	{
		return nullptr;
	}

	bool sourcepawn::GenerateUniqueKey(const char *root, char *buff, int len)
	{
		return false;
	}

	bool sourcepawn::ValueExists(gsdk::HSCRIPT object, const char *name)
	{
		return false;
	}

	bool sourcepawn::SetValue_nonvirtual(gsdk::HSCRIPT object, const char *name, const char *value) noexcept
	{
		return false;
	}

	bool sourcepawn::SetValue(gsdk::HSCRIPT object, const char *name, const char *value)
	{
		return SetValue_nonvirtual(object, name, value);
	}

	bool sourcepawn::SetValue_impl_nonvirtual(gsdk::HSCRIPT object, const char *name, const gsdk::ScriptVariant_t &value) noexcept
	{
		return false;
	}

	bool sourcepawn::SetValue_impl(gsdk::HSCRIPT object, const char *name, const gsdk::ScriptVariant_t &value)
	{
		return SetValue_impl_nonvirtual(object, name, value);
	}

	bool sourcepawn::SetValue_impl(gsdk::HSCRIPT object, int i, const gsdk::ScriptVariant_t &value)
	{
		return false;
	}

	void sourcepawn::CreateTable_impl(gsdk::ScriptVariant_t &value)
	{
		
	}

	bool sourcepawn::IsTable_nonvirtual(gsdk::HSCRIPT object) noexcept
	{
		return false;
	}

	bool sourcepawn::IsTable(gsdk::HSCRIPT object)
	{
		return IsTable_nonvirtual(object);
	}

	int sourcepawn::GetNumTableEntries(gsdk::HSCRIPT object) const
	{
		return 0;
	}

	int sourcepawn::GetKeyValue(gsdk::HSCRIPT object, int it, gsdk::ScriptVariant_t *name, gsdk::ScriptVariant_t *value)
	{
		return -1;
	}

	bool sourcepawn::GetValue_impl(gsdk::HSCRIPT object, const char *name, gsdk::ScriptVariant_t *value)
	{
		return false;
	}

	bool sourcepawn::GetValue_impl(gsdk::HSCRIPT object, int it, gsdk::ScriptVariant_t *value)
	{
		return false;
	}

	bool sourcepawn::GetScalarValue(gsdk::HSCRIPT object, gsdk::ScriptVariant_t *value)
	{
		return false;
	}

	void sourcepawn::ReleaseValue(gsdk::ScriptVariant_t &value)
	{
		
	}

	bool sourcepawn::ClearValue(gsdk::HSCRIPT object, const char *name)
	{
		return false;
	}

	void sourcepawn::CreateArray_impl_nonvirtual(gsdk::ScriptVariant_t &value) noexcept
	{
		
	}

	void sourcepawn::CreateArray_impl(gsdk::ScriptVariant_t &value)
	{
		CreateArray_impl_nonvirtual(value);
	}

	bool sourcepawn::IsArray_nonvirtual(gsdk::HSCRIPT object) noexcept
	{
		return false;
	}

	bool sourcepawn::IsArray(gsdk::HSCRIPT object)
	{
		return IsArray_nonvirtual(object);
	}

	int sourcepawn::GetArrayCount_nonvirtual(gsdk::HSCRIPT object) noexcept
	{
		return 0;
	}

	int sourcepawn::GetArrayCount(gsdk::HSCRIPT object)
	{
		return GetArrayCount_nonvirtual(object);
	}

	void sourcepawn::ArrayAddToTail(gsdk::HSCRIPT object, const gsdk::ScriptVariant_t &value)
	{
		
	}

	void sourcepawn::WriteState(gsdk::CUtlBuffer *)
	{
		
	}

	void sourcepawn::ReadState(gsdk::CUtlBuffer *)
	{
		
	}

	void sourcepawn::CollectGarbage(const char *, bool)
	{
		
	}

	void sourcepawn::RemoveOrphanInstances()
	{
		
	}

	void sourcepawn::DumpState()
	{
		
	}

	void sourcepawn::SetOutputCallback(gsdk::ScriptOutputFunc_t)
	{
		
	}

	void sourcepawn::SetErrorCallback(gsdk::ScriptErrorFunc_t)
	{
		
	}

	bool sourcepawn::RaiseException_impl(const char *)
	{
		return false;
	}

	gsdk::HSCRIPT sourcepawn::GetRootTable()
	{
		return gsdk::INVALID_HSCRIPT;
	}

	gsdk::HSCRIPT sourcepawn::CopyHandle(gsdk::HSCRIPT object)
	{
		return gsdk::INVALID_HSCRIPT;
	}

	gsdk::HIDENTITY sourcepawn::GetIdentity(gsdk::HSCRIPT object)
	{
		return sp_object_type::unknown;
	}

	gsdk::CSquirrelMetamethodDelegateImpl *sourcepawn::MakeSquirrelMetamethod_Get_impl(gsdk::HSCRIPT &, const char *, gsdk::ISquirrelMetamethodDelegate *, bool)
	{
		return nullptr;
	}

	void sourcepawn::DestroySquirrelMetamethod_Get(gsdk::CSquirrelMetamethodDelegateImpl *)
	{
		
	}

	int sourcepawn::GetKeyValue2(gsdk::HSCRIPT, int, gsdk::ScriptVariant_t *, gsdk::ScriptVariant_t *)
	{
		return -1;
	}
}
