// Copyright 2012 Idol Software, Inc.
//
// This file is part of CrashHandler library.
//
// CrashHandler library is free software: you can redistribute it and/or modify
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

#pragma once

#include <dbghelp.h>

struct Config
{
    CStringA Prefix;
    CStringW AppName;
    CStringW Company;
    CStringW PrivacyPolicyUrl;
    USHORT   V[4];
    USHORT   Hotfix;
    CStringW ApplicationGUID;
    BOOL     ServiceMode; // Service is terminated after all dumps has been collected, then send all needed information.
    BOOL     LeaveDumpFilesInTempFolder;
    BOOL     OpenProblemInBrowser;
    BOOL     UseWER;
    DWORD    SubmitterID;
    CStringW ProcessName;
    std::vector<std::pair<CStringW, CStringW> > FilesToAttach;
    std::vector<std::pair<CStringW, CStringW> > UserInfo;
};

struct Params
{
    HANDLE Process;
    DWORD  ProcessId;
    MINIDUMP_EXCEPTION_INFORMATION ExceptInfo;
    BOOL   WasAssert;
    HANDLE ReportReady;
};
