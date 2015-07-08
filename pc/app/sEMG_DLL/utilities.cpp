#include <stdio.h>
#include <stdarg.h>
#include "semg_debug.h"
#ifdef IN_WINDOWS
#include <windows.h>
#else
#define OutputDebugString(x) printf("%s", x)
#endif

void OutputDebugPrintf(const char * strOutputString,...)
{
	char strBuffer[4096]={0};
	va_list vlArgs;
	va_start(vlArgs,strOutputString);
	#ifdef IN_WINDOWS
	_vsnprintf(strBuffer,sizeof(strBuffer)-1,strOutputString,vlArgs);
	#else
	vsnprintf(strBuffer,sizeof(strBuffer)-1,strOutputString,vlArgs);
	#endif
	//vsprintf(strBuffer,strOutputString,vlArgs);
	va_end(vlArgs);
	OutputDebugString(strBuffer);
}

/*#ifdef DLL_DEBUG_MODE
#ifdef DEBUG_INFO

#endif
#ifdef DEBUG_WARN
void DebugWarn(const char * strOutputString,...)
{
	char strBuffer[4096]={0};
	va_list vlArgs;
	va_start(vlArgs,strOutputString);
	_vsnprintf(strBuffer,sizeof(strBuffer)-1,strOutputString,vlArgs);
	//vsprintf(strBuffer,strOutputString,vlArgs);
	va_end(vlArgs);
	OutputDebugString(strBuffer);
}
#endif
#ifdef DEBUG_ERROR
void DebugError(const char * strOutputString,...)
{
	char strBuffer[4096]={0};
	va_list vlArgs;
	va_start(vlArgs,strOutputString);
	_vsnprintf(strBuffer,sizeof(strBuffer)-1,strOutputString,vlArgs);
	//vsprintf(strBuffer,strOutputString,vlArgs);
	va_end(vlArgs);
	OutputDebugString(strBuffer);
}
#endif
#endif*/