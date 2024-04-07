#include "docs.hpp"
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include "../filesystem.hpp"
#include "../vscript/variant.hpp"
#include "../vscript/detail/base_class_desc.hpp"
#include "../vscript/function_desc.hpp"
#include "../hacking.hpp"

namespace vmod::bindings::docs
{
	namespace detail
	{
		static char time_str_buffer[256];

		static std::string datatype_str_buffer;

		static std::string_view datatype_to_raw_str(gsdk::ScriptDataType_t type) noexcept
		{
			using namespace std::literals::string_view_literals;

			switch(type) {
				case gsdk::FIELD_VOID:
				return "FIELD_VOID"sv;
				case gsdk::FIELD_FLOAT:
				return "FIELD_FLOAT"sv;
				case gsdk::FIELD_STRING:
				return "FIELD_STRING"sv;
				case gsdk::FIELD_VECTOR:
				return "FIELD_VECTOR"sv;
				case gsdk::FIELD_QUATERNION:
				return "FIELD_QUATERNION"sv;
				case gsdk::FIELD_INTEGER:
				return "FIELD_INTEGER"sv;
				case gsdk::FIELD_BOOLEAN:
				return "FIELD_BOOLEAN"sv;
				case gsdk::FIELD_SHORT:
				return "FIELD_SHORT"sv;
				case gsdk::FIELD_CHARACTER:
				return "FIELD_CHARACTER"sv;
				case gsdk::FIELD_COLOR32:
				return "FIELD_COLOR32"sv;
				case gsdk::FIELD_EMBEDDED:
				return "FIELD_EMBEDDED"sv;
				case gsdk::FIELD_CUSTOM:
				return "FIELD_CUSTOM"sv;
				case gsdk::FIELD_CLASSPTR:
				return "FIELD_CLASSPTR"sv;
				case gsdk::FIELD_EHANDLE:
				return "FIELD_EHANDLE"sv;
				case gsdk::FIELD_EDICT:
				return "FIELD_EDICT"sv;
				case gsdk::FIELD_POSITION_VECTOR:
				return "FIELD_POSITION_VECTOR"sv;
				case gsdk::FIELD_TIME:
				return "FIELD_TIME"sv;
				case gsdk::FIELD_TICK:
				return "FIELD_TICK"sv;
				case gsdk::FIELD_MODELNAME:
				return "FIELD_MODELNAME"sv;
				case gsdk::FIELD_SOUNDNAME:
				return "FIELD_SOUNDNAME"sv;
				case gsdk::FIELD_INPUT:
				return "FIELD_INPUT"sv;
				case gsdk::FIELD_FUNCTION:
				return "FIELD_FUNCTION"sv;
				case gsdk::FIELD_VMATRIX:
				return "FIELD_VMATRIX"sv;
				case gsdk::FIELD_VMATRIX_WORLDSPACE:
				return "FIELD_VMATRIX_WORLDSPACE"sv;
				case gsdk::FIELD_MATRIX3X4_WORLDSPACE:
				return "FIELD_MATRIX3X4_WORLDSPACE"sv;
				case gsdk::FIELD_INTERVAL:
				return "FIELD_INTERVAL"sv;
				case gsdk::FIELD_MODELINDEX:
				return "FIELD_MODELINDEX"sv;
				case gsdk::FIELD_MATERIALINDEX:
				return "FIELD_MATERIALINDEX"sv;
				case gsdk::FIELD_VECTOR2D:
				return "FIELD_VECTOR2D"sv;
				//case gsdk::FIELD_TYPECOUNT:
				//return "FIELD_TYPECOUNT"sv;
				case gsdk::FIELD_TYPEUNKNOWN:
				return "FIELD_TYPEUNKNOWN"sv;
				case gsdk::FIELD_CSTRING:
				return "FIELD_CSTRING"sv;
				case gsdk::FIELD_HSCRIPT:
				return "FIELD_HSCRIPT"sv;
				case gsdk::FIELD_VARIANT:
				return "FIELD_VARIANT"sv;
				case gsdk::FIELD_UINT64:
				return "FIELD_UINT64"sv;
				case gsdk::FIELD_FLOAT64:
				return "FIELD_FLOAT64"sv;
				case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
				return "FIELD_POSITIVEINTEGER_OR_NULL"sv;
				case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
				return "FIELD_HSCRIPT_NEW_INSTANCE"sv;
				case gsdk::FIELD_UINT32:
				return "FIELD_UINT32"sv;
				case gsdk::FIELD_UTLSTRINGTOKEN:
				return "FIELD_UTLSTRINGTOKEN"sv;
				case gsdk::FIELD_QANGLE:
				return "FIELD_QANGLE"sv;
			#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				case gsdk::FIELD_INTEGER64:
				return "FIELD_INTEGER64"sv;
			#endif
			#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
				case gsdk::FIELD_VECTOR4D:
				return "FIELD_VECTOR4D"sv;
			#endif
				default: {
					datatype_str_buffer = "<<unknown: "sv;

					std::size_t len{datatype_str_buffer.length()};

					datatype_str_buffer.reserve(len + 6);

					char *begin{datatype_str_buffer.data() + len};
					char *end{begin + 6};

					std::to_chars_result tc_res{std::to_chars(begin, end, type)};
					tc_res.ptr[0] = '\0';

					return datatype_str_buffer.data();
				}
			}
		}

