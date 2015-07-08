#ifndef _SEMG_DEBUG_H_
#define _SEMG_DEBUG_H_

/**********setting***********/
#define DATASCALE  1.0

#define DEBUG_INFO
#define DEBUG_WARN
#define DEBUG_ERROR
/**********************/

void OutputDebugPrintf(const char * strOutputString,...);
#ifndef NDEBUG
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

#endif