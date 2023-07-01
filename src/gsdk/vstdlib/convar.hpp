#pragma once

#include "../tier1/interface.hpp"
#include "../tier1/appframework.hpp"
#include "../tier0/dbg.hpp"
#include <string_view>
#include <cstring>
#include "../tier1/utlvector.hpp"

namespace gsdk
{
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
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		FCVAR_INTERNAL_USE =            (1 << 15),
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		FCVAR_SS =                      (1 << 15),
	#endif
		FCVAR_DEMO =                    (1 << 16),
		FCVAR_DONTRECORD =              (1 << 17),
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		FCVAR_ALLOWED_IN_COMPETITIVE =  (1 << 18),
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		FCVAR_SS_ADDED =                (1 << 18),
		FCVAR_RELEASE =                 (1 << 19),
	#endif
		FCVAR_RELOAD_MATERIALS =        (1 << 20),
		FCVAR_RELOAD_TEXTURES =         (1 << 21),
		FCVAR_NOT_CONNECTED =           (1 << 22),
		FCVAR_MATERIAL_SYSTEM_THREAD =  (1 << 23),
		FCVAR_ARCHIVE_XBOX =            (1 << 24),
		FCVAR_ACCESSIBLE_FROM_THREADS = (1 << 25),
		FCVAR_SERVER_CAN_EXECUTE =      (1 << 28),
		FCVAR_SERVER_CANNOT_QUERY =     (1 << 29),
		FCVAR_CLIENTCMD_CAN_EXECUTE =   (1 << 30),
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		FCVAR_EXEC_DESPITE_DEFAULT =    (1 << 31),
	#endif
		FCVAR_MATERIAL_THREAD_MASK =    (FCVAR_RELOAD_MATERIALS|FCVAR_RELOAD_TEXTURES|FCVAR_MATERIAL_SYSTEM_THREAD)
	};
#ifdef __clang__
	#pragma clang diagnostic pop
#endif

	using fcvar_t = decltype(FCVAR_NONE);
}

constexpr auto operator|(int rhs, gsdk::fcvar_t lhs) noexcept
{ return static_cast<gsdk::fcvar_t>(static_cast<int>(rhs) | static_cast<int>(lhs)); }

