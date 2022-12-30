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
