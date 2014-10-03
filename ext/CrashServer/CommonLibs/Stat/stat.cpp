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

#include "stat.h"
#include <atlutil.h>

namespace wininet
{
    class Error: public std::runtime_error
    {
        static std::string GetError(const char* text, DWORD err);
    public:
        Error(const char* text, DWORD err) : std::runtime_error(GetError(text, err)) {}
    };

    class Handle
    {
        HINTERNET m_handle;
    public:
        Handle(HINTERNET handle = NULL) : m_handle(handle) {}
        ~Handle() { if (m_handle) InternetCloseHandle(m_handle); }
        operator HINTERNET() { return m_handle; }
        operator bool() { return m_handle != NULL; }
    };

    std::vector<BYTE> ReadFile(HINTERNET hRequest);

    std::string EscapeString(const std::string& text);
}

using namespace wininet;

std::string wininet::Error::GetError(const char* text, DWORD err)
{
    char buf[1024];
    sprintf_s(buf, "%s, error %d", text, err);
    return buf;
}

std::vector<BYTE> wininet::ReadFile(HINTERNET hRequest)
{
    std::vector<BYTE> result;
    DWORD bytesAvailable;
    while (InternetQueryDataAvailable(hRequest, &bytesAvailable, 0, 0))
    {
        size_t cur = result.size();
        result.resize(cur + bytesAvailable);
        DWORD bytesRead = 0;
        bool res = !!InternetReadFile(hRequest, result.data() + cur, bytesAvailable, &bytesRead);
        result.resize(cur + bytesRead);
        if (!res || bytesRead == 0)
            break;
    }
    return result;
}

std::string wininet::EscapeString(const std::string& text)
{
    std::string result;
    result.resize(text.size() * 3);
    DWORD size = 0;
    if (!AtlEscapeUrl(text.c_str(), &result.front(), &size, (DWORD)result.size(), ATL_URL_ENCODE_PERCENT))
    {
        result.resize(size * 3);
        if (!AtlEscapeUrl(text.c_str(), &result.front(), &size, (DWORD)result.size(), ATL_URL_ENCODE_PERCENT))
            throw Error("AtlEscapeUrl failed", 0);
    }
    result.resize(size - 1);
    return result;
}

std::vector<BYTE> statistics::HttpPost(LPCTSTR agent, const CUrl& url, LPCTSTR* accept, LPCTSTR hdrs, LPVOID postData, DWORD postDataSize, DWORD& responseCode)
{
    Handle hSession(InternetOpen(agent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0));
    if (!hSession)
        throw Error("InternetOpen failed", GetLastError());

    Handle hConnect(InternetConnect(hSession, url.GetHostName(), INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1));
    if (!hConnect)
        throw Error("InternetConnect failed", GetLastError());

    Handle hRequest(HttpOpenRequest(hConnect, _T("POST"), url.GetUrlPath(), NULL, NULL, accept, 0, INTERNET_NO_CALLBACK));
    if (!hRequest)
        throw Error("HttpOpenRequest failed", GetLastError());

    if (!HttpSendRequest(hRequest, hdrs, -1, postData, postDataSize))
        throw Error("HttpSendRequest failed", GetLastError());

    DWORD size = sizeof(responseCode);
    HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &responseCode, &size, NULL);
    return ReadFile(hRequest);
}

std::vector<BYTE> statistics::HttpPostXWwwFormUrlencoded(LPCTSTR agent, const CUrl& url, LPCTSTR* accept, const Params& query, DWORD& responseCode)
{
    LPCTSTR hdrs = _T("Content-Type: application/x-www-form-urlencoded");

    std::string postData;
    for each (auto& var in query)
    {
        if (!postData.empty())
            postData += "&";
        postData += EscapeString(var.first) + "=" + EscapeString(var.second);
    }

    return HttpPost(agent, url, accept, hdrs, (LPVOID)postData.c_str(), (DWORD)postData.size(), responseCode);
}

