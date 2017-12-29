#pragma once

#include <tchar.h>
#include <string>

#ifdef MOTIONSENSORFUNC_EXPORTS
#define MOTIONSENSORFUNC_API __declspec(dllexport)
#else
#define MOTIONSENSORFUNC_API __declspec(dllimport) 
#endif

using tstring = std::basic_string<TCHAR>;

extern "C" {
	// Returns a + b
	MOTIONSENSORFUNC_API bool Initialize();
	MOTIONSENSORFUNC_API void Finilize();
	MOTIONSENSORFUNC_API bool Start(char* SpindleComName, char* MotionComName);
	MOTIONSENSORFUNC_API void Stop();
	MOTIONSENSORFUNC_API int  GetData(double* Data, int DataSize);
	MOTIONSENSORFUNC_API int  GetError();
}

