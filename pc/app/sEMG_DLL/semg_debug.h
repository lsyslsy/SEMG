#pragma once

/**********setting***********/
#define DATASCALE  1.0

#define DLL_DEBUG_MODE	/* debug mode right now, comment out if release the dll */
#define DEBUG_INFO
#define DEBUG_WARN
#define DEBUG_ERROR
/**********************/

void OutputDebugPrintf(const char * strOutputString,...);
#ifdef DLL_DEBUG_MODE
#ifdef DEBUG_INFO
#define DebugInfo  OutputDebugPrintf
#else
#define DebugInfo(x)
#endif
#ifdef DEBUG_WARN 
#define DebugWarn OutputDebugPrintf
#else
#define DebugWarn(x)
#endif
#ifdef DEBUG_ERROR
#define DebugError OutputDebugPrintf
#else
#define DebugError(x)
#endif

#else
#define DebugWarn(x)
#define DebugError(x)
#define DebugInfo(x)

#endif