#include "preprocessor.hpp"
#include "gsdk.hpp"
#include "vmod.hpp"
#include "filesystem.hpp"

namespace vmod
{
	squirrel_preprocessor::squirrel_preprocessor() noexcept
		: file_mgr{fs_opts},
		diageng{&diagids, &diagopts, &diagcl},
		src_mgr{diageng, file_mgr},
		hdr_srch{std::shared_ptr<header_search_options>{new header_search_options}, src_mgr, diageng, lang_opts, nullptr},
		pp{std::make_shared<clang::PreprocessorOptions>(), diageng, lang_opts, src_mgr, hdr_srch, modul_ldr}
	{
		pp.addPPCallbacks(std::unique_ptr<clang::PPCallbacks>{new pp_callbacks{*this}});
	}

	bool squirrel_preprocessor::initialize() noexcept
	{
		using namespace std::literals::string_view_literals;

		vmod_preproc_dump.initialize("vmod_preproc_dump"sv, false);

		std::error_code err;

		std::filesystem::path include_dir{vmod.root_dir()};
		include_dir /= "include"sv;
		std::filesystem::create_directories(include_dir, err);
		llvm::Expected<clang::DirectoryEntryRef> dir_ref{file_mgr.getDirectoryRef(include_dir.native())};
		if(!dir_ref) {
			error("failed to get directory '%s' reference\n", include_dir.c_str());
			return false;
		}

		hdr_srch.AddSearchPath({*dir_ref, clang::SrcMgr::CharacteristicKind::C_User, false}, true);

		include_dir = vmod.game_dir();
		include_dir /= "scripts"sv;
		include_dir /= "vscripts"sv;
		std::filesystem::create_directories(include_dir, err);
		dir_ref = file_mgr.getDirectoryRef(include_dir.native());
		if(!dir_ref) {
			error("failed to get directory '%s' reference\n", include_dir.c_str());
			return false;
		}

		hdr_srch.AddSearchPath({*dir_ref, clang::SrcMgr::CharacteristicKind::C_User, false}, true);

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
			} break;
			case clang::DiagnosticsEngine::Level::Fatal: {
				error("%s: fatal error: %s\n"sv, loc_str.c_str(), __sqpp_err_temp_buff.c_str());
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

		clang::FileID file_id{src_mgr.getOrCreateFileID(*file_ref, clang::SrcMgr::CharacteristicKind::C_User)};
		src_mgr.setMainFileID(file_id);

		pp.EnterMainSourceFile();

		curr_incs = &incs;
		curr_path = &path;

		str.reserve(static_cast<std::size_t>(file_ref->getSize()));

		struct scope_cleanup {
			inline scope_cleanup(squirrel_preprocessor &pp_, std::string &str_, const std::filesystem::path &path_) noexcept
				: pp{pp_}, str{str_}, path{path_}
			{}
			inline ~scope_cleanup() noexcept {
				if(pp.vmod_preproc_dump.get<bool>()) {
					std::filesystem::path pp_path{vmod.root_dir()};
					pp_path /= "plugins_pp"sv;
					pp_path /= path.filename();
					pp_path.replace_extension(".nut.pp"sv);

					write_file(pp_path, reinterpret_cast<const unsigned char *>(str.c_str()), str.length());
				}

				pp.pp.EndSourceFile();

				pp.diageng.Reset();

				pp.curr_incs = nullptr;
				pp.curr_path = nullptr;
			}
			squirrel_preprocessor &pp;
			std::string &str;
			const std::filesystem::path &path;
		};
		scope_cleanup sclp{*this, str, path};

		clang::Token tok;

		do {
			pp.Lex(tok);

			if(diageng.hasUnrecoverableErrorOccurred() ||
				diageng.hasUncompilableErrorOccurred() ||
				diageng.hasFatalErrorOccurred() ||
				diageng.hasErrorOccurred()) {
				return false;
			}

			if(tok.is(clang::tok::eof)) {
				break;
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

			if(tok.isAtStartOfLine()) {
				str += '\n';
			} else if(tok.hasLeadingSpace()) {
				str += ' ';
			}

			clang::IdentifierInfo *idinfo{tok.getIdentifierInfo()};
			if(idinfo) {
				str += idinfo->getName();
				continue;
			}

			std::size_t tok_len{static_cast<std::size_t>(tok.getLength())};

			if(tok.isLiteral()) {
				const char *tok_data{tok.getLiteralData()};
				if(!tok.needsCleaning() && tok_data) {
					str.append(tok_data, tok_len);
					continue;
				}
			}

			if(tok_len < std::size(__sqpp_temp_buff)) {
				bool invalid;
				__sqpp_temp_buff[0] = '\0';
				const char *tmp_buff{__sqpp_temp_buff};
				std::size_t len{static_cast<std::size_t>(pp.getSpelling(tok, tmp_buff, &invalid))};
				if(invalid) {
					error("vmod: invalid token spelling while preprocessing '%s'\n"sv, path.c_str());
					return false;
				}

				str.append(tmp_buff, len);
			} else {
				bool invalid;
				std::string tmp_buff{pp.getSpelling(tok, &invalid)};
				if(invalid) {
					error("vmod: invalid token spelling while preprocessing '%s'\n"sv, path.c_str());
					return false;
				}

				str += std::move(tmp_buff);
			}
		} while(true);

		return true;
	}
}
