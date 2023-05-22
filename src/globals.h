#pragma once

inline int l_ftoi(float s) { return (int)(s * 1000); }
inline float l_itof(int s) { return (float)(s / 1000.0f); }

#if defined(_WIN32) || defined(WIN32)

//#define UNICODE
#define ISOLATION_AWARE_ENABLED 1
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <litehtml.h>
#include <winhttp.h>
#include <algorithm>

//#include <TxDIB.h>
//#include <cairo.h>
//#include <cairo-win32.h>
//#include <dib.h>

#else

#include <fstream>
#include <string>
#include <cerrno>
#include <clocale>
#include <vector>
#include <iostream>
#include <cstdlib>
#ifdef __APPLE__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <memory.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cairo.h>
#include <litehtml.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#include <cairo-ft.h>
#include <cairomm/context.h>
#include <curl/curl.h>
#include <Poco/URI.h>

#endif

#include <stereokit.h>
#include <stereokit_ui.h>
#include "message.h"
using namespace sk;
