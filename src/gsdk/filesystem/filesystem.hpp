#pragma once

#include <string_view>
#include "../tier1/interface.hpp"
#include "../tier1/appframework.hpp"
#include "../tier1/utlsymbol.hpp"
#include "../config.hpp"

//#define GSDK_TRACK_BLOCKING_IO

namespace gsdk
{
	class CUtlBuffer;
	using FileHandle_t  = void *;
	using FileCacheHandle_t = void *;
	using PathTypeQuery_t = unsigned int;
	enum KeyValuesPreloadType_t : int;
	using FileFindHandle_t = int;
	enum FileWarningLevel_t : int;
	struct FileSystemStatistics;
	using FSAllocFunc_t = void *(*)(const char *, unsigned);
	enum FileSystemSeek_t : int;
	enum FilesystemMountRetval_t : int;
	class CSysModule;
	enum FSAsyncStatus_t : int;
	struct FSAsyncControl_t;
	class IAsyncFileFetch;
	struct FileAsyncRequest_t;
	struct FSAsyncFile_t;
	using WaitForResourcesHandle_t = int;
	using FileSystemLoggingFunc_t = void(*)(const char *, const char *);
	class IPureServerWhitelist;
	enum DVDMode_t : int;
	enum ECacheCRCType : int;
	class CUnverifiedFileHash;
	class CMemoryFileBacking;
	enum EFileCRCStatus : int;
	class IFileList;
	using FSDirtyDiskReportFunc_t = void(*)();
	struct FileHash_t;
	struct MD5Value_t;
	class CUtlString;
	using CRC32_t = unsigned int;
	template <typename T>
	class CUtlVector;
	class KeyValues;

	enum PathTypeFilter_t : int
	{
		FILTER_NONE,
		FILTER_CULLPACK,
		FILTER_CULLNONPACK,
	};

	enum SearchPathAdd_t : int
	{
		PATH_ADD_TO_HEAD,
		PATH_ADD_TO_TAIL,
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IBaseFileSystem
	{
	public:
		virtual int Read(void *, int, FileHandle_t) = 0;
		virtual int Write(const void *, int, FileHandle_t) = 0;
		virtual FileHandle_t Open(const char *, const char *, const char * = nullptr) = 0;
		virtual void Close(FileHandle_t) = 0;
		virtual void Seek(FileHandle_t, int, FileSystemSeek_t) = 0;
		virtual unsigned int Tell(FileHandle_t) = 0;
		virtual unsigned int Size(FileHandle_t) = 0;
		virtual unsigned int Size(const char *, const char * = nullptr) = 0;
		virtual void Flush(FileHandle_t) = 0;
		virtual bool Precache(const char *, const char * = nullptr) = 0;
		virtual bool FileExists(const char *, const char * = nullptr) = 0;
		virtual bool IsFileWritable(const char *, const char * = nullptr ) = 0;
		virtual bool SetFileWritable(const char *, bool, const char * = nullptr) = 0;
		virtual long GetFileTime(const char *, const char * = nullptr) = 0;
		virtual bool ReadFile(const char *, const char *, CUtlBuffer &, int = 0, int = 0, FSAllocFunc_t = nullptr) = 0;
		virtual bool WriteFile(const char *, const char *, CUtlBuffer & ) = 0;
		virtual bool UnzipFile(const char *, const char *, const char *) = 0;
	};
	#pragma GCC diagnostic pop

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IFileSystem : public IAppSystem, public IBaseFileSystem
	{
	public:
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V1)
		static constexpr std::string_view interface_name{"VFileSystem022"};
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		static constexpr std::string_view interface_name{"VFileSystem017"};
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, <=, GSDK_ENGINE_BRANCH_2010_V0)
		static constexpr std::string_view interface_name{"VFileSystem018"};
	#else
		#error
	#endif

