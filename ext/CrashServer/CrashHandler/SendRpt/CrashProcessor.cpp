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

#include "stdafx.h"
#include "Serializer.h"
#include "DumpWriter.h"
#include "..\..\CommonLibs\Zlib\ZipUnzip.h"
#include "..\..\CommonLibs\Log\log.h"
#include "..\..\CommonLibs\Log\log_media.h"
#include "SendReportDlg.h"
#include "..\DumpUploaderServiceLib\DumpUploaderWebService.h"
#include "..\CrashHandler\CrashHandler.h"
#include "CrashInfo.h"

Config g_Config;
Log g_Log(LogMediaPtr(), _T("sendrpt"));

using namespace std;

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

CStringA GetUTF8(const CStringW& string)
{
    int size = string.GetLength()+1;
    char * buffer = new char[4 * size];

    int len = WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)string, size, buffer, 4*size, 0, NULL);

    CStringA ret;
    if (len != 0)
        ret = CStringA(buffer, len-1);
    delete [] buffer;

    return ret;
}

struct ModuleInfo
{
    CStringW name;
    USHORT V[4];
};

class DumpFilter
{
public:
    DumpFilter(vector<ModuleInfo>& _Modules, DWORD _SaveOnlyThisThreadID = 0)
        : Modules(_Modules), SaveOnlyThisThreadID(_SaveOnlyThisThreadID)
    {
        Callback.CallbackRoutine = _MinidumpCallback;
        Callback.CallbackParam = this;
    }

    operator MINIDUMP_CALLBACK_INFORMATION*() { return &Callback; }

private:
    MINIDUMP_CALLBACK_INFORMATION Callback;
    DWORD SaveOnlyThisThreadID;
    vector<ModuleInfo>& Modules;

    BOOL MinidumpCallback(
        const PMINIDUMP_CALLBACK_INPUT CallbackInput,
        PMINIDUMP_CALLBACK_OUTPUT CallbackOutput)
    {
        switch (CallbackInput->CallbackType)
        {
        case IncludeThreadCallback:
            if (SaveOnlyThisThreadID != 0 && CallbackInput->IncludeThread.ThreadId != SaveOnlyThisThreadID)
                return FALSE;
            break;
        case ThreadCallback:
            if (SaveOnlyThisThreadID != 0 && CallbackInput->Thread.ThreadId != SaveOnlyThisThreadID)
                CallbackOutput->ThreadWriteFlags = 0;
            break;
        case ModuleCallback:
            {
                ModuleInfo mi;
                WCHAR drive[MAX_PATH], dir[MAX_PATH], fname[MAX_PATH], ext[MAX_PATH];
                _wsplitpath_s(CallbackInput->Module.FullPath, drive, dir, fname, ext);
                mi.name = CStringW(fname) + ext;
                mi.name.MakeLower();
                mi.V[0] = HIWORD(CallbackInput->Module.VersionInfo.dwProductVersionMS);
                mi.V[1] = LOWORD(CallbackInput->Module.VersionInfo.dwProductVersionMS);
                mi.V[2] = HIWORD(CallbackInput->Module.VersionInfo.dwProductVersionLS);
                mi.V[3] = LOWORD(CallbackInput->Module.VersionInfo.dwProductVersionLS);
                Modules.push_back(mi);
            }
            break;
        }

        return TRUE;
    }

    static BOOL CALLBACK _MinidumpCallback(
        PVOID CallbackParam,
        const PMINIDUMP_CALLBACK_INPUT CallbackInput,
        PMINIDUMP_CALLBACK_OUTPUT CallbackOutput)
    {
        return static_cast<DumpFilter*>(CallbackParam)->MinidumpCallback(CallbackInput, CallbackOutput);
    }
};

struct Solution
{
    bool askConfirmation;
    ns1__SolutionType type;
    wstring url;
    vector<BYTE> exe;
};

struct Response
{
    ns1__ResponseType type;
    wstring error;
    auto_ptr<Solution> solution;
    std::wstring clientID;
    int problemID;
    int dumpGroupID;
    int dumpID;
    std::wstring urlToProblem;
    ns1__AdditionalInfoType infoType;
    vector<BYTE> infoModule;
    std::wstring infoModuleCfg;

