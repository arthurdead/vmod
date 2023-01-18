#pragma once

#include "../config.hpp"
#include <cstdlib>
#include <cstring>
#include <type_traits>

#if GSDK_ENGINE_BRANCH != GSDK_ENGINE_BRANCH_2007
	#define GSDK_NO_ALLOC_OVERRIDE
#endif

namespace gsdk
{
	using MemAllocFailHandler_t = size_t(*)(size_t);
	struct _CrtMemState;
	class IVirtualMemorySection;
	struct GenericMemoryStat_t;

	enum DumpStatsFormat_t : int
	{
		FORMAT_TEXT,
		FORMAT_HTML
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IMemAlloc
	{
	public:
		virtual void *Alloc(size_t) = 0;
		virtual void *Realloc(void *, size_t) = 0;
		virtual void Free(void *) = 0;
		virtual void *Expand_NoLongerSupported(void *, size_t) = 0;
		virtual void *Alloc(size_t , const char *, int) = 0;
		virtual void *Realloc(void *, size_t, const char *, int) = 0;
		virtual void Free(void *, const char *, int) = 0;
		virtual void *Expand_NoLongerSupported(void *, size_t, const char *, int) = 0;
		virtual size_t GetSize(void *) = 0;
		virtual void PushAllocDbgInfo(const char *, int) = 0;
		virtual void PopAllocDbgInfo() = 0;
		virtual long CrtSetBreakAlloc(long) = 0;
		virtual int CrtSetReportMode(int, int) = 0;
		virtual int CrtIsValidHeapPointer(const void *) = 0;
		virtual int CrtIsValidPointer(const void *, unsigned int, int) = 0;
		virtual int CrtCheckMemory() = 0;
		virtual int CrtSetDbgFlag(int) = 0;
		virtual void CrtMemCheckpoint(_CrtMemState *) = 0;
		virtual void DumpStats() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual void DumpStatsFileBase(const char *) = 0;
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void DumpStatsFileBase(const char *, DumpStatsFormat_t = FORMAT_TEXT) = 0;
	#else
		#error
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual size_t ComputeMemoryUsedBy(const char *) = 0;
	#endif
		virtual void *CrtSetReportFile(int, void *) = 0;
		virtual void *CrtSetReportHook(void *) = 0;
		virtual int CrtDbgReport(int, const char *, int, const char *, const char *) = 0;
		virtual int heapchk() = 0;
		virtual bool IsDebugHeap() = 0;
		virtual void GetActualDbgInfo(const char *&, int &) = 0;
		virtual void RegisterAllocation(const char *, int, int, int, unsigned int) = 0;
		virtual void RegisterDeallocation(const char *, int, int, int, unsigned int) = 0;
		virtual int GetVersion() = 0;
		virtual void CompactHeap() = 0;
		virtual MemAllocFailHandler_t SetAllocFailHandler(MemAllocFailHandler_t) = 0;
		virtual void DumpBlockStats(void *) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void SetStatsExtraInfo(const char *, const char *) = 0;
	#endif
		virtual size_t MemoryAllocFailed() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void CompactIncremental() = 0;
		virtual void OutOfMemory(size_t = 0) = 0;
		virtual void *RegionAlloc(int, size_t) = 0;
		virtual void *RegionAlloc(int, size_t, const char *, int) = 0;
		virtual void GlobalMemoryStatus(size_t *, size_t *) = 0;
		virtual IVirtualMemorySection *AllocateVirtualMemorySection(size_t) = 0;
		virtual int GetGenericMemoryStats(GenericMemoryStat_t **) = 0;
	#endif
		virtual unsigned int GetDebugInfoSize() = 0;
		virtual void SaveDebugInfo(void *) = 0;
		virtual void RestoreDebugInfo(const void *) = 0;
		virtual void InitDebugInfo(void *, const char *, int) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual void GlobalMemoryStatus(size_t *, size_t *) = 0;
	#endif

		inline void *CAlloc(std::size_t num, std::size_t size) noexcept
		{
			std::size_t total{num * size};
			void *ptr{Alloc(total)};
			std::memset(ptr, 0, total);
			return ptr;
		}
	};
	#pragma GCC diagnostic pop
}

#ifndef GSDK_NO_ALLOC_OVERRIDE
extern "C" __attribute__((__visibility__("default"))) gsdk::IMemAlloc *g_pMemAlloc;
#endif

namespace gsdk
{
	inline __attribute__((__always_inline__)) char *realloc_string(char *ptr, std::size_t size) noexcept
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		return static_cast<char *>(g_pMemAlloc->Realloc(ptr, size));
	#else
		return static_cast<char *>(std::realloc(ptr, size));
	#endif
	}

	inline __attribute__((__always_inline__)) char *reallocatable_string(std::size_t size) noexcept
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		return static_cast<char *>(g_pMemAlloc->Alloc(size));
	#else
		return static_cast<char *>(std::malloc(size));
	#endif
	}

	inline __attribute__((__always_inline__)) void free_reallocatable_string(char *ptr) noexcept
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		g_pMemAlloc->Free(ptr);
	#else
		std::free(ptr);
	#endif
	}

	inline __attribute__((__always_inline__)) char *alloc_string(std::size_t size) noexcept
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		return static_cast<char *>(g_pMemAlloc->Alloc(size));
	#else
		return new char[size];
	#endif
	}

	inline __attribute__((__always_inline__)) void free_string(char *ptr) noexcept
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		g_pMemAlloc->Free(ptr);
	#else
		delete[] ptr;
	#endif
	}

	template <typename T>
	inline __attribute__((__always_inline__)) T *alloc() noexcept
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		return static_cast<T *>(g_pMemAlloc->Alloc(sizeof(T)));
	#else
		return new T;
	#endif
	}

	template <typename T>
	inline __attribute__((__always_inline__)) void free(T *ptr) noexcept
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		g_pMemAlloc->Free(ptr);
	#else
		if constexpr(std::is_void_v<std::decay_t<T>>) {
			std::free(ptr);
		} else {
			delete ptr;
		}
	#endif
	}
}
