#include "preprocessor.hpp"
#include "gsdk.hpp"
#include "vmod.hpp"
#include "filesystem.hpp"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wcomment"
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wint-in-bool-context"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcomment"
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#endif
#include <tpp.h>
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

namespace vmod
{
	squirrel_preprocessor *squirrel_preprocessor::current;

	squirrel_preprocessor::squirrel_preprocessor() noexcept
	{
		current = this;
	}

	static char __sqpp_temp_path_buff[PATH_MAX];
	static char __sqpp_temp_msg_buff[4026];
	bool squirrel_preprocessor::initialize() noexcept
	{
		using namespace std::literals::string_view_literals;

		vmod_preproc_dump.initialize("vmod_preproc_dump"sv, false);

		if(!TPP_INITIALIZE()) {
			return false;
		}

		TPPLexer_Current->l_flags = TPPLEXER_FLAG_WANTSPACE|TPPLEXER_FLAG_WANTLF|TPPLEXER_FLAG_MESSAGE_LOCATION;
		TPPLexer_Current->l_callbacks.c_new_textfile =
			[](TPPFile *file, [[maybe_unused]] int) noexcept -> int {
				if(current->curr_incs) {
					current->curr_incs->emplace_back(std::string{file->f_name, file->f_namesize});
				}
				return 1;
			};
		TPPLexer_Current->l_callbacks.c_unknown_file = nullptr;
		TPPLexer_Current->l_callbacks.c_warn =
			[](const char *fmt, va_list args) noexcept -> void {
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wformat-nonliteral"
				std::vsnprintf(__sqpp_temp_msg_buff, sizeof(__sqpp_temp_msg_buff), fmt, args);
				#pragma GCC diagnostic pop

				constexpr std::size_t warn_begin_len{__builtin_strlen(TPP_WARNF_WARN_BEGIN)};
				constexpr std::size_t err_begin_len{__builtin_strlen(TPP_WARNF_WARN_BEGIN)};

				if(std::strncmp(__sqpp_temp_msg_buff, TPP_WARNF_WARN_BEGIN, warn_begin_len) == 0) {
					current->print_state = print_state::warning;
				} else if(std::strncmp(__sqpp_temp_msg_buff, TPP_WARNF_ERROR_BEGIN, err_begin_len) == 0) {
					current->print_state = print_state::error;
				}

				switch(current->print_state) {
					case print_state::unknown:
					info("%s", __sqpp_temp_msg_buff);
					break;
					case print_state::warning:
					warning("%s", __sqpp_temp_msg_buff);
					break;
					case print_state::error:
					error("%s", __sqpp_temp_msg_buff);
					break;
				}
			};
		TPPLexer_Current->l_callbacks.c_message =
			[](const char *fmt, va_list args) noexcept -> void {
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wformat-nonliteral"
				std::vsnprintf(__sqpp_temp_msg_buff, sizeof(__sqpp_temp_msg_buff), fmt, args);
				#pragma GCC diagnostic pop
				info("%s", __sqpp_temp_msg_buff);
			};

		std::error_code err;

		std::filesystem::path include_dir{vmod.root_dir()};
		include_dir /= "include"sv;
		std::strncpy(__sqpp_temp_path_buff, include_dir.c_str(), include_dir.native().length());
		TPPLexer_AddIncludePath(__sqpp_temp_path_buff, include_dir.native().length());

		include_dir = vmod.game_dir();
		include_dir /= "scripts"sv;
		include_dir /= "vscripts"sv;
		std::strncpy(__sqpp_temp_path_buff, include_dir.c_str(), include_dir.native().length());
		TPPLexer_AddIncludePath(__sqpp_temp_path_buff, include_dir.native().length());

		return true;
	}

	bool squirrel_preprocessor::preprocess(std::string &str, const std::filesystem::path &path, std::vector<std::filesystem::path> &incs) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::strncpy(__sqpp_temp_path_buff, path.c_str(), path.native().length());
		TPPFile *file{TPPLexer_OpenFile(TPPLEXER_OPENFILE_MODE_NORMAL, __sqpp_temp_path_buff, path.native().length(), nullptr)};
		if(!file) {
			error("failed to open file '%s'\n", path.c_str());
			return false;
		}

		TPPLexer_PushFile(file);

		curr_incs = &incs;
		curr_path = &path;

		str.reserve(file->f_text->s_size);

		struct scope_cleanup {
			inline scope_cleanup(squirrel_preprocessor &pp_, std::string &str_, const std::filesystem::path &path_, TPPFile *file_) noexcept
				: pp{pp_}, str{str_}, path{path_}, file{file_}
			{}
			inline ~scope_cleanup() noexcept {
				if(pp.vmod_preproc_dump.get<bool>()) {
					std::filesystem::path pp_path{vmod.root_dir()};
					pp_path /= "plugins_pp"sv;
					pp_path /= path.filename();
					pp_path.replace_extension(".nut.pp"sv);

					write_file(pp_path, reinterpret_cast<const unsigned char *>(str.c_str()), str.length());
				}

				TPPLexer_PopFile();

				TPPLexer_Reset(TPPLexer_Current, TPPLEXER_RESET_INCLUDE|TPPLEXER_RESET_EXTENSIONS|TPPLEXER_RESET_WARNINGS|TPPLEXER_RESET_KEYWORDS);

				pp.print_state = print_state::unknown;

				pp.curr_incs = nullptr;
				pp.curr_path = nullptr;
			}
			squirrel_preprocessor &pp;
			std::string &str;
			const std::filesystem::path &path;
			TPPFile *file;
		};
		scope_cleanup sclp{*this, str, path, file};

		while(TPPLexer_Yield() > 0) {
			const char *tokstr{TPPLexer_Current->l_token.t_begin};
			std::size_t toklen{static_cast<std::size_t>(TPPLexer_Current->l_token.t_end - tokstr)};
			str.append(tokstr, toklen);
		}

		if((TPPLexer_Current->l_flags & TPPLEXER_FLAG_ERROR) ||
			(TPPLexer_Current->l_errorcount != 0)) {
			return false;
		}

		return true;
	}

	squirrel_preprocessor::~squirrel_preprocessor() noexcept
	{
		TPP_FINALIZE();
	}
}