    void Fill(const ns1__Response& response)
    {
        type = response.type;
        g_Log.Debug(_T("There is response type = %d:"), type);
        if (response.error)
        {
            error = *response.error;
            g_Log.Debug(_T("\terror\t\"%ls\""), error.c_str());
        }
        if (response.solution)
        {
            g_Log.Debug(_T("\thas solution"));
            solution.reset(new Solution);
            solution->askConfirmation = response.solution->askConfirmation;
            solution->type = response.solution->type;
            g_Log.Debug(_T("\t\ttype\t%d"), solution->type);
            if (response.solution->url)
            {
                solution->url = *response.solution->url;
                g_Log.Debug(_T("\t\tURL\t%ls"), solution->url.c_str());
            }
            if (response.solution->exe)
            {
                solution->exe.assign(response.solution->exe->__ptr, response.solution->exe->__ptr + response.solution->exe->__size);
                g_Log.Debug(_T("\t\thas EXE, size\t%d"), solution->exe.size());
            }
        }
        if (response.clientID)
        {
            clientID = *response.clientID;
            g_Log.Debug(_T("\tCliendID = \"%ls\""), clientID.c_str());
        }
        problemID = response.problemID;
        dumpGroupID = response.dumpGroupID;
        dumpID = response.dumpID;
        g_Log.Debug(_T("\tproblemID = %d, dumpGroupID = %d, dumpID = %d"), problemID, dumpGroupID, dumpID);
        if (response.urlToProblem)
        {
            urlToProblem = *response.urlToProblem;
            g_Log.Debug(_T("\tURL to problem = \"%ls\""), urlToProblem.c_str());
        }
        else
        {
            urlToProblem.clear();
        }
        infoType = response.infoType;
        if (response.infoModule && response.infoModule->__size > 0)
            infoModule.assign(response.infoModule->__ptr, response.infoModule->__ptr + response.infoModule->__size);
        else
            infoModule.clear();
        if (response.infoModuleCfg)
            infoModuleCfg = *response.infoModuleCfg;
        else
            infoModuleCfg.clear();
    }
};

class CrashProcessor
{
    auto_ptr<CrashInfo> m_CrashInfo;
    CString m_TempFolder;
    CString m_MiniDumpFile;
    CString m_MiniDumpZipFile;
    CString m_FullDumpFile;
    CString m_FullDumpZipFile;
    CString m_InfoDll;
    CString m_InfoFile;
    CString m_Patch;
    DumpWriter m_DumpWriter;
    DumpUploaderWebService m_WebService;
    CWindow* volatile m_pProgressDlg;
    bool m_bStop;

public:
    CrashProcessor()
        : m_bStop(false)
        , m_pProgressDlg(NULL)
        , m_WebService(5*60 /* Parsing a dump may require a lot of time (for example if symbols are uploaded from microsoft) */)
    {
    }

    ~CrashProcessor()
    {
        if (!m_FullDumpFile.IsEmpty())
            DeleteFile(m_FullDumpFile);
        if (!m_MiniDumpFile.IsEmpty())
            DeleteFile(m_MiniDumpFile);
        if (!g_Config.LeaveDumpFilesInTempFolder)
        {
            if (!m_FullDumpZipFile.IsEmpty())
                DeleteFile(m_FullDumpZipFile);
            if (!m_MiniDumpZipFile.IsEmpty())
                DeleteFile(m_MiniDumpZipFile);
            if (!m_TempFolder.IsEmpty())
                RemoveDirectory(m_TempFolder);
        }
    }

    void InitPathes()
    {
        GetTempPath(MAX_PATH, m_TempFolder.GetBuffer(MAX_PATH));
        m_TempFolder.ReleaseBuffer();

        SYSTEMTIME st;
        GetLocalTime(&st);

        CString t;
        t.Format(_T("%s_%d.%d.%d.%d_%04d%02d%02d_%02d%02d%02d"),
            (LPCTSTR)CA2CT(g_Config.Prefix),
            g_Config.V[0], g_Config.V[1], g_Config.V[2], g_Config.V[3],
            (int)st.wYear, (int)st.wMonth, (int)st.wDay, (int)st.wHour, (int)st.wMinute, (int)st.wSecond);

        m_TempFolder += t;
        m_MiniDumpFile = m_TempFolder + _T("\\") + t + _T(".mini.dmp");
        m_MiniDumpZipFile = m_TempFolder + _T("\\mini.zip");
        m_FullDumpFile = m_TempFolder + _T("\\") + t + _T(".full.dmp");
        m_FullDumpZipFile = m_TempFolder + _T("\\full.zip");
        m_InfoDll = m_TempFolder + _T("\\info.dll");
        m_InfoFile = m_TempFolder + _T("\\info.zip");
        m_Patch = m_TempFolder + _T("\\solution.exe");

        if (!CreateDirectory(m_TempFolder, NULL))
            throw std::runtime_error("failed to create temp directory");
    }

