#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#define WIN
#else
#define LIN
#endif

#ifdef WIN
#ifdef _DEBUG
#define ant_new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define ant_new new
#endif
#else
#define ant_new new
#endif

#ifdef WIN
#else
#define GetLastError() errno
#endif
