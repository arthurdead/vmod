#include "convar.hpp"

namespace gsdk
{
	ConCommandBase::~ConCommandBase() {}
	bool ConCommandBase::IsCommand() const { return true; }
	bool ConCommandBase::IsFlagSet(int flags) const { return (m_nFlags & flags); }
	void ConCommandBase::AddFlags(int flags) { m_nFlags |= flags; }
	const char *ConCommandBase::GetName() const { return m_pszName; }
	const char *ConCommandBase::GetHelpText() const { return m_pszHelpString; }
	bool ConCommandBase::IsRegistered() const { return m_bRegistered; }

	void ConCommandBase::CreateBase(const char *pName, const char *pHelpString, int flags)
	{
		m_pszName = pName;
		m_pszHelpString = pHelpString ? pHelpString : "";
		m_nFlags = flags;
	}

	int ConCommand::AutoCompleteSuggest(const char *, CUtlVector<CUtlString> &) { return 0; }
	bool ConCommand::CanAutoComplete() { return false; }
}