    static void FillClientLib(ns1__ClientLib* client)
    {
        client->v1 = 0;
        client->v2 = 0;
        client->v3 = 0;
        client->v4 = 0;
        client->arch =
#ifdef _M_AMD64
            ns1__Architecture__x64
#else
            ns1__Architecture__x32
#endif
            ;

        TCHAR path[MAX_PATH];
        if (!GetModuleFileName(NULL, path, _countof(path)))
            return;

        vector<BYTE> verInfo(GetFileVersionInfoSize(path, NULL));
        VS_FIXEDFILEINFO* lpFileinfo = NULL;
        UINT uLen;
        if (!GetFileVersionInfo(path, 0, static_cast<DWORD>(verInfo.size()), &verInfo[0])
            || !VerQueryValue(&verInfo[0], _T("\\"), (LPVOID*)&lpFileinfo, &uLen))
            return;
        client->v1 = HIWORD(lpFileinfo->dwProductVersionMS);
        client->v2 = LOWORD(lpFileinfo->dwProductVersionMS);
        client->v3 = HIWORD(lpFileinfo->dwProductVersionLS);
        client->v4 = LOWORD(lpFileinfo->dwProductVersionLS);
        g_Log.Info(_T("Client lib %d.%d.%d.%d"), client->v1, client->v2, client->v3, client->v4);
    }

    static void FillApp(ns1__Application* app)
    {
        app->applicationGUID = g_Config.ApplicationGUID;
        app->v1 = g_Config.V[0];
        app->v2 = g_Config.V[1];
        app->v3 = g_Config.V[2];
        app->v4 = g_Config.V[3];
        app->hotfix = g_Config.Hotfix;
        g_Log.Info(_T("App %d.%d.%d.%d %ls"), app->v1, app->v2, app->v3, app->v4, app->applicationGUID.c_str());
    }

    static int GetUserGuid()
    {
        int nUserGuid = 12345;
        HKEY hCryptography;
#if defined (_WIN64)
        DWORD param = 0;
#else
        DWORD param = IsWow64() ? KEY_WOW64_64KEY : 0;
#endif
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Cryptography"), 0, KEY_READ|param, &hCryptography))
        {
            TCHAR guid[1024];
            DWORD size = sizeof(guid);
            if (ERROR_SUCCESS == RegQueryValueEx(hCryptography, _T("MachineGuid"), NULL, NULL, (LPBYTE)guid, &size))
            {
                DWORD D1 = 0, D2 = 0, D3 = 0, D4 = 0, D5 = 0, D6 = 0;
                _stscanf_s(guid, _T("%08x-%04x-%04x-%04x-%04x%08x"), &D1, &D2, &D3, &D4, &D5, &D6);
                DWORD DD1 = MAKELONG(D2, D3);
                DWORD DD2 = MAKELONG(D4, D5);
                nUserGuid = D1 ^ MAKELONG(D3, D2) ^ MAKELONG(D5, D4) ^ D6;
            }
            RegCloseKey(hCryptography);
        }
        g_Log.Info(_T("User ID 0x%08X"), nUserGuid);
        return nUserGuid;
    }

    Response Hello(const vector<ModuleInfo>& modules)
    {
        g_Log.Info(_T("Sending Hello..."));

        Response result;

        wstring mainModule = g_Config.ProcessName;

        _ns1__Hello* pRequest = soap_new__ns1__Hello(&m_WebService, 1);
        pRequest->client = soap_new_ns1__ClientLib(&m_WebService, 1);
        pRequest->app = soap_new_ns1__Application(&m_WebService, 1);
        pRequest->mainModule = &mainModule;
        FillClientLib(pRequest->client);
        FillApp(pRequest->app);

        _ns1__HelloResponse* pResponse = soap_new__ns1__HelloResponse(&m_WebService, 1);

        bool bReqOK = m_WebService.Hello(pRequest, pResponse) == SOAP_OK;

        if (bReqOK)
            result.Fill(*pResponse->HelloResult);

        soap_delete__ns1__HelloResponse(&m_WebService, pResponse);
        soap_delete_ns1__Application(&m_WebService, pRequest->app);
        soap_delete_ns1__ClientLib(&m_WebService, pRequest->client);
        soap_delete__ns1__Hello(&m_WebService, pRequest);

        if (!bReqOK)
            throw runtime_error((const char*)CW2A(m_WebService.GetErrorText().c_str()));

        return result;
    }