namespace statistics
{
    BOOL IsWow64()
    {
        typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
        LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;

        BOOL bIsWow64 = FALSE;

        HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));
        if (NULL != hKernel32)
            fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(hKernel32,"IsWow64Process");

        if (NULL != fnIsWow64Process)
            fnIsWow64Process(GetCurrentProcess(), &bIsWow64);

        return bIsWow64;
    }

    std::string GetMachineGuid()
    {
        std::string result;
        HKEY hCryptography;
#if defined (_WIN64)
        DWORD param = 0;
#else
        DWORD param = IsWow64() ? KEY_WOW64_64KEY : 0;
#endif
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Cryptography"), 0, KEY_READ|param, &hCryptography))
        {
            result.resize(1024);
            DWORD size = (DWORD)result.size();
            if (ERROR_SUCCESS == RegQueryValueExA(hCryptography, "MachineGuid", NULL, NULL, (LPBYTE)&result.front(), &size))
                result.resize(size);
            else
                result = "unknown";
            RegCloseKey(hCryptography);
        }
        return result;
    }

    std::string GetCurrentProcessVersion()
    {
        TCHAR path[MAX_PATH];
        if (!GetModuleFileName(NULL, path, _countof(path)))
            return "0.0.0.0";

        std::vector<BYTE> verInfo(GetFileVersionInfoSize(path, NULL));
        VS_FIXEDFILEINFO* lpFileinfo = NULL;
        UINT uLen;
        if (verInfo.empty()
            || !GetFileVersionInfo(path, 0, static_cast<DWORD>(verInfo.size()), &verInfo[0])
            || !VerQueryValue(&verInfo[0], _T("\\"), (LPVOID*)&lpFileinfo, &uLen))
            return "0.0.0.0";

        char version[128];
        sprintf_s(version, "%d.%d.%d.%d",
            HIWORD(lpFileinfo->dwProductVersionMS),
            LOWORD(lpFileinfo->dwProductVersionMS),
            HIWORD(lpFileinfo->dwProductVersionLS),
            LOWORD(lpFileinfo->dwProductVersionLS));
        return version;
    }
}

bool statistics::SendExceptionToGoogleAnalytics(LPCSTR tid, const std::string& cid, const std::string& appName, const std::string& appVersion, const std::string& exceptionDescr, bool exceptionFatal)
{
    try
    {
        CUrl url;
#ifdef _DEBUG
        url.CrackUrl(_T("http://httpbin.org/post"));
#else
        url.CrackUrl(_T("http://www.google-analytics.com/collect"));
#endif
        LPCTSTR accept[]= { _T("*/*"), NULL };
        Params query;
        query.emplace_back(Param("v", "1"));
        query.emplace_back(Param("tid", tid));
        query.emplace_back(Param("cid", cid));
        query.emplace_back(Param("t", "exception"));
        query.emplace_back(Param("an", appName));
        query.emplace_back(Param("av", appVersion));
        query.emplace_back(Param("exd", exceptionDescr));
        query.emplace_back(Param("exf", exceptionFatal ? "1" : "0"));

        DWORD responseCode = 0;
        std::vector<BYTE> data = HttpPostXWwwFormUrlencoded(_T("MyAgent"), url, accept, query, responseCode);
#ifdef _DEBUG
        printf("HTTP %d\n", responseCode);
        data.push_back(0);
        printf("%s", (LPCSTR)data.data());
#endif
        return true;
    }
    catch (std::exception& ex)
    {
        UNREFERENCED_PARAMETER(ex);
#ifdef _DEBUG
        printf("Error: %s\n", ex.what());
#endif
        return false;
    }
}

bool statistics::SendExceptionToGoogleAnalytics(
    LPCSTR tid,
    const std::string& appName,
    const std::string& exceptionDescr,
    bool exceptionFatal)
{
    return SendExceptionToGoogleAnalytics(tid, GetMachineGuid(), appName, GetCurrentProcessVersion(), exceptionDescr, exceptionFatal);
}
