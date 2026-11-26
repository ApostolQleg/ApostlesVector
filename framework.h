#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <commctrl.h>
#include <commdlg.h>
#pragma comment(lib, "comctl32.lib")

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")