		virtual bool IsSteam() const = 0;
		virtual FilesystemMountRetval_t MountSteamContent(int = -1) = 0;
		virtual void AddSearchPath(const char *, const char *, SearchPathAdd_t = PATH_ADD_TO_TAIL) = 0;
		virtual bool RemoveSearchPath(const char *, const char * = nullptr) = 0;
		virtual void RemoveAllSearchPaths() = 0;
		virtual void RemoveSearchPaths(const char *) = 0;
		virtual void MarkPathIDByRequestOnly(const char *, bool) = 0;
		virtual const char *RelativePathToFullPath(const char *, const char *, char *, int, PathTypeFilter_t = FILTER_NONE, PathTypeQuery_t * = nullptr) = 0;
		virtual int GetSearchPath(const char *, bool, char *, int) = 0;
		virtual bool AddPackFile(const char *, const char *) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, <=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsLocalizedPath(const char *) = 0;
	#endif
		virtual void RemoveFile(const char *, const char * = nullptr) = 0;
		virtual bool RenameFile(const char *, const char *, const char * = nullptr) = 0;
		virtual void CreateDirHierarchy( const char *, const char * = nullptr) = 0;
		virtual bool IsDirectory(const char *, const char * = nullptr) = 0;
		virtual void FileTimeToString(char *, int, long) = 0;
		virtual void SetBufferSize(FileHandle_t, unsigned) = 0;
		virtual bool IsOk(FileHandle_t) = 0;
		virtual bool EndOfFile(FileHandle_t) = 0;
		virtual char *ReadLine(char *, int, FileHandle_t) = 0;
		virtual int FPrintf(FileHandle_t, const char *, ...) = 0;
		virtual CSysModule *LoadModule(const char *, const char * = nullptr, bool = true) = 0;
		virtual void UnloadModule(CSysModule *) = 0;
		virtual const char *FindFirst(const char *, FileFindHandle_t *) = 0;
		virtual const char *FindNext(FileFindHandle_t) = 0;
		virtual bool FindIsDirectory(FileFindHandle_t) = 0;
		virtual void FindClose(FileFindHandle_t) = 0;
		virtual const char *FindFirstEx(const char *, const char *, FileFindHandle_t *) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void FindFileAbsoluteList(CUtlVector<CUtlString> &, const char *, const char *) = 0;
	#endif
		virtual const char *GetLocalPath(const char *, char *, int) = 0;
		virtual bool FullPathToRelativePath(const char *, char *, int) = 0;
		virtual bool GetCurrentDirectory(char *, int) = 0;
		virtual FileNameHandle_t FindOrAddFileName(const char *) = 0;
		virtual bool String(const FileNameHandle_t &, char *, int) = 0;
		virtual FSAsyncStatus_t AsyncReadMultiple(const FileAsyncRequest_t *, int,  FSAsyncControl_t * = nullptr) = 0;
		virtual FSAsyncStatus_t AsyncAppend(const char *, const void *, int, bool, FSAsyncControl_t * = nullptr) = 0;
		virtual FSAsyncStatus_t AsyncAppendFile(const char *, const char *, FSAsyncControl_t * = nullptr) = 0;
		virtual void AsyncFinishAll(int = 0) = 0;
		virtual void AsyncFinishAllWrites() = 0;
		virtual FSAsyncStatus_t AsyncFlush() = 0;
		virtual bool AsyncSuspend() = 0;
		virtual bool AsyncResume() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual void AsyncAddFetcher(IAsyncFileFetch *) = 0;
		virtual void AsyncRemoveFetcher(IAsyncFileFetch *) = 0;
	#endif
		virtual FSAsyncStatus_t AsyncBeginRead(const char *, FSAsyncFile_t *) = 0;
		virtual FSAsyncStatus_t AsyncEndRead(FSAsyncFile_t) = 0;
		virtual FSAsyncStatus_t AsyncFinish(FSAsyncControl_t, bool = true) = 0;
		virtual FSAsyncStatus_t AsyncGetResult(FSAsyncControl_t, void **, int *) = 0;
		virtual FSAsyncStatus_t AsyncAbort(FSAsyncControl_t) = 0;
		virtual FSAsyncStatus_t AsyncStatus(FSAsyncControl_t) = 0;
		virtual FSAsyncStatus_t AsyncSetPriority(FSAsyncControl_t, int) = 0;
		virtual void AsyncAddRef(FSAsyncControl_t) = 0;
		virtual void AsyncRelease(FSAsyncControl_t) = 0;
		virtual WaitForResourcesHandle_t WaitForResources(const char *) = 0;
		virtual bool GetWaitForResourcesProgress(WaitForResourcesHandle_t, float *, bool *) = 0;
		virtual void CancelWaitForResources(WaitForResourcesHandle_t) = 0;
		virtual int HintResourceNeed(const char *, int) = 0;
		virtual bool IsFileImmediatelyAvailable(const char *) = 0;
		virtual void GetLocalCopy(const char *) = 0;
		virtual void PrintOpenedFiles() = 0;
		virtual void PrintSearchPaths() = 0;
		virtual void SetWarningFunc(void (*)(const char *, ...)) = 0;
		virtual void SetWarningLevel(FileWarningLevel_t) = 0;
		virtual void AddLoggingFunc(void (*)(const char *, const char *)) = 0;
		virtual void RemoveLoggingFunc(FileSystemLoggingFunc_t) = 0;
		virtual const FileSystemStatistics *GetFilesystemStatistics() = 0;
		virtual FileHandle_t OpenEx(const char *, const char *, unsigned = 0, const char * = nullptr, char ** = nullptr) = 0;
		virtual int ReadEx(void *, int, int, FileHandle_t) = 0;
		virtual int ReadFileEx(const char *, const char *, void **, bool = false, bool = false, int = 0, int = 0, FSAllocFunc_t = nullptr) = 0;
		virtual FileNameHandle_t FindFileName(const char *) = 0;
	#ifdef GSDK_TRACK_BLOCKING_IO
		virtual void EnableBlockingFileAccessTracking(bool) = 0;
		virtual bool IsBlockingFileAccessEnabled() const = 0;
		virtual IBlockingFileItemList *RetrieveBlockingFileAccessInfo() = 0;
	#endif
		virtual void SetupPreloadData() = 0;
		virtual void DiscardPreloadData() = 0;
		virtual void LoadCompiledKeyValues(KeyValuesPreloadType_t, const char *) = 0;
		virtual KeyValues *LoadKeyValues(KeyValuesPreloadType_t, const char *, const char * = nullptr) = 0;
		virtual bool LoadKeyValues(KeyValues &, KeyValuesPreloadType_t, const char *, const char * = nullptr) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual bool ExtractRootKeyName(KeyValuesPreloadType_t, char *, size_t, const char *, const char *pPathID = nullptr) = 0;
	#endif
		virtual FSAsyncStatus_t AsyncWrite(const char *, const void *, int, bool, bool = false, FSAsyncControl_t * = nullptr ) = 0;
		virtual FSAsyncStatus_t AsyncWriteFile(const char *, const CUtlBuffer *, int, bool, bool = false, FSAsyncControl_t *pControl = nullptr) = 0;
		virtual FSAsyncStatus_t AsyncReadMultipleCreditAlloc(const FileAsyncRequest_t *, int, const char *, int, FSAsyncControl_t * = nullptr) = 0;
		virtual bool GetFileTypeForFullPath(const char *, wchar_t *, size_t) = 0;
		virtual bool ReadToBuffer(FileHandle_t, CUtlBuffer &, int = 0, FSAllocFunc_t = nullptr) = 0;
		virtual bool GetOptimalIOConstraints(FileHandle_t, unsigned *, unsigned *, unsigned *) = 0;
		virtual void *AllocOptimalReadBuffer(FileHandle_t, unsigned = 0, unsigned = 0) = 0;
		virtual void FreeOptimalReadBuffer(void *) = 0;
		virtual void BeginMapAccess() = 0;
		virtual void EndMapAccess() = 0;
		virtual bool FullPathToRelativePathEx(const char *, const char *, char *, int) = 0;
		virtual int GetPathIndex(const FileNameHandle_t &) = 0;
		virtual long GetPathTime(const char *, const char *) = 0;
		virtual DVDMode_t GetDVDMode() = 0;
		virtual void EnableWhitelistFileTracking(bool, bool, bool) = 0;
		virtual void RegisterFileWhitelist(IPureServerWhitelist *, IFileList **) = 0;
		virtual void MarkAllCRCsUnverified() = 0;
		virtual void CacheFileCRCs(const char *, ECacheCRCType, IFileList *) = 0;
		virtual EFileCRCStatus CheckCachedFileHash(const char *, const char *, int, FileHash_t *) = 0;
		virtual int GetUnverifiedFileHashes(CUnverifiedFileHash *, int) = 0;
		virtual int GetWhitelistSpewFlags() = 0;
		virtual void SetWhitelistSpewFlags(int) = 0;
		virtual void InstallDirtyDiskReportFunc(FSDirtyDiskReportFunc_t) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual FileCacheHandle_t CreateFileCache() = 0;
		virtual void AddFilesToFileCache(FileCacheHandle_t, const char **, int, const char *) = 0;
		virtual bool IsFileCacheFileLoaded(FileCacheHandle_t, const char *) = 0;
		virtual bool IsFileCacheLoaded(FileCacheHandle_t) = 0;
		virtual void DestroyFileCache(FileCacheHandle_t) = 0;
		virtual bool RegisterMemoryFile(CMemoryFileBacking *, CMemoryFileBacking **) = 0;
		virtual void UnregisterMemoryFile(CMemoryFileBacking *) = 0;
		virtual void CacheAllVPKFileHashes(bool, bool) = 0;
		virtual bool CheckVPKFileHash(int, int, int, MD5Value_t &) = 0;
		virtual void NotifyFileUnloaded(const char *, const char *) = 0;
		virtual bool GetCaseCorrectFullPath_Ptr(const char *, char *, int) = 0;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual bool IsLaunchedFromXboxHDD() = 0;
		virtual bool IsInstalledToXboxHDDCache() = 0;
		virtual bool IsDVDHosted() = 0;
		virtual bool IsInstallAllowed() = 0;
		virtual int GetSearchPathID(char *, int) = 0;
		virtual bool FixupSearchPathsAfterInstall() = 0;
		virtual FSDirtyDiskReportFunc_t GetDirtyDiskReportFunc() = 0;
		virtual void AddVPKFile(const char *, SearchPathAdd_t = PATH_ADD_TO_TAIL) = 0;
		virtual void RemoveVPKFile(const char * ) = 0;
		virtual void GetVPKFileNames(CUtlVector<CUtlString> &) = 0;
		virtual void RemoveAllMapSearchPaths() = 0;
		virtual void SyncDvdDevCache() = 0;
		virtual bool GetStringFromKVPool(CRC32_t, unsigned int, char *, int) = 0;
		virtual bool DiscoverDLC(int) = 0;
		virtual int IsAnyDLCPresent(bool * = nullptr) = 0;
		virtual bool GetAnyDLCInfo(int, unsigned int *, wchar_t *, int) = 0;
		virtual int IsAnyCorruptDLC() = 0;
		virtual bool GetAnyCorruptDLCInfo(int, wchar_t *, int) = 0;
		virtual bool AddDLCSearchPaths() = 0;
		virtual bool IsSpecificDLCPresent(unsigned int) = 0;
		virtual void SetIODelayAlarm(float) = 0;
		virtual void AddXLSPUpdateSearchPath(const void *, int) = 0;
	#endif
	};
	#pragma GCC diagnostic pop
}
