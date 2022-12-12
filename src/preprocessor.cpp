#include "preprocessor.hpp"
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/PreprocessorOutputOptions.h>
#include <clang/Frontend/Utils.h>
#include "gsdk.hpp"
#include "vmod.hpp"
#include "filesystem.hpp"
#include "convar.hpp"

namespace vmod
{
	squirrel_preprocessor::squirrel_preprocessor() noexcept
		: file_mgr{fs_opts},
		diagcl{*this},
		diageng{&diagids, &diagopts, &diagcl},
		src_mgr{diageng, file_mgr},
		hdr_srch{std::make_shared<clang::HeaderSearchOptions>(), src_mgr, diageng, lang_opts, nullptr},
		pp{std::make_shared<clang::PreprocessorOptions>(), diageng, lang_opts, src_mgr, hdr_srch, modul_ldr}
	{
		clang::HeaderSearchOptions &hdr_srch_opts{hdr_srch.getHeaderSearchOpts()};
		hdr_srch_opts.UseBuiltinIncludes = false;
		hdr_srch_opts.UseStandardSystemIncludes = false;
		hdr_srch_opts.UseStandardCXXIncludes = false;

		pp.addPPCallbacks(std::unique_ptr<clang::PPCallbacks>{new pp_callbacks{*this}});
	}

	bool squirrel_preprocessor::initialize() noexcept
	{
		using namespace std::literals::string_view_literals;

		clang::HeaderSearchOptions &hdr_srch_opts{hdr_srch.getHeaderSearchOpts()};

		std::filesystem::path include_dir{vmod.root_dir()};
		include_dir /= "include"sv;

		std::error_code err;
		std::filesystem::create_directories(include_dir, err);

		hdr_srch_opts.AddPath(include_dir.native(), clang::frontend::IncludeDirGroup::Angled, false, true);

		include_dir = vmod.game_dir();
		include_dir /= "scripts"sv;
		include_dir /= "vscripts"sv;

		std::filesystem::create_directories(include_dir, err);

		hdr_srch_opts.AddPath(include_dir.native(), clang::frontend::IncludeDirGroup::Angled, false, true);

		return true;
	}

	squirrel_preprocessor::module_loader::~module_loader()
	{
	}

	clang::ModuleLoadResult squirrel_preprocessor::module_loader::loadModule([[maybe_unused]] clang::SourceLocation, [[maybe_unused]] clang::ModuleIdPath, [[maybe_unused]] clang::Module::NameVisibilityKind, [[maybe_unused]] bool)
	{ return nullptr; }

	void squirrel_preprocessor::module_loader::createModuleFromSource([[maybe_unused]] clang::SourceLocation, [[maybe_unused]] clang::StringRef, [[maybe_unused]] clang::StringRef)
	{
	}

	void squirrel_preprocessor::module_loader::makeModuleVisible([[maybe_unused]] clang::Module *, [[maybe_unused]] clang::Module::NameVisibilityKind, [[maybe_unused]] clang::SourceLocation)
	{
	}

	clang::GlobalModuleIndex *squirrel_preprocessor::module_loader::loadGlobalModuleIndex([[maybe_unused]] clang::SourceLocation)
	{ return nullptr; }

	bool squirrel_preprocessor::module_loader::lookupMissingImports([[maybe_unused]] clang::StringRef, [[maybe_unused]] clang::SourceLocation)
	{ return false; }

	squirrel_preprocessor::diagnostic_client::~diagnostic_client()
	{
	}

	static clang::SmallString<4096> __sqpp_err_temp_buff;
	void squirrel_preprocessor::diagnostic_client::HandleDiagnostic(clang::DiagnosticsEngine::Level lvl, const clang::Diagnostic &diaginfo)
	{
		using namespace std::literals::string_view_literals;

		clang::DiagnosticConsumer::HandleDiagnostic(lvl, diaginfo);

		__sqpp_err_temp_buff.resize_for_overwrite(0);
		diaginfo.FormatDiagnostic(__sqpp_err_temp_buff);

		const clang::SourceManager &srcmgr{diaginfo.getSourceManager()};

		const clang::SourceLocation &loc{diaginfo.getLocation()};
		std::string loc_str{loc.printToString(srcmgr)};

		switch(lvl) {
			case clang::DiagnosticsEngine::Level::Ignored: {
				print("%s: %s\n"sv, loc_str.c_str(), __sqpp_err_temp_buff.c_str());
			} break;
			case clang::DiagnosticsEngine::Level::Note: {
				info("%s: note: %s\n"sv, loc_str.c_str(), __sqpp_err_temp_buff.c_str());
			} break;
			case clang::DiagnosticsEngine::Level::Remark: {
				info("%s: remark:%s\n"sv, loc_str.c_str(), __sqpp_err_temp_buff.c_str());
			} break;
			case clang::DiagnosticsEngine::Level::Warning: {
				warning("%s: warning: %s\n"sv, loc_str.c_str(), __sqpp_err_temp_buff.c_str());
			} break;
			case clang::DiagnosticsEngine::Level::Error: {
				error("%s: error: %s\n"sv, loc_str.c_str(), __sqpp_err_temp_buff.c_str());
				pp.fatal = true;
			} break;
			case clang::DiagnosticsEngine::Level::Fatal: {
				error("%s: fatal error: %s\n"sv, loc_str.c_str(), __sqpp_err_temp_buff.c_str());
				pp.fatal = true;
			} break;
		}
	}

