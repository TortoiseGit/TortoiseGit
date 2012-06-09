#pragma once

class CrashInfo
{
public:
    CrashInfo(HANDLE hProcess);
    bool GetCrashInfoFile(const CString& crashInfoFile) const;

private:
    DWORD     m_guiResources;
    DWORD     m_processHandleCount;
    ULONGLONG m_workingSetSize;
    ULONGLONG m_privateMemSize;
    ULONGLONG m_physicalSize;
    ULONGLONG m_physicalFreeSize;
    CString   m_geoLocation;
};