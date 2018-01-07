#pragma once

#include <tchar.h>
#include <string>


#ifdef NOTIFYDEVICEFUNC_EXPORTS
#define NOTIFYDEVICEFUNC_API __declspec(dllexport)
#else
#define NOTIFYDEVICEFUNC_API __declspec(dllimport) 
#endif

using tstring = std::basic_string<TCHAR>;

extern "C" {
	NOTIFYDEVICEFUNC_API bool Start(char* PortName);
	NOTIFYDEVICEFUNC_API void StopNotify();
	NOTIFYDEVICEFUNC_API void Notify(int time);
}

