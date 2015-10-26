#ifndef _SEMG_DEBUG_H
#define _SEMG_DEBUG_H

#define DBUG_THREAD

//#define ARM_VERSION // ARM版本
//#define TEST_MODE

//============ setting ===========//
#define DEBUG_MODE
//#define DEBUG_INFO
#define DEBUG_WARN
#define DEBUG_ERROR
//=================================//

#ifdef DEBUG_MODE
	#ifdef DEBUG_INFO
		#define DebugInfo(x, args...)  printf(x, ##args)
	#else
		#define DebugInfo(x, args...)
	#endif
	#ifdef DEBUG_WARN
		#define DebugWarn(x, args...) printf(x, ##args)
	#else
		#define DebugWarn(x, args...)
	#endif
	#ifdef DEBUG_ERROR
		#define DebugError(x, args...) fprintf(stderr, x, ##args)
	#else
		#define DebugError(x, args...)
	#endif

#else
	#define DebugWarn(x, args...)
	#define DebugError(x, args...)
	#define DebugInfo(x, args...)

#endif

#define DebugNone(x, args...)	//do nothing




#endif
