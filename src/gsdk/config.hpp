#pragma once

#define GSDK_ENGINE_TF2 0
#define GSDK_ENGINE_PORTAL2 2
#define GSDK_ENGINE_L4D2 3

#ifndef GSDK_ENGINE
	#error
#endif

#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	#define GSDK_L4D2_SPECIFIC(...) __VA_ARGS__
	#define GSDK_TF2_SPECIFIC(...) 
#elif GSDK_ENGINE == GSDK_ENGINE_TF2
	#define GSDK_L4D2_SPECIFIC(...) 
	#define GSDK_TF2_SPECIFIC(...) __VA_ARGS__
#else
	#error
#endif

#define GSDK_DLL_HYBRID 0
#define GSDK_DLL_SERVER 1
#define GSDK_DLL_CLIENT 2

#define GSDK_DLL GSDK_DLL_HYBRID

#if GSDK_DLL == GSDK_DLL_CLIENT
	#error "not supported yet"
#endif

#if GSDK_ENGINE == GSDK_ENGINE_PORTAL2
	#define GSDK_NO_SYMBOLS
#elif GSDK_ENGINE == GSDK_ENGINE_TF2 || \
		GSDK_ENGINE == GSDK_ENGINE_L4D2
	#if GSDK_DLL == GSDK_DLL_CLIENT
		#define GSDK_NO_SYMBOLS
	#endif
#endif

#ifdef GSDK_NO_SYMBOLS
	#error "not supported yet"
#endif
