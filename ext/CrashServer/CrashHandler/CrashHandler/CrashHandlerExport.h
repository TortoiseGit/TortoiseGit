// Copyright 2014 Idol Software, Inc.
//
// This file is part of Doctor Dump SDK.
//
// Doctor Dump SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __CRASH_HANDLER_EXPORT_H__
#define __CRASH_HANDLER_EXPORT_H__

#include "CrashHandler.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL InitCrashHandler(ApplicationInfo* applicationInfo, HandlerSettings* handlerSettings, BOOL ownProcess);
BOOL IsReadyToExit();
void SetCustomInfo(LPCWSTR text);
void AddUserInfoToReport(LPCWSTR key, LPCWSTR value);
void RemoveUserInfoFromReport(LPCWSTR key);
void AddFileToReport(LPCWSTR path, LPCWSTR reportFileName /* = NULL */);
void RemoveFileFromReport(LPCWSTR path);
BOOL GetVersionFromApp(ApplicationInfo* appInfo);
BOOL GetVersionFromFile(LPCWSTR path, ApplicationInfo* appInfo);
LONG SendReport(EXCEPTION_POINTERS* exceptionPointers);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __CRASH_HANDLER_EXPORT_H__