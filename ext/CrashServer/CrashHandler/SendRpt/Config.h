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

#pragma once

#pragma warning(push)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(pop)
#include <atlstr.h>
#include <vector>
#include <map>

struct Config
{
    CStringW Prefix;
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
    BOOL     SendAdditionalDataWithoutApproval;
    DWORD    FullDumpType;
    CStringW LangFilePath;
    CStringW SendRptPath;
    CStringW DbgHelpPath;
    CStringW ProcessName;
    std::vector<std::pair<CStringW, CStringW> > FilesToAttach;
    std::map<CStringW, CStringW> UserInfo;
    CStringW CustomInfo;

    Config()
    {
        V[0] = V[1] = V[2] = V[3] = 0;
        Hotfix = 0;
        ServiceMode = FALSE;
        LeaveDumpFilesInTempFolder = FALSE;
        OpenProblemInBrowser = TRUE;
        UseWER = FALSE;
        SubmitterID = 0;
        SendAdditionalDataWithoutApproval = FALSE;
        FullDumpType = 0;
    }
};

struct Params
{
    HANDLE Process;
    DWORD  ProcessId;
    MINIDUMP_EXCEPTION_INFORMATION ExceptInfo;
    BOOL   WasAssert;
    HANDLE ReportReady;
    CStringA Group;
};
