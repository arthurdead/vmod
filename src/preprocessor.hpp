#pragma once

#include <clang/Lex/Preprocessor.h>
#include <clang/Serialization/PCHContainerOperations.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/HeaderSearchOptions.h>
#include <clang/Basic/DiagnosticLex.h>
#include <string>
#include <filesystem>
#include <memory>
#include "convar.hpp"

namespace vmod
{
	class squirrel_preprocessor final
	{
	public:
		squirrel_preprocessor() noexcept;

		bool preprocess(std::string &str, const std::filesystem::path &path, std::vector<std::filesystem::path> &incs) noexcept;

	private:
		friend class vmod;

		class module_loader final : public clang::ModuleLoader
		{
			friend class squirrel_preprocessor;

			~module_loader() override;

			clang::ModuleLoadResult loadModule([[maybe_unused]] clang::SourceLocation, [[maybe_unused]] clang::ModuleIdPath, [[maybe_unused]] clang::Module::NameVisibilityKind, [[maybe_unused]] bool) override;
			void createModuleFromSource([[maybe_unused]] clang::SourceLocation, [[maybe_unused]] clang::StringRef, [[maybe_unused]] clang::StringRef) override;
			void makeModuleVisible([[maybe_unused]] clang::Module *, [[maybe_unused]] clang::Module::NameVisibilityKind, [[maybe_unused]] clang::SourceLocation) override;
			clang::GlobalModuleIndex *loadGlobalModuleIndex([[maybe_unused]] clang::SourceLocation) override;
			bool lookupMissingImports([[maybe_unused]] clang::StringRef, [[maybe_unused]] clang::SourceLocation) override;
		};

		class diagnostic_client final : public clang::DiagnosticConsumer
		{
			friend class squirrel_preprocessor;

			inline diagnostic_client(squirrel_preprocessor &pp_) noexcept
				:pp{pp_}
			{
			}

			~diagnostic_client() override;

			void HandleDiagnostic(clang::DiagnosticsEngine::Level lvl, const clang::Diagnostic &info) override;

			squirrel_preprocessor &pp;
		};

		class pp_callbacks final : public clang::PPCallbacks
		{
			friend class squirrel_preprocessor;

			inline pp_callbacks(squirrel_preprocessor &pp_) noexcept
				:pp{pp_}
			{
			}

			~pp_callbacks() override;

			void PragmaMark(clang::SourceLocation loc, clang::StringRef str) override;

			void InclusionDirective([[maybe_unused]] clang::SourceLocation, [[maybe_unused]] const clang::Token &, [[maybe_unused]] clang::StringRef, [[maybe_unused]] bool, [[maybe_unused]] clang::CharSourceRange, const clang::FileEntry *file, clang::StringRef search_path, clang::StringRef relative_path, [[maybe_unused]] const clang::Module *, [[maybe_unused]] clang::SrcMgr::CharacteristicKind) override;

			squirrel_preprocessor &pp;
		};

		bool initialize() noexcept;

		clang::LangOptions lang_opts;
		clang::FileSystemOptions fs_opts;
		clang::FileManager file_mgr;
		clang::DiagnosticIDs diagids;
		clang::DiagnosticOptions diagopts;
		diagnostic_client diagcl;
		clang::DiagnosticsEngine diageng;
		clang::SourceManager src_mgr;
		clang::HeaderSearch hdr_srch;
		module_loader modul_ldr;
		clang::Preprocessor pp;

		bool fatal;

		ConVar vmod_preproc_dump;

		std::vector<std::filesystem::path> *curr_incs;
		const std::filesystem::path *curr_path;
	};
}