	squirrel_preprocessor::pp_callbacks::~pp_callbacks()
	{
	}

	void squirrel_preprocessor::pp_callbacks::PragmaMark(clang::SourceLocation loc, clang::StringRef str)
	{ pp.pp.Diag(loc, pp.diagids.getCustomDiagID(clang::DiagnosticIDs::Level::Remark, str)) << str; }

	void squirrel_preprocessor::pp_callbacks::InclusionDirective([[maybe_unused]] clang::SourceLocation, [[maybe_unused]] const clang::Token &, [[maybe_unused]] clang::StringRef, [[maybe_unused]] bool, [[maybe_unused]] clang::CharSourceRange, const clang::FileEntry *file, clang::StringRef search_path, clang::StringRef relative_path, [[maybe_unused]] const clang::Module *, [[maybe_unused]] clang::SrcMgr::CharacteristicKind)
	{
		if(!file) {
			return;
		}

		std::filesystem::path path{search_path.str()};
		path /= relative_path.str();

		if(!path.is_absolute() || !std::filesystem::exists(path)) {
			return;
		}

		pp.curr_incs->emplace_back(path);
	}

	static char __sqpp_temp_buff[256];
	bool squirrel_preprocessor::preprocess(std::string &str, const std::filesystem::path &path, std::vector<std::filesystem::path> &incs) noexcept
	{
		using namespace std::literals::string_view_literals;

		llvm::Expected<clang::FileEntryRef> file_ref{file_mgr.getFileRef(path.native())};
		if(!file_ref) {
			return false;
		}

		clang::FileID file_id{src_mgr.createFileID(*file_ref, clang::SourceLocation{}, clang::SrcMgr::CharacteristicKind::C_User)};
		src_mgr.setMainFileID(file_id);

		curr_incs = &incs;
		curr_path = &path;

		pp.EnterMainSourceFile();

		std::string out_str;
		out_str.reserve(str.length());

		struct scope_cleanup {
			inline scope_cleanup(squirrel_preprocessor &pp_) noexcept
				: pp{pp_}
			{}
			inline ~scope_cleanup() noexcept {
				pp.pp.EndSourceFile();

				pp.curr_incs = nullptr;
				pp.curr_path = nullptr;

				pp.src_mgr.clearIDTables();
			}
			squirrel_preprocessor &pp;
		};
		scope_cleanup sclp{*this};

		{
			clang::Token tok;

			do {
				pp.Lex(tok);

				if(fatal) {
					return false;
				}

				if(tok.is(clang::tok::comment) ||
					tok.is(clang::tok::eod) ||
					tok.is(clang::tok::annot_module_include) ||
					tok.is(clang::tok::annot_module_begin) ||
					tok.is(clang::tok::annot_module_end) ||
					tok.is(clang::tok::annot_header_unit) ||
					tok.is(clang::tok::code_completion) ||
					tok.isAnnotation()) {
					continue;
				}

				if(tok.is(clang::tok::eof)) {
					break;
				}

				if(tok.isAtStartOfLine()) {
					out_str += '\n';
				} else if(tok.hasLeadingSpace()) {
					out_str += ' ';
				}

				clang::IdentifierInfo *idinfo{tok.getIdentifierInfo()};
				if(idinfo) {
					out_str += idinfo->getName();
				} else if(tok.isLiteral() && !tok.needsCleaning() && tok.getLiteralData()) {
					out_str.append(tok.getLiteralData(), tok.getLength());
				} else if(tok.getLength() < std::size(__sqpp_temp_buff)) {
					__sqpp_temp_buff[0] = '\0';
					const char *tmp_buff{__sqpp_temp_buff};
					std::size_t len{pp.getSpelling(tok, tmp_buff)};
					out_str.append(tmp_buff, len);
				} else {
					out_str += pp.getSpelling(tok);
				}
			} while(true);
		}

		str = std::move(out_str);

	#if 0
		{
			std::filesystem::path pp_path{vmod.root_dir()};
			pp_path /= "plugins_pp"sv;
			pp_path /= path.filename();
			pp_path.replace_extension(".nut.pp"sv);

			write_file(pp_path, reinterpret_cast<const unsigned char *>(str.c_str()), str.length());
		}
	#endif

		return true;
	}
}
