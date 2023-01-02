#include "preprocessor.hpp"
#include "gsdk.hpp"
#include "main.hpp"
#include "filesystem.hpp"
#include <cstddef>

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
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include <tpp.h>
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

namespace vmod
{
	squirrel_preprocessor *squirrel_preprocessor::current{nullptr};

	char squirrel_preprocessor::msg_buff[gsdk::MAXPRINTMSG];

	squirrel_preprocessor::squirrel_preprocessor() noexcept
	{
		current = this;
	}

	bool squirrel_preprocessor::initialize() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(!TPP_INITIALIZE()) {
			error("vmod: tpp failed to initialize\n"sv);
			return false;
		}

		initialized = true;

		vmod_preproc_dump.initialize("vmod_preproc_dump"sv, false);

		const std::filesystem::path &plugins_dir{main::instance().plugins_dir()};

		pp_dir = plugins_dir.parent_path();

		{
			std::string dir_name{plugins_dir.filename().native()};
			dir_name += "_pp"sv;
			pp_dir /= std::move(dir_name);
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
			[](const char *fmt, va_list args) __attribute__((__format__(__printf__, 2, 0))) noexcept -> void
			{
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wformat-nonliteral"
				std::vsnprintf(msg_buff, sizeof(msg_buff), fmt, args);
				#pragma GCC diagnostic pop

				constexpr std::size_t warn_begin_len{__builtin_strlen(TPP_WARNF_WARN_BEGIN)};
				constexpr std::size_t err_begin_len{__builtin_strlen(TPP_WARNF_WARN_BEGIN)};

				if(std::strncmp(msg_buff, TPP_WARNF_WARN_BEGIN, warn_begin_len) == 0) {
					current->print_state = print_state::warning;
				} else if(std::strncmp(msg_buff, TPP_WARNF_ERROR_BEGIN, err_begin_len) == 0) {
					current->print_state = print_state::error;
				}

				switch(current->print_state) {
					case print_state::warning:
					warning("%s", msg_buff);
					break;
					case print_state::error:
					error("%s", msg_buff);
					break;
					default:
					info("%s", msg_buff);
					break;
				}
			};
		TPPLexer_Current->l_callbacks.c_message =
			[](const char *fmt, va_list args) __attribute__((__format__(__printf__, 2, 0))) noexcept -> void
			{
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wformat-nonliteral"
				std::vsnprintf(msg_buff, sizeof(msg_buff), fmt, args);
				#pragma GCC diagnostic pop
				info("%s", msg_buff);
			};

		std::error_code err;

		std::filesystem::path include_dir{main::instance().root_dir()};
		include_dir /= "include"sv;
		std::strncpy(path_buff, include_dir.c_str(), include_dir.native().length());
		TPPLexer_AddIncludePath(path_buff, include_dir.native().length());

		include_dir = main::instance().game_dir();
		include_dir /= "scripts"sv;
		include_dir /= "vscripts"sv;
		std::strncpy(path_buff, include_dir.c_str(), include_dir.native().length());
		TPPLexer_AddIncludePath(path_buff, include_dir.native().length());

		auto add_define{
			[](std::string_view name, auto &&value) noexcept -> bool {
				using decayed_t = std::decay_t<decltype(value)>;

				const char *value_ptr{nullptr};
				std::size_t value_size{0};

				if constexpr(std::is_same_v<decayed_t, std::nullptr_t>) {
					
				} else if constexpr(std::is_same_v<decayed_t, std::string_view>) {
					value_ptr = value.data();
					value_size = value.size();
				} else if constexpr(std::is_integral_v<decayed_t>) {
					char *begin{msg_buff};
					char *end{begin + (6 + 6)};

					std::to_chars_result tc_res{std::to_chars(begin, end, value)};
					tc_res.ptr[0] = '\0';

					value_ptr = begin;
					value_size = std::strlen(begin);
				} else {
					static_assert(false_t<decayed_t>::value);
				}

				return (TPPLexer_Define(name.data(), name.size(), value_ptr, value_size, TPPLEXER_DEFINE_FLAG_BUILTIN) > 0);
			}
		};

		add_define("GSDK_ENGINE_TF2"sv, GSDK_ENGINE_TF2);
		add_define("GSDK_ENGINE_L4D2"sv, GSDK_ENGINE_L4D2);
		add_define("GSDK_ENGINE_PORTAL2"sv, GSDK_ENGINE_PORTAL2);
		add_define("GSDK_ENGINE"sv, GSDK_ENGINE);

		add_define("GSDK_DLL_HYBRID"sv, GSDK_DLL_HYBRID);
		add_define("GSDK_DLL_SERVER"sv, GSDK_DLL_SERVER);
		add_define("GSDK_DLL_CLIENT"sv, GSDK_DLL_CLIENT);
		add_define("GSDK_DLL"sv, GSDK_DLL);

	#ifdef GSDK_NO_SYMBOLS
		add_define("GSDK_NO_SYMBOLS"sv, nullptr);
	#endif

		return true;
	}

	bool squirrel_preprocessor::preprocess(std::string &str, const std::filesystem::path &path, std::vector<std::filesystem::path> &incs) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::strncpy(path_buff, path.c_str(), path.native().length());
		TPPFile *file{TPPLexer_OpenFile(TPPLEXER_OPENFILE_MODE_NORMAL, path_buff, path.native().length(), nullptr)};
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
					std::filesystem::path pp_path{pp.pp_dir};
					{
						const std::filesystem::path &plugins_dir{main::instance().plugins_dir()};

						std::string path_delta{path.native()};
						path_delta.erase(0, plugins_dir.native().length()+1);
						pp_path /= std::move(path_delta);
					}

					{
						std::string ext{path.extension().native()};
						ext += ".pp"sv;
						pp_path.replace_extension(std::move(ext));
					}

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
		if(initialized) {
			TPP_FINALIZE();
		}
	}
}
