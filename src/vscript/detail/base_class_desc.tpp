namespace vmod::vscript::detail
{
	template <typename T>
	base_class_desc<T>::base_class_desc(std::string_view path, bool obfuscate, bool singleton) noexcept
	{
		using namespace std::literals::string_view_literals;

		m_pNextDesc = reinterpret_cast<ScriptClassDesc_t *>(uninitialized_memory);
		m_pszClassname = demangle<T>().c_str();
		m_pszDescription = "@";

		if(singleton) {
			extra_.singleton = true;
		}

		std::size_t name_start{0};
		while(true) {
			std::size_t name_end{path.find("::"sv, name_start)};
			bool done{name_end == std::string_view::npos};
			if(done) {
				name_end = path.length();
			}

			std::string_view name{path.substr(name_start, name_end-name_start)};

			if(!done) {
				extra_.space_ += name;
				extra_.space_ += '_';
			} else {
				extra_.name_ = name;
			}

			name_start = name_end;

			if(done) {
				break;
			}

			name_start += 2;
		}

		if(!extra_.space_.empty()) {
			extra_.space_.pop_back();
		}

		extra_.obfuscated_name_ = "__vmod_"sv;
		if(!extra_.space_.empty()) {
			extra_.obfuscated_name_ += extra_.space_;
			extra_.obfuscated_name_ += '_';
		}
		extra_.obfuscated_name_ += extra_.name_;
		if(singleton) {
			extra_.obfuscated_name_ += "_singleton"sv;
		}

		extra_.obfuscated_class_name_ = extra_.obfuscated_name_;
		extra_.obfuscated_class_name_ += "_class"sv;

		if(!obfuscate) {
			m_pszScriptName = extra_.name_.data();
		} else {
			m_pszScriptName = extra_.obfuscated_class_name_.c_str();
		}
	}
}
