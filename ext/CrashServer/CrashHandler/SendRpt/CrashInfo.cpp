#include "StdAfx.h"
#define PSAPI_VERSION 1
#include <Psapi.h>
#include "CrashInfo.h"

#pragma comment(lib, "Psapi.lib")

DWORD _GetProcessHandleCount(HANDLE hProcess)
{
    DWORD handleCount = 0;
    HMODULE hKernel32 = GetModuleHandle(_T("kernel32.dll"));
    if (hKernel32 != NULL)
    {
        typedef BOOL (WINAPI *LPGETPROCESSHANDLECOUNT)(HANDLE, PDWORD);
        LPGETPROCESSHANDLECOUNT pfnGetProcessHandleCount =
            (LPGETPROCESSHANDLECOUNT) GetProcAddress(hKernel32, "GetProcessHandleCount");
        if (pfnGetProcessHandleCount != NULL)
            pfnGetProcessHandleCount(hProcess, &handleCount);
    }
    return handleCount;
}

CString GetGeoLocation()
{
    CString res;

    GEOID geoLocation = GetUserGeoID(GEOCLASS_NATION);
    if (geoLocation != GEOID_NOT_AVAILABLE)
    {
        TCHAR geoInfo[1024] = _T("");
        int n = GetGeoInfo(geoLocation, GEO_RFC1766, geoInfo, _countof(geoInfo), 0);
        if (n != 0)
            res = geoInfo;
    }

    return res;
}

CrashInfo::CrashInfo(HANDLE hProcess)
{
    if (hProcess == NULL)
        return;

    m_guiResources = GetGuiResources(hProcess, GR_GDIOBJECTS);
    m_processHandleCount = _GetProcessHandleCount(hProcess);

    PROCESS_MEMORY_COUNTERS meminfo = {};
    GetProcessMemoryInfo(hProcess, &meminfo, sizeof(meminfo));
    m_workingSetSize = meminfo.WorkingSetSize;
    m_privateMemSize = meminfo.PagefileUsage;

    MEMORYSTATUSEX memstat = {};
    memstat.dwLength = sizeof(memstat);
    GlobalMemoryStatusEx(&memstat);
    m_physicalSize = memstat.ullTotalPhys;
    m_physicalFreeSize = memstat.ullAvailPhys;

    m_geoLocation = GetGeoLocation();
}

bool CrashInfo::GetCrashInfoFile(const CString& crashInfoFile) const
{
    FILE* f = NULL;
    if (0 != _tfopen_s(&f, crashInfoFile, _T("wt")))
        return false;
    fprintf_s(f,
        "<CrashInfo>\n"
        "   <GeoLocation>%ls</GeoLocation>\n"
        "   <GUIResourceCount>%d</GUIResourceCount>\n"
        "   <OpenHandleCount>%d</OpenHandleCount>\n"
        "   <WorkingSetKbytes>%I64u</WorkingSetKbytes>\n"
        "   <PrivateMemKbytes>%I64u</PrivateMemKbytes>\n"
        "   <PhysicalMemoryMbytes>%I64u</PhysicalMemoryMbytes>\n"
        "   <AvailablePhysicalMemoryMbytes>%I64u</AvailablePhysicalMemoryMbytes>\n"
        "</CrashInfo>",
        (LPCWSTR)m_geoLocation,
        m_guiResources,
        m_processHandleCount,
        m_workingSetSize / 1024,
        m_privateMemSize / 1024,
        m_physicalSize / (1024 * 1024),
        m_physicalFreeSize / (1024 * 1024));
    fclose(f);
    return true;
}