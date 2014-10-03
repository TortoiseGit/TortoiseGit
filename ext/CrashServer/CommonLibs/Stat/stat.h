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

#include <vector>
#include <string>
#include <stdexcept>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib,"Wininet.lib")

namespace ATL
{
    class CUrl; // #include <atlutil.h>
}

namespace statistics
{
    std::vector<BYTE> HttpPost(LPCTSTR agent, const ATL::CUrl& url, LPCTSTR* accept, LPCTSTR hdrs, LPVOID postData, DWORD postDataSize, DWORD& responseCode);

    typedef std::pair<std::string, std::string> Param;
    typedef std::vector<Param> Params;

    std::vector<BYTE> HttpPostXWwwFormUrlencoded(
        LPCTSTR agent,
        const ATL::CUrl& url,
        LPCTSTR* accept,
        const Params& query,
        DWORD& responseCode);

    bool SendExceptionToGoogleAnalytics(
        LPCSTR tid,
        const std::string& cid,
        const std::string& appName,
        const std::string& appVersion,
        const std::string& exceptionDescr,
        bool exceptionFatal);

    bool SendExceptionToGoogleAnalytics(
        LPCSTR tid,
        const std::string& appName,
        const std::string& exceptionDescr,
        bool exceptionFatal);
}

