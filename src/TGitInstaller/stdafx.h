// stdafx.h : Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

#include "targetver.h"

#define CSTRING_AVAILABLE

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Teile der Windows-Header nicht einbinden.
// Windows-Headerdateien:
#include <windows.h>

// C RunTime-Headerdateien
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // Einige CString-Konstruktoren sind explizit.

#include <atlbase.h>
#include <atlstr.h>

#include <WinInet.h>

#include "UnicodeUtils.h"

#include <memory>
#include <map>
#include <vector>
#include <algorithm>

#include "SmartHandle.h"

#include "DebugOutput.h"
#include "scope_exit_noexcept.h"