		struct func_info
		{
			std::string alias;

			std::string_view ret_type;

			struct param_info
			{
				std::string_view type;
				std::string_view name;
			};

			std::unordered_map<std::size_t, param_info> params;

			void clear_params() noexcept
			{
				ret_type = {};
				params.clear();
			}
		};

		static std::string_view get_func_desc_desc(const gsdk::ScriptFuncDescriptor_t *desc, func_info &info) noexcept
		{
			const char *description{desc->m_pszDescription};
			if(!description) {
				return {};
			}

			if(description[0] == '@') {
				++description;
			} else if(description[0] == '#') {
				++description;
				const char *begin{description};
				while(description[0] != ':') {
					++description;
				}
				const char *end{description};
				info.alias.assign(begin, end);
				++description;
			}

			const char *base_description{description};

			auto is_valid_start_id{
				[](char c) noexcept -> bool {
					return (
						std::isalpha(c) ||
						c == '_' ||
						c == ':'
					);
				}
			};

			auto is_valid_base_id{
				[is_valid_start_id](char c) noexcept -> bool {
					return (
						is_valid_start_id(c) ||
						std::isdigit(c) ||
						(c == '<' || c == '>')
					);
				}
			};

			auto parse_args{
				[&description,&info,base_description,is_valid_start_id,is_valid_base_id]() noexcept -> bool {
					if(description[0] == '(') {
						++description;
						while(description[0] == ' ') { ++description; }

						std::size_t i{0};

						while(true) {
							const char *name_begin{description};

							if(!is_valid_start_id(description[0])) {
								description = base_description;
								info.clear_params();
								return false;
							} else {
								++description;
							}

							while(
								is_valid_base_id(description[0]) ||
								description[0] == '|' ||
								(description[0] == '[' || description[0] == ']')
							) {
								++description;
							}

							const char *name_end{description};

							while(description[0] == ' ') { ++description; }

							std::string_view name{name_begin, name_end};
							std::string_view type{};

							std::size_t type_idx{name.find('|')};
							if(type_idx != std::string_view::npos) {
								if(type_idx == name.length()) {
									name = {};
								} else {
									name = {name_begin+type_idx+1, name_end};
								}
								type = {name_begin, name_begin+type_idx};
							}

							info.params.emplace(i++, func_info::param_info{type,name});

							if(description[0] == ',') {
								++description;
								while(description[0] == ' ') { ++description; }
								continue;
							} else if(description[0] == ')') {
								++description;
								return true;
							} else {
								description = base_description;
								info.clear_params();
								return false;
							}
						}
					} else {
						return false;
					}
				}
			};

			auto parse_ret{
				[&description,&info,base_description,is_valid_start_id,is_valid_base_id]() noexcept -> bool {
					if(description[0] == '[') {
						++description;
						while(description[0] == ' ') { ++description; }

						const char *name_begin{description};

						if(!is_valid_start_id(description[0])) {
							description = base_description;
							info.clear_params();
							return false;
						} else {
							++description;
						}

						while(is_valid_base_id(description[0])) {
							++description;
						}

						const char *name_end{description};

						while(description[0] == ' ') { ++description; }

						info.ret_type = std::string_view{name_begin, name_end};

						if(description[0] == ']') {
							++description;
							return true;
						} else {
							description = base_description;
							info.clear_params();
							return false;
						}
					} else {
						return false;
					}
				}
			};

			bool started_with_params{false};
			if(std::strncmp(description, "Arguments: ", 11) == 0) {
				started_with_params = true;
				description += 11;
			} else if(std::strncmp(description, "Params: ", 8) == 0) {
				started_with_params = true;
				description += 8;
			}
			if(started_with_params) {
				if(parse_args()) {
					if(std::strncmp(description, " - ", 3) == 0) {
						description += 3;
					} else if(std::strncmp(description, " : ", 3) == 0) {
						description += 3;
					} else if(description[0] != '\0') {
						info.clear_params();
						return base_description;
					}
				} else {
					info.clear_params();
					return base_description;
				}
			} else {
				bool started_with_binding{false};
				if(std::strncmp(description, "Binding_", 8) == 0) {
					description += 8;
					started_with_binding = true;
				}
				std::size_t name_len{std::strlen(desc->m_pszScriptName)};
				if(std::strncmp(description, desc->m_pszScriptName, name_len) == 0) {
					description += name_len;
					if(parse_args()) {
						if(std::strncmp(description, " : ", 3) == 0) {
							description += 3;
						} else if(std::strncmp(description, ": ", 2) == 0) {
							description += 2;
						} else {
							info.clear_params();
							return base_description;
						}
					} else {
						info.clear_params();
						return base_description;
					}
				} else {
					if(started_with_binding) {
						info.clear_params();
						return base_description;
					} else {
						if(parse_ret()) {
							if(parse_args()) {
								if(description[0] != '\0') {
									info.clear_params();
									return base_description;
								}
							} else if(description[0] != '\0') {
								info.clear_params();
								return base_description;
							}
						} else if(parse_args()) {
							if(std::strncmp(description, " - ", 3) == 0) {
								description += 3;
							} else if(description[0] == ' ') {
								++description;
							} else if(description[0] != '\0') {
								info.clear_params();
								return base_description;
							}
						} else {
							info.clear_params();
							return base_description;
						}
					}
				}
			}

			if(description[0] == '\0') {
				return {};
			}

			return description;
		}

