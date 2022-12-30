#include "convar.hpp"
#include <charconv>
#include <cstring>
#include <cstdlib>

#include <cstdio>

namespace gsdk
{
	ConCommandBase::~ConCommandBase() {}
	bool ConCommandBase::IsCommand() const { return true; }

	int ConCommand::AutoCompleteSuggest(const char *, CUtlVector<CUtlString> &) { return 0; }
	bool ConCommand::CanAutoComplete() { return false; }
	bool ConCommand::IsCommand() const { return true; }

	bool ConVar::IsCommand() const { return false; }
	void ConVar::SetValue(const char *value) { ConVar::InternalSetValue(value); }
	void ConVar::SetValue(float value) { ConVar::InternalSetFloatValue(value); }
	void ConVar::SetValue(int value) { ConVar::InternalSetIntValue(value); }
	const char *ConVar::GetName() const { return ConCommandBase::GetName(); }
	bool ConVar::IsFlagSet(int flags) const { return ConCommandBase::IsFlagSet(flags); }

	const char *ConCommandBase::GetName() const
	{
		if(!IsCommand()) {
			const ConVar *var{static_cast<const ConVar *>(this)};
			if(var->m_pParent && var->m_pParent != var) {
				return var->m_pParent->ConCommandBase::GetName();
			}
		}

		return m_pszName;
	}

	const char *ConCommandBase::GetHelpText() const
	{
		if(!IsCommand()) {
			const ConVar *var{static_cast<const ConVar *>(this)};
			if(var->m_pParent && var->m_pParent != var) {
				return var->m_pParent->ConCommandBase::GetHelpText();
			}
		}

		return m_pszHelpString;
	}

	bool ConCommandBase::IsFlagSet(int flags) const
	{
		if(!IsCommand()) {
			const ConVar *var{static_cast<const ConVar *>(this)};
			if(var->m_pParent && var->m_pParent != var) {
				return var->m_pParent->ConCommandBase::IsFlagSet(flags);
			}
		}

		if(flags & FCVAR_UNREGISTERED) {
			return ((m_nFlags & flags) && !m_bRegistered);
		} else {
			return (m_nFlags & flags);
		}
	}

	void ConCommandBase::AddFlags(int flags)
	{
		flags &= ~FCVAR_UNREGISTERED;

		if(!IsCommand()) {
			const ConVar *var{static_cast<const ConVar *>(this)};
			if(var->m_pParent && var->m_pParent != var) {
				var->m_pParent->ConCommandBase::AddFlags(flags);
			}
		}

		m_nFlags |= flags;
	}

	bool ConCommandBase::IsRegistered() const
	{
		if(!IsCommand()) {
			const ConVar *var{static_cast<const ConVar *>(this)};
			if(var->m_pParent && var->m_pParent != var) {
				return var->m_pParent->ConCommandBase::IsRegistered();
			}
		}

		return (m_bRegistered && !(m_nFlags & FCVAR_UNREGISTERED));
	}

	void ConCommandBase::CreateBase(const char *name, const char *help, int flags)
	{
		m_pszName = name;
		m_pszHelpString = help ? help : "";
		m_nFlags = flags|FCVAR_UNREGISTERED;
	}

	void ConCommand::CreateBase(const char *name, const char *help, int flags)
	{
		ConCommandBase::CreateBase(name, help, flags);
	}

	bool ConCommandBase::IsCompetitiveRestricted() const noexcept
	{
		if(!IsCommand()) {
			const ConVar *var{static_cast<const ConVar *>(this)};
			if(var->m_pParent && var->m_pParent != var) {
				return var->ConCommandBase::IsCompetitiveRestricted();
			}

			if(var->m_bHasCompMin || var->m_bHasCompMax) {
				return true;
			}
		}

		if(m_nFlags & (FCVAR_HIDDEN|FCVAR_DEVELOPMENTONLY|FCVAR_INTERNAL_USE|FCVAR_GAMEDLL|FCVAR_REPLICATED|FCVAR_CHEAT)) {
			return false;
		}

		if(m_nFlags & (FCVAR_ARCHIVE|FCVAR_ALLOWED_IN_COMPETITIVE)) {
			return false;
		}

		return true;
	}