    Response UploadMiniDump(const CString& dumpFile)
    {
        g_Log.Info(_T("Sending UploadMiniDump..."));

        Response result;

        wstring mainModule = g_Config.ProcessName;

        vector<unsigned char> data = ReadFile(dumpFile);

        _ns1__UploadMiniDump* pRequest = soap_new__ns1__UploadMiniDump(&m_WebService, 1);
        pRequest->client = soap_new_ns1__ClientLib(&m_WebService, 1);
        pRequest->app = soap_new_ns1__Application(&m_WebService, 1);
        pRequest->mainModule = &mainModule;
        FillClientLib(pRequest->client);
        FillApp(pRequest->app);
        pRequest->PCID = GetUserGuid();
        pRequest->submitterID = g_Config.SubmitterID;
        pRequest->dump = soap_new__xop__Include(&m_WebService, 1);
        pRequest->dump->__ptr = &data[0];
        pRequest->dump->__size = static_cast<int>(data.size());
        pRequest->dump->id = NULL;
        pRequest->dump->options = NULL;
        pRequest->dump->type = "binary";

        _ns1__UploadMiniDumpResponse* pResponse = soap_new__ns1__UploadMiniDumpResponse(&m_WebService, 1);

        bool bReqOK = m_WebService.UploadMiniDump(pRequest, pResponse) == SOAP_OK;

        if (bReqOK)
            result.Fill(*pResponse->UploadMiniDumpResult);

        soap_delete__ns1__UploadMiniDumpResponse(&m_WebService, pResponse);
        soap_delete__xop__Include(&m_WebService, pRequest->dump);

        soap_delete_ns1__Application(&m_WebService, pRequest->app);
        soap_delete_ns1__ClientLib(&m_WebService, pRequest->client);
        soap_delete__ns1__UploadMiniDump(&m_WebService, pRequest);

        if (!bReqOK)
            throw runtime_error((const char*)CW2A(m_WebService.GetErrorText().c_str()));

        if (m_bStop)
            throw std::runtime_error("canceled");

        return result;
    }

    Response SendAdditionalInfoUploadRejected(const int miniDumpId)
    {
        g_Log.Info(_T("Sending SendAdditionalInfoUploadRejected..."));

        return UploadAdditionalInfo(miniDumpId, NULL, ns1__AdditionalInfoType__None);
    }

    Response UploadAdditionalInfo(const int miniDumpId, const CString& addInfoFile, const bool isFullDump)
    {
        g_Log.Info(_T("Sending UploadAdditionalInfo..."));

        vector<unsigned char> data = ReadFile(addInfoFile);

        return UploadAdditionalInfo(miniDumpId, &data, isFullDump ? ns1__AdditionalInfoType__FullDump : ns1__AdditionalInfoType__Info);
    }