		static std::string_view get_class_desc_desc(const gsdk::ScriptClassDesc_t *desc) noexcept
		{
			const char *description{desc->m_pszDescription};

			if(description[0] == '!') {
				++description;
			}

			return description;
		}

		static std::string_view type_name(gsdk::ScriptDataType_t type, std::size_t size, bool vscript, int flags) noexcept
		{
			using namespace std::literals::string_view_literals;

			switch(type) {
				case gsdk::FIELD_VOID:
				return "void"sv;
				case gsdk::FIELD_CHARACTER:
				return "char"sv;
				case gsdk::FIELD_SHORT:
				return "short"sv;
				case gsdk::FIELD_UINT32:
				return "unsigned int"sv;
			#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				case gsdk::FIELD_INTEGER64:
				return "long long"sv;
			#endif
				case gsdk::FIELD_UINT64:
				return "unsigned long long"sv;
				case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
				return "int"sv;
				case gsdk::FIELD_INTEGER: {
					switch(size) {
						case sizeof(char):
						return "char"sv;
						case sizeof(short):
						return "short"sv;
						case sizeof(int):
						return "int"sv;
						case sizeof(long long):
						return "long long"sv;
						case static_cast<std::size_t>(-1):
						return "int"sv;
						default:
						return "<<unknown>>"sv;
					}
				}
				case gsdk::FIELD_MODELINDEX:
				return "modelindex"sv;
				case gsdk::FIELD_MATERIALINDEX:
				return "materialindex"sv;
				case gsdk::FIELD_TICK:
				return "int"sv;
				case gsdk::FIELD_FLOAT64:
				return "double"sv;
				case gsdk::FIELD_FLOAT: {
					switch(size) {
						case sizeof(float):
						return "float"sv;
						case sizeof(double):
						return "double"sv;
						case sizeof(long double):
						return "long double"sv;
						case static_cast<std::size_t>(-1):
						return "float"sv;
						default:
						return "<<unknown>>"sv;
					}
				}
				case gsdk::FIELD_INTERVAL:
				return "interval_t"sv;
				case gsdk::FIELD_TIME:
				return "float"sv;
				case gsdk::FIELD_BOOLEAN:
				return "bool"sv;
				case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
				case gsdk::FIELD_HSCRIPT:
				return "handle"sv;
				case gsdk::FIELD_POSITION_VECTOR:
				case gsdk::FIELD_VECTOR:
				return "Vector"sv;
				case gsdk::FIELD_VECTOR2D:
				return "Vector2D"sv;
			#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
				case gsdk::FIELD_VECTOR4D:
				return "Vector4D"sv;
			#endif
				case gsdk::FIELD_QANGLE:
				return "QAngle"sv;
				case gsdk::FIELD_QUATERNION:
				return "Quaternion"sv;
				case gsdk::FIELD_STRING:
				return "string_t"sv;
				case gsdk::FIELD_CSTRING: {
					if(vscript) {
						return "string"sv;
					} else {
						return "char[]"sv;
					}
				}
				case gsdk::FIELD_MODELNAME:
				return "modelpath"sv;
				case gsdk::FIELD_SOUNDNAME:
				return "soundpath"sv;
				case gsdk::FIELD_VARIANT:
				return "variant"sv;
				case gsdk::FIELD_EHANDLE:
				return "EHANDLE"sv;
				case gsdk::FIELD_EDICT:
				return "edict"sv;
				case gsdk::FIELD_FUNCTION: {
					if(vscript) {
						return "function *"sv;
					} else if(flags & gsdk::FTYPEDESC_OUTPUT) {
						return "output"sv;
					} else if(flags & gsdk::FTYPEDESC_FUNCTIONTABLE) {
						return "function *"sv;
					} else {
						return "input"sv;
					}
				}
				case gsdk::FIELD_CLASSPTR:
				return "object *"sv;
				case gsdk::FIELD_COLOR32:
				return "color32"sv;
				case gsdk::FIELD_EMBEDDED:
				return "struct"sv;
				case gsdk::FIELD_CUSTOM:
				return "<<unknown>>"sv;
				case gsdk::FIELD_TYPEUNKNOWN:
				return "<<unknown>>"sv;
				default:
				return detail::datatype_to_raw_str(type);
			}
		}
	}

