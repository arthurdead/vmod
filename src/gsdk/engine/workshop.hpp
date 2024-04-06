#pragma once

#include "../config.hpp"
#include "../tier1/appframework.hpp"
#include "../tier1/utlvector.hpp"
#include "../filesystem/filesystem.hpp"
#include "../tier0/platform.hpp"
#include <string_view>

#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wredundant-tags"
#pragma GCC diagnostic ignored "-Wconditionally-supported"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wsuggest-override"
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
#include <steam/isteamremotestorage.h>
#include <steam/isteamugc.h>
#ifndef __clang__
#pragma GCC diagnostic pop
#else
#pragma clang diagnostic pop
#endif

namespace gsdk
{
#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	class IWorkshopManager;
	struct AddonInfo;

	struct PublishedFileInfo_t
	{
		PublishedFileId_t m_nPublishedFileId;
		char m_rgchTitle[k_cchPublishedDocumentTitleMax];
		UGCHandle_t m_hFile;
		UGCHandle_t m_hPreviewFile;
		uint64 m_ulSteamIDOwner;
		uint32 m_rtimeCreated;
		uint32 m_rtimeUpdated;
		ERemoteStoragePublishedFileVisibility m_eVisibility;
		uint32 m_rtimeSubscribed;
		uint32 m_rtimeLastPlayed;
		uint32 m_rtimeCompleted;
		char m_rgchDescription[k_cchPublishedDocumentDescriptionMax];
		char m_pchFileName[k_cchFilenameMax];
		char m_rgchTags[k_cchTagListMax];
		bool m_bTagsTruncated;
		CCopyableUtlVector<char *> m_vTags;
		bool m_bVotingDataValid;
		uint32 m_unUpVotes;
		uint32 m_unDownVotes;
		float m_flVotingScore;
		uint32 m_unNumReports;
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class CBasePublishedFileRequest
	{
	public:
		virtual void OnError(EResult) = 0;
		virtual void OnLoaded(PublishedFileInfo_t &) = 0;

		PublishedFileId_t m_nTargetID;
	};
	#pragma GCC diagnostic pop

	enum UGCFileRequestStatus_t
	{
		UGCFILEREQUEST_ERROR = -1,
		UGCFILEREQUEST_READY,
		UGCFILEREQUEST_DOWNLOADING,
		UGCFILEREQUEST_DOWNLOAD_WRITING,
		UGCFILEREQUEST_UPLOADING,
		UGCFILEREQUEST_FINISHED
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IWorkshop : public IAppSystem
	{
	public:
		static constexpr std::string_view interface_name{"VWorkshop001"};

		virtual IWorkshopManager *WorkshopManager() const = 0;
		virtual void RunFrame() = 0;
		virtual int GetNumAddOnFiles() = 0;
		virtual AddonInfo *GetAddOnByIndex(unsigned int) const = 0;
		virtual const PublishedFileInfo_t *GetPublishedFileInfoByID(PublishedFileId_t) const = 0;
		virtual void GetAddOnFiles(CUtlVector<PublishedFileId_t> *) = 0;
		virtual bool AddOnQueryComplete() const = 0;
		virtual UGCHandle_t GetCurrentUGCDownloadHandle() const = 0;
		virtual void QueryForMapItems() = 0;
		virtual bool WorkshopItemsReady(bool &) const = 0;
		virtual int GetNumWorkshopItems() const = 0;
		virtual PublishedFileId_t GetWorkshopItemByIndex(int) const = 0;
		virtual void AddItem(PublishedFileId_t) = 0;
	};

	class IWorkshopManager
	{
	public:
		virtual void Update() = 0;
		virtual const PublishedFileInfo_t *GetPublishedFileInfoByID(PublishedFileId_t) const = 0;
		virtual bool AddFileInfoQuery(CBasePublishedFileRequest *, bool) = 0;
		virtual bool AddPublishedFileVoteInfoRequest(const PublishedFileInfo_t *, bool) = 0;
		virtual void UpdatePublishedItemVote(PublishedFileId_t, bool) = 0;
		virtual bool RemovePublishedFileInfo(PublishedFileId_t) = 0;
		virtual bool DeletePublishedFile(PublishedFileId_t) = 0;
		virtual bool DeleteUGCFileRequest(UGCHandle_t, bool) = 0;
		virtual bool UGCFileRequestExists(UGCHandle_t) = 0;
		virtual void GetUGCFullPath(UGCHandle_t, char *, size_t) = 0;
		virtual const char *GetUGCFileDirectory(UGCHandle_t) = 0;
		virtual const char *GetUGCFilename(UGCHandle_t) = 0;
		virtual UGCFileRequestStatus_t GetUGCFileRequestStatus(UGCHandle_t) = 0;
		virtual bool PromoteUGCFileRequestToTop(UGCHandle_t) = 0;
		virtual UGCHandle_t GetCurrentUGCDownloadHandle() const = 0;
		virtual UGCHandle_t GetUGCFileHandleByFilename(const char *) = 0;
		virtual UGCFileRequestStatus_t GetUGCFileRequestStatusByFilename(const char *) = 0;
		virtual float GetUGCFileDownloadProgress(UGCHandle_t) = 0;
		virtual void CreateFileDownloadRequest(UGCHandle_t, const char *, const char *, unsigned int, unsigned int = 0, bool = false) = 0;
		virtual void CreateFileUploadRequest(const char *, const char *, const char *, unsigned int) = 0;
	};
	#pragma GCC diagnostic pop
#endif
}