constexpr gsdk::fcvar_t &operator|=(gsdk::fcvar_t &rhs, int lhs) noexcept
{ rhs = static_cast<gsdk::fcvar_t>(static_cast<int>(rhs) | static_cast<int>(lhs)); return rhs; }
constexpr gsdk::fcvar_t &operator&=(gsdk::fcvar_t &rhs, int lhs) noexcept
{ rhs = static_cast<gsdk::fcvar_t>(static_cast<int>(rhs) & static_cast<int>(lhs)); return rhs; }

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
	class CCommand;
	using FnCommandCallbackVoid_t = void(*)();
	using FnCommandCallback_t = void(*)(const CCommand &);
	class ICommandCallback;
	class ICommandCompletionCallback;

	constexpr int COMMAND_COMPLETION_MAXITEMS{64};
	constexpr int COMMAND_COMPLETION_ITEM_LENGTH{64};

	using FnCommandCompletionCallback = int(*)(const char *, char[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class ICVarIterator
	{
	public:
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V1)
		virtual ~ICVarIterator() = 0;
	#endif
		virtual void SetFirst() = 0;
		virtual void Next() = 0;
		virtual bool IsValid() = 0;
		virtual ConCommandBase *Get() = 0;
	};
	#pragma GCC diagnostic pop

	constexpr int INVALID_CVAR_DLL_IDENTIFIER{-1};

	class ConCommandBase
	{
	public:
		ConCommandBase() noexcept = default;

		virtual ~ConCommandBase();
		virtual bool IsCommand() const;
		virtual bool IsFlagSet(int flags) const;
		virtual void AddFlags(int flags) final;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void RemoveFlags(int flags) final;
		virtual int GetFlags() const final;
	#endif
		virtual const char *GetName() const;
		virtual const char *GetHelpText() const final;
		virtual bool IsRegistered() const final;
		virtual CVarDLLIdentifier_t GetDLLIdentifier() const = 0;
		virtual void Create(const char *name, const char *help = nullptr, int flags = FCVAR_NONE);
		virtual void Init() = 0;

		bool IsCompetitiveRestricted() const noexcept;

		ConCommandBase *m_pNext{nullptr};
		bool m_bRegistered{false};
		const char *m_pszName{nullptr};
		const char *m_pszHelpString{nullptr};
		fcvar_t m_nFlags{FCVAR_UNREGISTERED};

	private:
		ConCommandBase(const ConCommandBase &) = delete;
		ConCommandBase &operator=(const ConCommandBase &) = delete;
		ConCommandBase(ConCommandBase &&) = delete;
		ConCommandBase &operator=(ConCommandBase &&) = delete;
	};

#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
	#pragma clang diagnostic ignored "-Wweak-vtables"
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
	class IConVar
	{
	public:
		virtual void SetValue(const char *value) = 0;
		virtual void SetValue(float value) = 0;
		virtual void SetValue(int value) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void SetValue(Color value) = 0;
	#endif
		virtual const char *GetName() const = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual const char *GetBaseName() const = 0;
	#endif
		virtual bool IsFlagSet(int flags) const = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual int GetSplitScreenPlayerSlot() const = 0;
	#endif
	};
#ifdef __clang__
	#pragma clang diagnostic pop
#else
	#pragma GCC diagnostic pop
#endif

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class ConVar : public ConCommandBase, public IConVar
	{
	public:
		ConVar() noexcept = default;
		~ConVar() override;

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual const char *GetBaseName() const final;
		virtual int GetSplitScreenPlayerSlot() const final;
	#endif
		virtual void SetValue(const char *value) final;
		virtual void SetValue(float value) final;
		virtual void SetValue(int value) final;
		void SetValue(bool value) noexcept;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void SetValue(Color value) final;
	#endif
		virtual void InternalSetValue(const char *value) final;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V2)
		virtual void InternalSetFloatValue(float value, bool force = false) final;
	#else
		virtual void InternalSetFloatValue(float value) final;
	#endif
		virtual void InternalSetIntValue(int value) final;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void InternalSetColorValue(Color value) final;
	#endif
		virtual bool ClampValue(float &value) final;
		bool ClampValue(int &value);
		virtual void ChangeStringValue(const char *value, float old_float) final;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void Create(const char *name, const char *default_value, int flags = FCVAR_NONE, const char *help = nullptr, bool has_min = false, float min = 0.0f, bool has_max = false, float max = 0.0f, FnChangeCallback_t callback = nullptr) final;
	#endif

		void ClearString();

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
		void Create(const char *name, const char *help = nullptr, int flags = FCVAR_NONE) override final;

		ConVar *m_pParent{nullptr};
		const char *m_pszDefaultValue{nullptr};
		char *m_pszString{nullptr};
		int m_StringLength{0};
		float m_fValue{0.0f};
		int m_nValue{0};
		bool m_bHasMin{false};
		float m_fMinVal{0.0f};
		bool m_bHasMax{false};
		float m_fMaxVal{0.0f};
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		bool m_bHasCompMin{false};
		float m_fCompMinVal{0.0f};
		bool m_bHasCompMax{false};
		float m_fCompMaxVal{0.0f};
		bool m_bCompetitiveRestrictions{false};
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		FnChangeCallback_t m_fnChangeCallback{nullptr};
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		CUtlVector<FnChangeCallback_t> m_fnChangeCallbacks;
	#else
		#error
	#endif

	private:
		ConVar(const ConVar &) = delete;
		ConVar &operator=(const ConVar &) = delete;
		ConVar(ConVar &&) = delete;
		ConVar &operator=(ConVar &&) = delete;
	};
	#pragma GCC diagnostic pop

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class ICvar : public IAppSystem
	{
	public:
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		static constexpr std::string_view interface_name{"VEngineCvar004"};
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		static constexpr std::string_view interface_name{"VEngineCvar007"};
	#else
		#error
	#endif

		virtual CVarDLLIdentifier_t AllocateDLLIdentifier() = 0;
	private:
		virtual void RegisterConCommand_impl(ConCommandBase *) = 0;
		virtual void UnregisterConCommand_impl(ConCommandBase *) = 0;
	public:
		inline void RegisterConCommand(ConCommandBase *cmd) noexcept
		{
			cmd->m_nFlags &= ~FCVAR_UNREGISTERED;
			RegisterConCommand_impl(cmd);
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
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual ConCommandBase *GetCommands() = 0;
		virtual const ConCommandBase *GetCommands() const = 0;
	#endif
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
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void SetMaxSplitScreenSlots(int) = 0;
		virtual int GetMaxSplitScreenSlots() const = 0;
		virtual void AddSplitScreenConVars() = 0;
		virtual void RemoveSplitScreenConVars(CVarDLLIdentifier_t) = 0;
		virtual int GetConsoleDisplayFuncCount() const = 0;
		virtual void GetConsoleText(int, char *, size_t) const = 0;
	#endif
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

		inline CCommand() noexcept
		{
			std::memset(m_pArgSBuffer, 0, sizeof(m_pArgSBuffer));
			std::memset(m_pArgvBuffer, 0, sizeof(m_pArgvBuffer));
			std::memset(m_ppArgv, 0, sizeof(m_ppArgv));
		}

		int m_nArgc{0};
		int m_nArgv0Size{0};
		char m_pArgSBuffer[COMMAND_MAX_LENGTH];
		char m_pArgvBuffer[COMMAND_MAX_LENGTH];
		const char *m_ppArgv[COMMAND_MAX_ARGC];

	private:
		CCommand(const CCommand &) = delete;
		CCommand &operator=(const CCommand &) = delete;
		CCommand(CCommand &&) = delete;
		CCommand &operator=(CCommand &&) = delete;
	};

	class ConCommand : public ConCommandBase
	{
	public:
		ConCommand() noexcept = default;

		virtual int AutoCompleteSuggest(const char *, CUtlVector<CUtlString> &);
		virtual bool CanAutoComplete();
		virtual void Dispatch(const CCommand &) = 0;

		bool IsCommand() const override final;
		void Create(const char *name, const char *help = nullptr, int flags = FCVAR_NONE) override final;

		union
		{
			FnCommandCallbackVoid_t m_fnCommandCallbackV1;
			FnCommandCallback_t m_fnCommandCallback{nullptr};
			ICommandCallback *m_pCommandCallback; 
		};

		union
		{
			FnCommandCompletionCallback m_fnCompletionCallback{nullptr};
			ICommandCompletionCallback *m_pCommandCompletionCallback;
		};

		bool m_bHasCompletionCallback : 1 {false};
		bool m_bUsingNewCommandCallback : 1 {false};
		bool m_bUsingCommandCallbackInterface : 1 {false};

	private:
		ConCommand(const ConCommand &) = delete;
		ConCommand &operator=(const ConCommand &) = delete;
		ConCommand(ConCommand &&) = delete;
		ConCommand &operator=(ConCommand &&) = delete;
	};
}