	std::string_view type_name(gsdk::ScriptDataType_t type, std::size_t size, int flags) noexcept
	{
		return detail::type_name(type, size, false, flags);
	}

	std::string_view type_name(gsdk::ScriptDataType_t type, bool vscript) noexcept
	{
		return detail::type_name(type, static_cast<std::size_t>(-1), vscript, gsdk::FTYPEDESC_NOFLAGS);
	}

#ifdef __VMOD_USING_CUSTOM_VM
	std::string param_type_name(gsdk::ScriptDataTypeAndFlags_t type, std::string_view alt) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string ret;

		if(type.can_be_optional()) {
			ret += "optional<"sv;
		}
		if(!alt.empty()) {
			ret += alt;
		} else {
			if(type.has_extra_types()) {
				ret += "variant<"sv;
			}
			ret += type_name(type.main_type(), true);
			if(type.has_extra_types()) {
				ret += ", "sv;
				for(int i{0}; i <= static_cast<int>(gsdk::SlimScriptDataType_t::SLIMFIELD_TYPECOUNT); ++i) {
					auto fat{gsdk::fat_datatype(static_cast<gsdk::SlimScriptDataType_t>(i))};
					if(type.has_extra_type(static_cast<gsdk::SlimScriptDataType_t>(i))) {
						ret += type_name(fat, true);
						ret += ", "sv;
					}
				}
				ret.erase(ret.end()-2, ret.end());
				ret += '>';
			}
		}
		if(type.can_be_optional()) {
			ret += '>';
		}

		return ret;
	}
#else
	std::string_view param_type_name(gsdk::ScriptDataType_t type, std::string_view alt) noexcept
	{
		if(!alt.empty()) {
			return alt;
		} else {
			return type_name(type, true);
		}
	}
