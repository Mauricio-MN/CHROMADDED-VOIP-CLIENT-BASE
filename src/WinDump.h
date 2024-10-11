#ifndef H_WINDUMP
#define H_WINDUMP
#ifdef WIN32
#ifdef NEED_DGB

#include <windows.h>
LONG WINAPI unhandled_handler(struct _EXCEPTION_POINTERS* apExceptionInfo);

#endif
#endif // WIN32
#endif