    static vector<unsigned char> ReadFile(const CString& path)
    {
        vector<unsigned char> data;
        CAtlFile file(CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
        if (file == INVALID_HANDLE_VALUE)
            throw runtime_error(std::string("failed to open file ") + (const char*)CT2A(path));
        ULONGLONG fileSize;
        if (FAILED(file.GetSize(fileSize)))
            throw runtime_error(std::string("failed to get file size ") + (const char*)CT2A(path));

        if (fileSize != 0)
        {
            data.resize((size_t)fileSize);
            if (FAILED(file.Read(&data[0], static_cast<DWORD>(fileSize))))
                throw runtime_error(std::string("failed to read file ") + (const char*)CT2A(path));
        }

        return data;
    }

    struct ProgressNotifier
    {
        size_t m_sent;
        size_t m_total;
        CrashProcessor& m_crashProcessor;
        ProgressNotifier(size_t total, CrashProcessor& crashProcessor) : m_sent(0), m_total(total), m_crashProcessor(crashProcessor) {}

        static void ProgressCallback(BOOL send, SIZE_T bytesCount, LPVOID context)
        {
            static_cast<ProgressNotifier*>(context)->ProgressCallback(send, bytesCount);
        }
        void ProgressCallback(BOOL send, SIZE_T bytesCount)
        {
            if (!send)
                return;
            m_sent += bytesCount;
            if (m_sent > m_total)
                m_sent = m_total;

            if (m_crashProcessor.m_pProgressDlg)
                m_crashProcessor.m_pProgressDlg->PostMessage(WM_USER, m_total, m_sent);
        }
    };

    Response UploadAdditionalInfo(const int miniDumpId, vector<unsigned char> *data, const ns1__AdditionalInfoType infoType)
    {
        Response result;

        wstring mainModule = g_Config.ProcessName;

        _ns1__UploadAdditionalInfo* pRequest = soap_new__ns1__UploadAdditionalInfo(&m_WebService, 1);
        pRequest->client = soap_new_ns1__ClientLib(&m_WebService, 1);
        pRequest->app = soap_new_ns1__Application(&m_WebService, 1);
        pRequest->mainModule = &mainModule;
        FillClientLib(pRequest->client);
        FillApp(pRequest->app);
        pRequest->miniDumpID = miniDumpId;
        pRequest->info = soap_new__xop__Include(&m_WebService, 1);
        if (data && !data->empty())
        {
            pRequest->info->__ptr = &(*data->begin());
            pRequest->info->__size = static_cast<int>(data->size());
        }
        else
        {
            pRequest->info->__ptr = NULL;
            pRequest->info->__size = 0;
        }
        pRequest->info->id = NULL;
        pRequest->info->options = NULL;
        pRequest->info->type = "binary";
        pRequest->infoType = infoType;

        _ns1__UploadAdditionalInfoResponse* pResponse = soap_new__ns1__UploadAdditionalInfoResponse(&m_WebService, 1);
        auto_ptr<ProgressNotifier> pProgress;
        if (data)
        {
            pProgress.reset(new ProgressNotifier(data->size(), *this));
            m_WebService.SetProgressCallback(ProgressNotifier::ProgressCallback, pProgress.get());
        }
        bool bReqOK = m_WebService.UploadAdditionalInfo(pRequest, pResponse) == SOAP_OK;
        if (data)
            m_WebService.SetProgressCallback(NULL, NULL);

        if (bReqOK)
            result.Fill(*pResponse->UploadAdditionalInfoResult);

        soap_delete__ns1__UploadAdditionalInfoResponse(&m_WebService, pResponse);
        if (data)
            soap_delete__xop__Include(&m_WebService, pRequest->info);
        soap_delete_ns1__Application(&m_WebService, pRequest->app);
        soap_delete_ns1__ClientLib(&m_WebService, pRequest->client);
        soap_delete__ns1__UploadAdditionalInfo(&m_WebService, pRequest);

        if (!bReqOK)
            throw runtime_error((const char*)CW2A(m_WebService.GetErrorText().c_str()));

        if (m_bStop)
            throw std::runtime_error("canceled");

        return result;
    }

    void PrepareDump(
        const CString& dumpFile,
        const CString& zipFile,
        HANDLE hProcess,
        DWORD dwProcessId,
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo,
        MINIDUMP_TYPE type,
        MINIDUMP_CALLBACK_INFORMATION* pCallback,
        bool attachFiles)
    {
        if (m_bStop)
            throw std::runtime_error("canceled");

        g_Log.Info(_T("Writing dump (%s)..."), static_cast<LPCTSTR>(dumpFile));
        if (!m_DumpWriter.WriteMiniDump(hProcess, dwProcessId, pExceptInfo, dumpFile, type, pCallback))
            throw std::runtime_error("dump was not created");
        g_Log.Info(_T("Dump ready."));

        if (m_bStop)
            throw std::runtime_error("canceled");

        g_Log.Info(_T("Zipping dump (%s)..."), static_cast<LPCTSTR>(zipFile));
        {
            Zip zip(zipFile);
            zip.AddFile(dumpFile, dumpFile.Mid(dumpFile.ReverseFind(_T('\\')) + 1));
            if (attachFiles)
            {
                CString crashInfoFile = m_TempFolder + _T("\\crashinfo.xml");
                if (m_CrashInfo->GetCrashInfoFile(crashInfoFile))
                    g_Config.FilesToAttach.push_back(std::make_pair<CStringW, CStringW>(crashInfoFile, L"crashinfo.xml"));

                CString crashUserInfoFile = m_TempFolder + _T("\\crashuserinfo.xml");
                if (g_Config.UserInfo.size())
                {
                    FILE* f = NULL;
                    if (0 == _tfopen_s(&f, crashUserInfoFile, _T("wt")))
                    {
                        fprintf_s(f, "<UserInfo>\n");
						for (std::vector<std::pair<CStringW, CStringW>>::iterator it = g_Config.UserInfo.begin(), end = g_Config.UserInfo.end(); it != end; ++it)
                        {
                            g_Log.Info(_T("Adding UserInfo \"%ws\" as \"%ws\"..."), static_cast<LPCWSTR>(it->first), static_cast<LPCWSTR>(it->second));
                            fprintf_s(f,
                                "<%s>%s</%s>\n",
                                static_cast<LPCSTR>(GetUTF8(it->first)),
                                static_cast<LPCSTR>(GetUTF8(it->second)),
                                static_cast<LPCSTR>(GetUTF8(it->first)));
                        }
                        fprintf_s(f, "</UserInfo>");
                        fclose(f);
                        g_Config.FilesToAttach.push_back(std::make_pair<CStringW, CStringW>(crashUserInfoFile, L"crashuserinfo.xml"));
                    }
                }

                WIN32_FIND_DATAW ff;
                FindClose(FindFirstFileW(dumpFile, &ff));
                __int64 attachedSizeLimit = max(1024*1024I64, (static_cast<__int64>(ff.nFileSizeHigh) << 32) | ff.nFileSizeLow);

                g_Log.Info(_T("Adding %d attaches..."), g_Config.FilesToAttach.size());
				for (std::vector<std::pair<CStringW, CStringW> >::iterator it = g_Config.FilesToAttach.begin(), end = g_Config.FilesToAttach.end(); it != end; ++it)
                {
                    g_Log.Info(_T("Adding \"%ls\" as \"%ls\"..."), static_cast<LPCWSTR>(it->first), static_cast<LPCWSTR>(it->second));
                    WIN32_FIND_DATAW ff;
                    HANDLE hFind = FindFirstFileW(it->first, &ff);
                    if (hFind == INVALID_HANDLE_VALUE)
                    {
                        g_Log.Warning(_T("File \"%ls\" not found..."), static_cast<LPCWSTR>(it->first));
                        continue;
                    }
                    FindClose(hFind);
                    __int64 size = (static_cast<__int64>(ff.nFileSizeHigh) << 32) | ff.nFileSizeLow;
                    if (size > attachedSizeLimit)
                    {
                        g_Log.Warning(_T("File \"%ls\" not skipped by size..."), static_cast<LPCWSTR>(it->first));
                        continue;
                    }
                    attachedSizeLimit -= size;

                    zip.AddFile(it->first, it->second);
                    g_Log.Info(_T("Done."));
                }
                DeleteFile(crashInfoFile);
            }
        }
        g_Log.Info(_T("Zipping done."));

        if (m_bStop)
            throw std::runtime_error("canceled");
    }

    void PrepareMiniDump(
        HANDLE hProcess,
        DWORD dwProcessId,
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo,
        vector<ModuleInfo>& modules)
    {
        g_Log.Info(_T("Prepare mini dump..."));

        DumpFilter dumpFilter(modules, pExceptInfo ? pExceptInfo->ThreadId : 0);
        PrepareDump(m_MiniDumpFile, m_MiniDumpZipFile, hProcess, dwProcessId, pExceptInfo, MiniDumpFilterMemory, dumpFilter, false);
    }

    void PrepareFullDump(
        HANDLE hProcess,
        DWORD dwProcessId,
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo)
    {
        g_Log.Info(_T("Prepare full dump..."));

        PrepareDump(m_FullDumpFile, m_FullDumpZipFile, hProcess, dwProcessId, pExceptInfo, MiniDumpWithFullMemory, NULL, true);
    }

    typedef bool (WINAPI *pfnCollectInfo)(LPCWSTR file, LPCWSTR cfg, HANDLE Process, DWORD ProcessId);
    void PrepareAdditionalInfo(const Response& response, HANDLE Process, DWORD ProcessId)
    {
        g_Log.Info(_T("Prepare additional info..."));
        HMODULE hInfoDll = NULL;
        try
        {
            CAtlFile hFile(CreateFile(m_InfoDll, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
            if (hFile == INVALID_HANDLE_VALUE)
                throw runtime_error("failed to create info.dll file");
            if (FAILED(hFile.Write(&response.infoModule[0], static_cast<DWORD>(response.infoModule.size()))))
                throw runtime_error("failed to write info.dll file");
            hFile.Close();

            hInfoDll = LoadLibrary(m_InfoDll);
            if (!hInfoDll)
                throw runtime_error("failed to load info.dll");

            pfnCollectInfo CollectInfo = (pfnCollectInfo) GetProcAddress(hInfoDll, "CollectInfo");
            if (!CollectInfo)
                throw runtime_error("failed to get CollectInfo from info.dll");

            if (!CollectInfo(m_InfoFile, response.infoModuleCfg.c_str(), Process, ProcessId))
                throw runtime_error("CollectInfo failed");

            FreeLibrary(hInfoDll);
            DeleteFile(m_InfoDll);
        }
        catch (...)
        {
            if (hInfoDll)
                FreeLibrary(hInfoDll);
            DeleteFile(m_InfoDll);
            throw;
        }
    }

    static CAtlStringW ToString(int n)
    {
        CAtlStringW result;
        result.Format(L"%i", n);
        return result;
    }

    void ProcessSolution(const Solution& solution, const std::wstring& clientID, int problemID, int dumpGroupID, int dumpID)
    {
        g_Log.Info(_T("Process solution..."));
        switch (solution.type)
        {
        case ns1__SolutionType__Url:
            if (!solution.askConfirmation || IDOK == CSolutionDlg(g_Config.AppName, g_Config.Company, IDS_SOLUTION_URL).DoModal())
            {
                CAtlStringW url = solution.url.c_str();
                url.Replace(L"{ClientID}", clientID.c_str());
                url.Replace(L"{ProblemID}", ToString(problemID));
                url.Replace(L"{DumpGroupID}", ToString(dumpGroupID));
                url.Replace(L"{DumpID}", ToString(dumpID));
                ShellExecute(NULL, _T("open"), CW2CT(url), NULL, NULL, SW_SHOWNORMAL);
            }
            break;
        case ns1__SolutionType__Exe:
            if (!solution.askConfirmation || IDOK == CSolutionDlg(g_Config.AppName, g_Config.Company, IDS_SOLUTION_EXE).DoModal())
            {
                CAtlFile hFile(CreateFile(m_Patch, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
                if (hFile == INVALID_HANDLE_VALUE)
                    throw runtime_error("failed to create solution.exe file");
                if (FAILED(hFile.Write(&solution.exe[0], static_cast<DWORD>(solution.exe.size()))))
                    throw runtime_error("failed to write solution.exe file");
                hFile.Close();

                STARTUPINFO si = {};
                si.cb = sizeof(si);
                PROCESS_INFORMATION pi = {};
                if (!CreateProcess(NULL, m_Patch.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                    throw runtime_error("failed to start solution.exe");
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
            break;
        default:
            throw runtime_error("Unknown SolutionType");
        }
    }

    void SendReportDlgThread()
    {
        CSendReportDlg dlg(g_Config.AppName, g_Config.Company);
        assert(!m_pProgressDlg);
        m_pProgressDlg = &dlg;
        if (dlg.DoModal() == IDCANCEL)
            m_bStop = true;
        m_pProgressDlg = NULL;
    }

    static DWORD WINAPI _SendReportDlgThread(LPVOID param)
    {
        static_cast<CrashProcessor*>(param)->SendReportDlgThread();
        return 0;
    }

    void SendAssertReportDlgThread()
    {
        CSendAssertReportDlg dlg(g_Config.AppName, g_Config.Company);
        assert(!m_pProgressDlg);
        m_pProgressDlg = &dlg;
        if (dlg.DoModal() == IDCANCEL)
            m_bStop = true;
        m_pProgressDlg = NULL;
    }

    static DWORD WINAPI _SendAssertReportDlgThread(LPVOID param)
    {
        static_cast<CrashProcessor*>(param)->SendAssertReportDlgThread();
        return 0;
    }

    void SendFullDumpDlgThread()
    {
        CSendFullDumpDlg dlg(g_Config.AppName, g_Config.Company);
        assert(!m_pProgressDlg);
        m_pProgressDlg = &dlg;
        if (dlg.DoModal() == IDCANCEL)
            m_bStop = true;
        m_pProgressDlg = NULL;
    }

    static DWORD WINAPI _SendFullDumpDlgThread(LPVOID param)
    {
        static_cast<CrashProcessor*>(param)->SendFullDumpDlgThread();
        return 0;
    }

    void CloseProgress(HANDLE hThread, bool bCancel = true)
    {
        if (!hThread)
            return;
        Sleep(100); // It could be that m_pProgressDlg->DoModal not yet created a window
        if (m_pProgressDlg)
            m_pProgressDlg->PostMessage(WM_CLOSE);
        WaitForSingleObject(hThread, INFINITE);
        if (!bCancel)
            m_bStop = false;
    }

    bool Process(HANDLE hProcess, DWORD dwProcessId, MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo, bool wasAssert, HANDLE hReportReady)
    {
        m_DumpWriter.Init();

        // we need to get CrashInfo before writing the dumps, since dumps writing will change WorkingSet
        m_CrashInfo.reset(new CrashInfo(hProcess));

        InitPathes();

        HANDLE hThreadTemp = NULL;
        if (!g_Config.ServiceMode)
        {
            DWORD t;
            hThreadTemp = CreateThread(NULL, 0, wasAssert ? _SendAssertReportDlgThread : _SendReportDlgThread, this, 0, &t);
            if (!hThreadTemp)
                throw runtime_error("Failed to create thread");
        }
        CHandle hShowProgressThread(hThreadTemp);
        wstring urlToProblem;
        std::wstring clientID;
        int problemID = 0;
        int dumpGroupID = 0;
        int dumpID = 0;

#if 0
        // This code is for test purposes
        {
            CloseProgress(hShowProgressThread, false);
            DWORD t;
            CHandle hThreadTemp2(CreateThread(NULL, 0, _SendFullDumpDlgThread, this, 0, &t));
            if (!hThreadTemp2)
                throw runtime_error("Failed to create thread");
            hShowProgressThread = hThreadTemp2;
            Sleep(1000); // Give time to draw a dialog before writing a dump (it will freeze a process and a dialog)
            for (int i = 0; m_pProgressDlg && i < 100; ++i)
            {
                m_pProgressDlg->PostMessage(WM_USER, 100, i);
                Sleep(30);
            }
            throw runtime_error("end");
        }
#endif

        try
        {
            vector<ModuleInfo> modules;
            PrepareMiniDump(hProcess, dwProcessId, pExceptInfo, modules);
            if (g_Config.ServiceMode)
            {
                PrepareFullDump(hProcess, dwProcessId, pExceptInfo);
                TerminateProcess(hProcess, E_FAIL); // It is necessary for DUMPPARSER to terminate app, because it should process our dump, and it could not do it since it crashes.
                CloseHandle(hProcess);
                hProcess = NULL;
            }

            bool additionalInfoAlreadyApproved = false;

            Response response = Hello(modules);
            while (1)
            {
                switch (response.type)
                {
                case ns1__ResponseType__HaveSolution:
                    CloseProgress(hShowProgressThread);
                    if (response.solution.get())
                        ProcessSolution(*response.solution, clientID, problemID, dumpGroupID, dumpID);
                    goto finish;
                case ns1__ResponseType__NeedMiniDump:
                    response = UploadMiniDump(m_MiniDumpZipFile);
                    urlToProblem = response.urlToProblem;
                    clientID = response.clientID;
                    problemID = response.problemID;
                    dumpGroupID = response.dumpGroupID;
                    dumpID = response.dumpID;
                    continue;
                case ns1__ResponseType__NeedMoreInfo:
                    {
                        CloseProgress(hShowProgressThread, false);

                        if (!g_Config.ServiceMode)
                        {
                            if (!additionalInfoAlreadyApproved && CAskSendFullDumpDlg(g_Config.AppName, g_Config.Company, g_Config.PrivacyPolicyUrl).DoModal() == IDCANCEL)
                            {
                                response = SendAdditionalInfoUploadRejected(response.dumpID);
                                continue;
                            }
                            additionalInfoAlreadyApproved = true;

                            DWORD t;
                            CHandle hThreadTemp2(CreateThread(NULL, 0, _SendFullDumpDlgThread, this, 0, &t));
                            if (!hThreadTemp2)
                                throw runtime_error("Failed to create thread");
                            hShowProgressThread = hThreadTemp2;
                            Sleep(100); // Give time to draw a dialog before writing a dump (it will freeze a process and a dialog)
                        }

                        if (response.infoType == ns1__AdditionalInfoType__FullDump)
                        {
                            if (!g_Config.ServiceMode)
                            {
                                PrepareFullDump(hProcess, dwProcessId, pExceptInfo);
                                CloseHandle(hProcess);
                                hProcess = NULL;
                            }
                            SetEvent(hReportReady);
                            response = UploadAdditionalInfo(response.dumpID, m_FullDumpZipFile, true);
                        }
                        else if (response.infoType == ns1__AdditionalInfoType__Info)
                        {
                            PrepareAdditionalInfo(response, hProcess, dwProcessId);
                            response = UploadAdditionalInfo(response.dumpID, m_InfoFile, false);
                        }
                        else
                        {
                            throw runtime_error("Unknown AdditionalInfoType");
                        }
                    }
                    continue;
                case ns1__ResponseType__Error:
                    throw runtime_error((const char*)CW2A(response.error.c_str()));
                case ns1__ResponseType__Stop:
                default:
                    goto finish;
                }
            }
finish:
            if (!g_Config.ServiceMode && !urlToProblem.empty() && g_Config.OpenProblemInBrowser)
                ShellExecute(NULL, _T("open"), CW2CT(urlToProblem.c_str()), NULL, NULL, SW_SHOWNORMAL);
        }
        catch (...)
        {
            CloseProgress(hShowProgressThread);
            throw;
        }

        CloseProgress(hShowProgressThread);

        return true;
    }
};

DWORD SendReportImpl(HANDLE hProcess, DWORD dwProcessId, MINIDUMP_EXCEPTION_INFORMATION* mei, bool wasAssert, HANDLE hReportReady)
{
    try
    {
        g_Log.Info(_T("SENDRPT was invoked to send report, PID 0x%x"), dwProcessId);
        CrashProcessor().Process(hProcess, dwProcessId, mei, wasAssert, hReportReady);
    }
    catch (std::exception& ex)
    {
        g_Log.Error(_T("Problem occured: %hs"), ex.what());
        OutputDebugStringA(ex.what());
//#ifdef _DEBUG
        ::MessageBoxA(0, ex.what(), "SendRpt: Error", MB_ICONERROR);
//#endif
        return FALSE;
    }
    return TRUE;
}

int __cdecl SendReport(int argc, wchar_t* argv[])
{
    CRegKey reg;
    if (ERROR_SUCCESS == reg.Open(HKEY_LOCAL_MACHINE, _T("Software\\Idol Software\\DumpUploader"), KEY_READ))
    {
        DWORD attachDebugger = FALSE;
        reg.QueryDWORDValue(_T("AttachDebugger"), attachDebugger);
        if (attachDebugger != FALSE)
        {
            if (IDYES == ::MessageBoxA(0, "Do you want to debug?\n\nYour should attach debugger to sendrpt.exe before choosing Yes.", "SendRpt: SendReport", MB_ICONWARNING | MB_YESNO))
                DebugBreak();
        }

        DWORD traceEnable = FALSE;
        if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("TraceEnable"), traceEnable) && traceEnable != FALSE)
        {
            CString str;
            ULONG size = 1000;
            if (ERROR_SUCCESS == reg.QueryStringValue(_T("TraceFolder"), str.GetBuffer(size), &size))
            {
                str.ReleaseBuffer(size-1);
                InitializeLog();
                g_Log.SetParams(LogMediaPtr(new FileMedia(str + _T("\\sendrpt.log"), true, false)));
            }
        }
    }

    Params params;

    Serializer ser(argv[1]);
    ser << params << g_Config;
    params.ExceptInfo.ClientPointers = TRUE;

    return SendReportImpl(params.Process, params.ProcessId, &params.ExceptInfo, !!params.WasAssert, params.ReportReady);
}