#endif

	std::string_view get_class_desc_name(const gsdk::ScriptClassDesc_t *desc) noexcept
	{
		if(desc->m_pNextDesc == reinterpret_cast<const gsdk::ScriptClassDesc_t *>(uninitialized_memory)) {
			const vscript::extra_class_desc &extra{static_cast<const vscript::detail::base_class_desc<generic_class> *>(desc)->extra()};
			return extra.name();
		}

		return desc->m_pszScriptName;
	}

	void gen_date(std::string &file) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::time_t time{std::time(nullptr)};
		std::strftime(detail::time_str_buffer, sizeof(detail::time_str_buffer), "//date: %Y-%m-%d %H:%M:%S UTC\n", std::gmtime(&time));
		file += detail::time_str_buffer;

		file += "//engine: "sv;
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		file += "tf2"sv;
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		file += "l4d2"sv;
	#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2
		file += "portal2"sv;
	#else
		#error
	#endif
		file += "\n\n"sv;
	}

	bool write(const gsdk::ScriptFunctionBinding_t *func, bool global, std::size_t depth, std::string &file, bool respect_hide) noexcept
	{
		using namespace std::literals::string_view_literals;

		const gsdk::ScriptFuncDescriptor_t &func_desc{func->m_desc};

		if(respect_hide) {
			if(func_desc.m_pszDescription && func_desc.m_pszDescription[0] == '@') {
				return false;
			}
		}

		detail::func_info info;
		std::string_view description{detail::get_func_desc_desc(&func_desc, info)};
		if(!description.empty()) {
			ident(file, depth);
			file += "//"sv;
			file += description;
			file += '\n';
		}

		std::string ret_str;
		ident(ret_str, depth);
		if(global) {
			if(!(func->m_flags & gsdk::SF_MEMBER_FUNC)) {
				ret_str += "static "sv;
			}
		}
		if(!info.ret_type.empty()) {
			ret_str += info.ret_type;
		} else {
			ret_str += type_name(func_desc.m_ReturnType, true);
		}
		ret_str += ' ';

		std::string params_str;
		params_str += '(';
		//TODO!!!! skip va args
		std::size_t num_args{func_desc.m_Parameters.size()};
		if(num_args > 0) {
			auto do_param{
				[&info,&params_str,&func_desc](std::size_t j) noexcept -> void {
					auto param_it{info.params.find(j)};
					if(param_it != info.params.end()) {
						params_str += param_type_name(func_desc.m_Parameters[j], param_it->second.type);
						if(!param_it->second.name.empty()) {
							params_str += ' ';
							params_str += param_it->second.name;
						}
					} else {
						params_str += param_type_name(func_desc.m_Parameters[j]);
					}
				}
			};

			if(num_args > 1) {
				std::size_t optionals_start{num_args};
				for(ssize_t j{static_cast<ssize_t>(num_args-1)}; j >= 0; --j) {
					if(!func_desc.m_Parameters[static_cast<std::size_t>(j)].can_be_optional()) {
						optionals_start = static_cast<std::size_t>(j);
						break;
					}
				}

				if(optionals_start == num_args) {
					params_str += '[';
					for(std::size_t j{0}; j < num_args-1; ++j) {
						do_param(j);
						params_str += ", "sv;
					}
					do_param(num_args-1);
					params_str += ']';
				} else if(optionals_start == (num_args-1)) {
					for(std::size_t j{0}; j < num_args-1; ++j) {
						do_param(j);
						params_str += ", "sv;
					}
					do_param(num_args-1);
				} else {
					if(optionals_start > 0) {
						for(std::size_t j{0}; j < optionals_start-1; ++j) {
							do_param(j);
							params_str += ", "sv;
						}

						do_param(optionals_start-1);
					} else {
						do_param(0);
					}
					params_str += "[, "sv;

					for(std::size_t j{optionals_start+1}; j < num_args-1; ++j) {
						do_param(j);
						params_str += ", "sv;
					}

					do_param(num_args-1);
					params_str += ']';
				}
			} else {
				bool opt{func_desc.m_Parameters[0].can_be_optional()};
				if(opt) {
					params_str += '[';
				}
				do_param(0);
				if(opt) {
					params_str += ']';
				}
			}
		}
		if(func->m_flags & gsdk::SF_VA_FUNC) {
			if(num_args > 0) {
				params_str += ", "sv;
			}
			params_str += "..."sv;
		}
		params_str += ");\n"sv;

		file += ret_str;
		file += func_desc.m_pszScriptName;
		file += params_str;

		if(!info.alias.empty()) {
			file += ret_str;
			file += info.alias;
			file += params_str;
		}

		file += '\n';

		return true;
	}

	bool write(const gsdk::ScriptClassDesc_t *desc, bool global, std::size_t depth, std::string &file, bool respect_hide) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(respect_hide) {
			if(desc->m_pszDescription && desc->m_pszDescription[0] == '@') {
				return false;
			}
		}

		std::size_t written{0};

		if(global) {
			if(desc->m_pszDescription && desc->m_pszDescription[0] != '\0' && desc->m_pszDescription[0] != '@') {
				ident(file, depth);
				file += "//"sv;
				file += detail::get_class_desc_desc(desc);
				file += '\n';
			}

			ident(file, depth);
			file += "class "sv;
			file += get_class_desc_name(desc);

			if(desc->m_pBaseDesc != reinterpret_cast<gsdk::ScriptClassDesc_t *>(uninitialized_memory)) {
				if(desc->m_pBaseDesc) {
					file += " : "sv;
					file += get_class_desc_name(desc->m_pBaseDesc);
				} else {
					file += " : "sv;
					file += "instance"sv;
				}
			}

			file += '\n';
			ident(file, depth);
			file += "{\n"sv;

			if(desc->m_pfnConstruct) {
				ident(file, depth+1);
				file += get_class_desc_name(desc);
				file += "();\n\n"sv;
				++written;
			}

			if(desc->m_pfnDestruct) {
				ident(file, depth+1);
				file += '~';
				file += get_class_desc_name(desc);
				file += "();\n\n"sv;
				++written;
			}
		}

		for(std::size_t i{0}; i < desc->m_FunctionBindings.size(); ++i) {
			if(write(&desc->m_FunctionBindings[i], global, global ? depth+1 : depth, file, respect_hide)) {
				++written;
			}
		}
		if(written > 0) {
			file.erase(file.end()-1, file.end());
		}

		if(global) {
			ident(file, depth);
			file += "};"sv;
		}

		return true;
	}

	void write(const std::filesystem::path &dir, const std::vector<const gsdk::ScriptClassDesc_t *> &vec, bool respect_hide) noexcept
	{
		using namespace std::literals::string_view_literals;

		for(const gsdk::ScriptClassDesc_t *it : vec) {
			std::string file;

			gen_date(file);

			if(!write(it, true, 0, file, respect_hide)) {
				continue;
			}

			std::filesystem::path doc_path{dir};
			doc_path /= get_class_desc_name(it);
			doc_path.replace_extension(".txt"sv);

			write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
		}
	}

	void write(const std::filesystem::path &dir, const std::vector<const gsdk::ScriptFunctionBinding_t *> &vec, bool respect_hide) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		gen_date(file);

		std::size_t written{0};
		for(const gsdk::ScriptFunctionBinding_t *it : vec) {
			if(write(it, false, 0, file, respect_hide)) {
				++written;
			}
		}
		if(written > 0) {
			file.erase(file.end()-1, file.end());
		}

		std::filesystem::path doc_path{dir};
		doc_path /= "global_funcs"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	value &value::operator=(value &&other) noexcept
	{
		type = other.type;

		switch(type) {
			case type::variant: {
				var = std::move(other.var);
			} break;
			case type::desc: {
				desc = other.desc;
			} break;
		#ifndef __clang__
			default: break;
		#endif
		}

		return *this;
	}

	void write(const std::filesystem::path &dir, const std::unordered_map<std::string, value> &map) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		gen_date(file);

		for(const auto &it : map) {
			switch(it.second.type) {
				case value::type::variant: {
					const vscript::variant &var{it.second.var};
					file += type_name(var.m_type, true);
				} break;
				case value::type::desc: {
					const gsdk::ScriptClassDesc_t *desc{it.second.desc};
					file += get_class_desc_name(desc);
				} break;
			#ifndef __clang__
				default: break;
			#endif
			}

			file += ' ';
			file += it.first;
			file += ';';

			if(it.second.type == value::type::variant) {
				const vscript::variant &var{it.second.var};
				file += " //"sv;
				file += var.get<std::string_view>();
			}

			file += "\n\n"sv;
		}
		if(!map.empty()) {
			file.erase(file.end()-2, file.end());
		}

		std::filesystem::path doc_path{dir};
		doc_path /= "global_values"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void write(std::string &file, std::size_t depth, vscript::handle_ref enum_table, write_enum_how how) noexcept
	{
		//TODO!!!! sort by value

		using namespace std::literals::string_view_literals;

		std::unordered_map<unsigned char, std::string> bit_str_map;

		gsdk::IScriptVM *vm{vscript::vm()};

		int num2{vm->GetNumTableEntries(*enum_table)};

		if(how == write_enum_how::flags) {
			for(int j{0}, it2{0}; it2 != -1 && j < num2; ++j) {
				vscript::variant key2;
				vscript::variant value2;
				it2 = vm->GetKeyValue(*enum_table, it2, &key2, &value2);

				if(value2.m_type == gsdk::FIELD_VOID) {
					continue;
				}

				std::string_view value_name{key2.get<std::string_view>()};

				unsigned int val{value2.get<unsigned int>()};

				unsigned char found_bits{0};
				unsigned char last_bit{0};

				for(int k{0}; val; val >>= 1, ++k) {
					if(val & 1) {
						++found_bits;
						last_bit = static_cast<unsigned char>(k);
						if(found_bits > 1) {
							break;
						}
					}
				}

				if(found_bits <= 1) {
					bit_str_map.emplace(last_bit, value_name);
				}
			}
		}

		for(int j{0}, it2{0}; it2 != -1 && j < num2; ++j) {
			vscript::variant key2;
			vscript::variant value2;
			it2 = vm->GetKeyValue(*enum_table, it2, &key2, &value2);

			std::string_view value_name{key2.get<std::string_view>()};

			ident(file, depth);
			file += value_name;
			if(how != write_enum_how::name) {
				file += " = "sv;
			}

			constexpr bool respect_null{true};

			std::string_view value_str;
			if(value2.m_type == gsdk::FIELD_VOID) {
				if constexpr(respect_null) {
					value_str = "null"sv;
				} else {
					value_str = "0"sv;
				}
			} else {
				value_str = value2.get<std::string_view>();
			}

			if(value2.m_type == gsdk::FIELD_VOID) {
				if(how != write_enum_how::name) {
					file += value_str;
				}
			} else {
				if(how == write_enum_how::flags) {
					unsigned int val{value2.get<unsigned int>()};

					std::vector<unsigned char> bits;

					for(unsigned char k{0}; val; val >>= 1, ++k) {
						if(val & 1) {
							bits.emplace_back(k);
						}
					}

					std::size_t num_bits{bits.size()};

					if(num_bits > 0) {
						std::string temp_bit_str;

						if(num_bits > 1) {
							file += '(';
						}
						for(unsigned char bit : bits) {
							auto name_it{bit_str_map.find(bit)};
							if(name_it != bit_str_map.end() && name_it->second != value_name) {
								file += name_it->second;
								file += '|';
							} else {
								file += "(1 << "sv;

								temp_bit_str.resize(6);

								char *begin{temp_bit_str.data()};
								char *end{begin + 6};

								std::to_chars_result tc_res{std::to_chars(begin, end, bit)};
								tc_res.ptr[0] = '\0';

								file += begin;
								file += ")|"sv;
							}
						}
						file.pop_back();
						if(num_bits > 1) {
							file += ')';
						}
					} else {
						file += value_str;
					}
				} else if(how == write_enum_how::normal) {
					file += value_str;
				}
			}

			if(j < num2-1) {
				file += ',';
			}

			if(value2.m_type != gsdk::FIELD_VOID) {
				if(how == write_enum_how::flags) {
					file += " //"sv;
					file += value_str;
				}
			} else if constexpr(!respect_null) {
				if(how != write_enum_how::name) {
					file += " //null"sv;
				}
			}

			file += '\n';
		}
	}
}