	void ConVar::CreateBase(const char *name, const char *help, int flags)
	{
		ConCommandBase::CreateBase(name, help, flags);
		m_pParent = this;
	}

	bool ConVar::ClampValue(float &value)
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::ClampValue(value);
		}

		if(m_bCompetitiveRestrictions && ConVar::IsCompetitiveRestricted()) {
			if(m_bHasCompMin && value < m_fCompMinVal) {
				value = m_fCompMinVal;
				return true;
			}

			if(m_bHasCompMax && value > m_fCompMaxVal) {
				value = m_fCompMaxVal;
				return true;
			}

			if(!m_bHasCompMin && !m_bHasCompMax && m_pszDefaultValue && m_pszDefaultValue[0] != '\0') {
				const char *begin{m_pszDefaultValue};
				const char *end{begin + std::strlen(m_pszDefaultValue)};

				std::from_chars(begin, end, value);
				return true;
			}
		}

		if(m_bHasMin && value < m_fMinVal) {
			value = m_fMinVal;
			return true;
		}

		if(m_bHasMax && value > m_fMaxVal) {
			value = m_fMaxVal;
			return true;
		}

		return false;
	}

	bool ConVar::ClampValue(int &value)
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::ClampValue(value);
		}

		float temp{static_cast<float>(value)};
		bool ret{ClampValue(temp)};
		value = static_cast<int>(temp);
		return ret;
	}

	void ConVar::ClearString()
	{
		if(m_pParent && m_pParent != this) {
			m_pParent->ConVar::ClearString();
		}

		if(m_pszString) {
			m_pszString = static_cast<char *>(std::realloc(m_pszString, 2));
		} else {
			m_pszString = new char[2];
		}

		m_pszString[0] = '\0';
		m_StringLength = 0;
	}

	void ConVar::InternalSetValue(const char *value)
	{
		if(m_pParent && m_pParent != this) {
			m_pParent->ConVar::InternalSetValue(value);
		}

		std::size_t len{std::strlen(value)};

		if(!ConCommandBase::IsFlagSet(FCVAR_NEVER_AS_STRING)) {
			if(m_pszString) {
				m_pszString = static_cast<char *>(std::realloc(m_pszString, len+1));
			} else {
				m_pszString = static_cast<char *>(std::malloc(len+1));
			}

			std::strncpy(m_pszString, value, len);
			m_pszString[len] = '\0';
			m_StringLength = static_cast<int>(len);
		} else {
			ClearString();
		}

		if(std::strcmp(value, "true") == 0) {
			m_fValue = 1.0f;
			ConVar::ClampValue(m_fValue);

			m_nValue = 1;
			ConVar::ClampValue(m_nValue);
		} else if(std::strcmp(value, "false") == 0) {
			m_fValue = 0.0f;
			ConVar::ClampValue(m_fValue);

			m_nValue = 0;
			ConVar::ClampValue(m_nValue);
		} else {
			const char *begin{value};
			const char *end{begin + len};

			std::from_chars(begin, end, m_fValue);
			ConVar::ClampValue(m_fValue);

			std::from_chars(begin, end, m_nValue);
			ConVar::ClampValue(m_nValue);
		}
	}

	void ConVar::InternalSetFloatValue(float value, bool force)
	{
		if(m_pParent && m_pParent != this) {
			m_pParent->ConVar::InternalSetFloatValue(value, force);
		}

		if(!ConCommandBase::IsFlagSet(FCVAR_NEVER_AS_STRING)) {
			constexpr std::size_t len{6 + 6};

			if(m_pszString) {
				m_pszString = static_cast<char *>(std::realloc(m_pszString, len+1));
			} else {
				m_pszString = static_cast<char *>(std::malloc(len+1));
			}

			char *begin{m_pszString};
			char *end{begin + len};

			std::to_chars_result tc_res{std::to_chars(begin, end, value)};
			tc_res.ptr[0] = '\0';

			m_StringLength = static_cast<int>(len);
		} else {
			ClearString();
		}

		m_fValue = value;
		ConVar::ClampValue(m_fValue);

		m_nValue = static_cast<int>(value);
		ConVar::ClampValue(m_nValue);
	}

	void ConVar::SetValue(bool value) noexcept
	{
		if(m_pParent && m_pParent != this) {
			m_pParent->ConVar::SetValue(value);
		}

		if(!ConCommandBase::IsFlagSet(FCVAR_NEVER_AS_STRING)) {
			std::size_t len{value ? 4u : 5u};

			if(m_pszString) {
				m_pszString = static_cast<char *>(std::realloc(m_pszString, len+1));
			} else {
				m_pszString = static_cast<char *>(std::malloc(len+1));
			}

			std::strncpy(m_pszString, value ? "true" : "false", len);
			m_pszString[len] = '\0';
			m_StringLength = static_cast<int>(len);
		} else {
			ClearString();
		}

		m_fValue = value ? 1.0f : 0.0f;
		ConVar::ClampValue(m_fValue);

		m_nValue = value ? 1 : 0;
		ConVar::ClampValue(m_nValue);
	}

	void ConVar::InternalSetIntValue(int value)
	{
		if(m_pParent && m_pParent != this) {
			m_pParent->ConVar::InternalSetIntValue(value);
		}

		if(!ConCommandBase::IsFlagSet(FCVAR_NEVER_AS_STRING)) {
			constexpr std::size_t len{6};

			if(m_pszString) {
				m_pszString = static_cast<char *>(std::realloc(m_pszString, len+1));
			} else {
				m_pszString = static_cast<char *>(std::malloc(len+1));
			}

			char *begin{m_pszString};
			char *end{begin + len};

			std::to_chars_result tc_res{std::to_chars(begin, end, value)};
			tc_res.ptr[0] = '\0';

			m_StringLength = static_cast<int>(len);
		} else {
			ClearString();
		}

		m_fValue = static_cast<float>(value);
		ConVar::ClampValue(m_fValue);

		m_nValue = value;
		ConVar::ClampValue(m_nValue);
	}

	void ConVar::ChangeStringValue(const char *value, [[maybe_unused]] float)
	{ ConVar::InternalSetValue(value); }

	ConVar::~ConVar()
	{
		if(m_pszString) {
			free(m_pszString);
		}
	}

	float ConVar::GetFloat() const noexcept
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::GetFloat();
		}

		return m_fValue;
	}

	int ConVar::GetInt() const noexcept
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::GetInt();
		}

		return m_nValue;
	}

	bool ConVar::GetBool() const noexcept
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::GetBool();
		}

		return (m_nValue > 0);
	}

	const char *ConVar::InternalGetString() const noexcept
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::InternalGetString();
		}

		return m_pszString;
	}

	const char *ConVar::GetString() const noexcept
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::GetString();
		}

		if(!ConCommandBase::IsFlagSet(FCVAR_NEVER_AS_STRING)) {
			return m_pszString;
		} else {
			return "";
		}
	}

	std::size_t ConVar::InternalGetStringLength() const noexcept
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::InternalGetStringLength();
		}

		return static_cast<std::size_t>(m_StringLength);
	}

	std::size_t ConVar::GetStringLength() const noexcept
	{
		if(m_pParent && m_pParent != this) {
			return m_pParent->ConVar::GetStringLength();
		}

		if(!ConCommandBase::IsFlagSet(FCVAR_NEVER_AS_STRING)) {
			return static_cast<std::size_t>(m_StringLength);
		} else {
			return 0;
		}
	}
}
