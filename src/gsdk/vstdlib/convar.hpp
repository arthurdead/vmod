#pragma once

#include "../tier1/interface.hpp"
#include <string_view>

namespace gsdk
{
	template <typename T>
	class CUtlVector;
	class CUtlString;
	class ConVar;
	class ConCommandBase;
	class IConsoleDisplayFunc;
	class ICvarQuery;
	using CVarDLLIdentifier_t = int;
	class IConVar;
	using FnChangeCallback_t = void(*)(IConVar *, const char *, float);
	class ConCommand;
	struct Color;
	class CCommand;
	using FnCommandCallbackVoid_t = void(*)();
	using FnCommandCallback_t = void(*)(const CCommand &);
	class ICommandCallback;
	class ICommandCompletionCallback;

	constexpr int COMMAND_COMPLETION_MAXITEMS{64};
	constexpr int COMMAND_COMPLETION_ITEM_LENGTH{64};

	using FnCommandCompletionCallback = int(*)(const char *, char[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wshift-sign-overflow"
#endif
	enum : int
	{
		FCVAR_NONE =                          0,
		FCVAR_UNREGISTERED =            (1 << 0),
		FCVAR_DEVELOPMENTONLY =         (1 << 1),
		FCVAR_GAMEDLL =                 (1 << 2),
		FCVAR_CLIENTDLL =               (1 << 3),
		FCVAR_HIDDEN =                  (1 << 4),
		FCVAR_PROTECTED =               (1 << 5),
		FCVAR_SPONLY =                  (1 << 6),
		FCVAR_ARCHIVE =                 (1 << 7),
		FCVAR_NOTIFY =                  (1 << 8),
		FCVAR_USERINFO =                (1 << 9),
		FCVAR_CHEAT =                   (1 << 14),
		FCVAR_PRINTABLEONLY =           (1 << 10),
		FCVAR_UNLOGGED =                (1 << 11),
		FCVAR_NEVER_AS_STRING =         (1 << 12),
		FCVAR_REPLICATED =              (1 << 13),
		FCVAR_INTERNAL_USE =            (1 << 15),
		FCVAR_DEMO =                    (1 << 16),
		FCVAR_DONTRECORD =              (1 << 17),
		FCVAR_ALLOWED_IN_COMPETITIVE =  (1 << 18),
		FCVAR_RELOAD_MATERIALS =        (1 << 20),
		FCVAR_RELOAD_TEXTURES =         (1 << 21),
		FCVAR_NOT_CONNECTED =           (1 << 22),
		FCVAR_MATERIAL_SYSTEM_THREAD =  (1 << 23),
		FCVAR_ARCHIVE_XBOX =            (1 << 24),
		FCVAR_ACCESSIBLE_FROM_THREADS = (1 << 25),
		FCVAR_SERVER_CAN_EXECUTE =      (1 << 28),
		FCVAR_SERVER_CANNOT_QUERY =     (1 << 29),
		FCVAR_CLIENTCMD_CAN_EXECUTE =   (1 << 30),
		FCVAR_EXEC_DESPITE_DEFAULT =    (1 << 31),
		FCVAR_MATERIAL_THREAD_MASK =    (FCVAR_RELOAD_MATERIALS|FCVAR_RELOAD_TEXTURES|FCVAR_MATERIAL_SYSTEM_THREAD)
	};
#ifdef __clang__
	#pragma clang diagnostic pop
#endif

	class ICVarIterator
	{
	public:
		virtual ~ICVarIterator() = 0;
		virtual void SetFirst() = 0;
		virtual void Next() = 0;
		virtual	bool IsValid() = 0;
		virtual ConCommandBase *Get() = 0;
	};

	constexpr int INVALID_CVAR_DLL_IDENTIFIER{-1};

	class ConCommandBase
	{
	public:
		ConCommandBase() noexcept;

		virtual ~ConCommandBase();
		virtual bool IsCommand() const;
		virtual bool IsFlagSet(int flags) const;
		virtual void AddFlags(int flags) final;
		virtual const char *GetName() const;
		virtual const char *GetHelpText() const final;
		virtual bool IsRegistered() const final;
		virtual CVarDLLIdentifier_t GetDLLIdentifier() const = 0;
		virtual void CreateBase(const char *name, const char *help = nullptr, int flags = FCVAR_NONE);
		virtual void Init() = 0;

		bool IsCompetitiveRestricted() const noexcept;

		ConCommandBase *m_pNext;
		bool m_bRegistered;
		const char *m_pszName;
		const char *m_pszHelpString;
		int m_nFlags;
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IConVar
	{
	public:
		virtual void SetValue(const char *value) = 0;
		virtual void SetValue(float value) = 0;
		virtual void SetValue(int value) = 0;
		virtual const char *GetName() const = 0;
		virtual bool IsFlagSet(int flags) const = 0;
	};
	#pragma GCC diagnostic pop

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class ConVar : public ConCommandBase, public IConVar
	{
	public:
		ConVar() noexcept;
		~ConVar() override;

		virtual void InternalSetValue(const char *value) final;
		virtual void InternalSetFloatValue(float value, bool force = false) final;
		virtual void InternalSetIntValue(int value) final;
		virtual bool ClampValue(float &value) final;
		bool ClampValue(int &value);
		virtual void ChangeStringValue(const char *value, float old_float) final;

		void ClearString();

		void SetValue(const char *value) override final;
		void SetValue(float value) override final;
		void SetValue(int value) override final;
		void SetValue(bool value) noexcept;
		const char *GetName() const override final;
		bool IsFlagSet(int flags) const override final;

		float GetFloat() const noexcept;
		int GetInt() const noexcept;
		bool GetBool() const noexcept;
		const char *GetString() const noexcept;
		const char *InternalGetString() const noexcept;
		std::size_t GetStringLength() const noexcept;
		std::size_t InternalGetStringLength() const noexcept;

		bool IsCommand() const override final;
		void CreateBase(const char *name, const char *help = nullptr, int flags = FCVAR_NONE) override final;

		ConVar *m_pParent;
		const char *m_pszDefaultValue;
		char *m_pszString;
		int m_StringLength;
		float m_fValue;
		int m_nValue;
		bool m_bHasMin;
		float m_fMinVal;
		bool m_bHasMax;
		float m_fMaxVal;
		bool m_bHasCompMin;
		float m_fCompMinVal;
		bool m_bHasCompMax;
		float m_fCompMaxVal;
		bool m_bCompetitiveRestrictions;
		FnChangeCallback_t m_fnChangeCallback;
	};
	#pragma GCC diagnostic pop

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class ICvar : public IAppSystem
	{
	public:
		static constexpr std::string_view interface_name{"VEngineCvar004"};

		virtual CVarDLLIdentifier_t AllocateDLLIdentifier() = 0;
	private:
		virtual void RegisterConCommand_impl(ConCommandBase *) = 0;
		virtual void UnregisterConCommand_impl(ConCommandBase *) = 0;
	public:
		inline void RegisterConCommand(ConCommandBase *cmd) noexcept
		{
			RegisterConCommand_impl(cmd);
			cmd->m_nFlags &= ~FCVAR_UNREGISTERED;
		}
		inline void UnregisterConCommand(ConCommandBase *cmd) noexcept
		{
			UnregisterConCommand_impl(cmd);
			cmd->m_nFlags |= FCVAR_UNREGISTERED;
		}
		virtual void UnregisterConCommands(CVarDLLIdentifier_t) = 0;
		virtual const char *GetCommandLineValue(const char *) = 0;
		virtual ConCommandBase *FindCommandBase(const char *) = 0;
		virtual const ConCommandBase *FindCommandBase(const char *) const = 0;
		virtual ConVar *FindVar(const char *) = 0;
		virtual const ConVar *FindVar(const char *) const = 0;
		virtual ConCommand *FindCommand(const char *) = 0;
		virtual const ConCommand *FindCommand(const char *) const = 0;
		virtual ConCommandBase *GetCommands() = 0;
		virtual const ConCommandBase *GetCommands() const = 0;
		virtual void InstallGlobalChangeCallback(FnChangeCallback_t) = 0;
		virtual void RemoveGlobalChangeCallback(FnChangeCallback_t) = 0;
		virtual void CallGlobalChangeCallbacks(ConVar *, const char *, float) = 0;
		virtual void InstallConsoleDisplayFunc(IConsoleDisplayFunc *) = 0;
		virtual void RemoveConsoleDisplayFunc(IConsoleDisplayFunc *) = 0;
		virtual void ConsoleColorPrintf(const Color &, const char *, ...) const = 0;
		virtual void ConsolePrintf(const char *, ...) const = 0;
		virtual void ConsoleDPrintf(const char *, ...) const = 0;
		virtual void RevertFlaggedConVars(int) = 0;
		virtual void InstallCVarQuery(ICvarQuery *) = 0;
		virtual bool IsMaterialThreadSetAllowed() const = 0;
		virtual void QueueMaterialThreadSetValue(ConVar *, const char *) = 0;
		virtual void QueueMaterialThreadSetValue(ConVar *, int) = 0;
		virtual void QueueMaterialThreadSetValue(ConVar *, float) = 0;
		virtual bool HasQueuedMaterialThreadConVarSets() const = 0;
		virtual int ProcessQueuedMaterialThreadConVarSets() = 0;
		virtual ICVarIterator *FactoryInternalIterator() = 0;
	};
	#pragma GCC diagnostic pop

	class CCommand
	{
	public:
		static constexpr int COMMAND_MAX_ARGC{64};
		static constexpr int COMMAND_MAX_LENGTH{512};

		int m_nArgc;
		int m_nArgv0Size;
		char m_pArgSBuffer[COMMAND_MAX_LENGTH];
		char m_pArgvBuffer[COMMAND_MAX_LENGTH];
		const char *m_ppArgv[COMMAND_MAX_ARGC];
	};

	class ConCommand : public ConCommandBase
	{
	public:
		ConCommand() noexcept;

		virtual int AutoCompleteSuggest(const char *, CUtlVector<CUtlString> &);
		virtual bool CanAutoComplete();
		virtual void Dispatch(const CCommand &) = 0;

		bool IsCommand() const override final;
		void CreateBase(const char *name, const char *help = nullptr, int flags = FCVAR_NONE) override final;

		union
		{
			FnCommandCallbackVoid_t m_fnCommandCallbackV1;
			FnCommandCallback_t m_fnCommandCallback;
			ICommandCallback *m_pCommandCallback; 
		};

		union
		{
			FnCommandCompletionCallback m_fnCompletionCallback;
			ICommandCompletionCallback *m_pCommandCompletionCallback;
		};

		bool m_bHasCompletionCallback : 1;
		bool m_bUsingNewCommandCallback : 1;
		bool m_bUsingCommandCallbackInterface : 1;
	};
}
