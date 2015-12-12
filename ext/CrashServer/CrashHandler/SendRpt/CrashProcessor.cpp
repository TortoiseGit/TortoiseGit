// Copyright 2015 Idol Software, Inc.
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

#include "stdafx.h"
#include "Serializer.h"
#include "DumpWriter.h"
#include "../../CommonLibs/Zlib/ZipUnzip.h"
#include "../../CommonLibs/Log/log.h"
#include "../../CommonLibs/Log/log_media.h"
#include "../../CommonLibs/Stat/stat.h"
#include "SendReportDlg.h"
#include "DoctorDump.h"
#include "../CrashHandler/CrashHandler.h"
#include "CrashInfo.h"

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

CAtlStringW ToString(int n)
{
    CAtlStringW result;
    result.Format(L"%i", n);
    return result;
}

bool g_cancel = false;

class UI: public doctor_dump::IUploadProgress
{
    CHandle m_progressWindowThread;
    CWindow* volatile m_progressWindow;
    Translator m_translator;
    bool m_additionalInfoAlreadyApproved;
    bool m_additionalInfoRejected;
    bool m_serviceMode;
    CStringW m_privacyPolicyUrl;

public:
    UI(Config& config)
        : m_progressWindow(NULL)
        , m_translator(config.AppName, config.Company, GetModuleHandle(NULL), IDR_TRANSLATE_INI, config.LangFilePath)
        , m_additionalInfoAlreadyApproved(config.SendAdditionalDataWithoutApproval != FALSE)
        , m_additionalInfoRejected(false)
        , m_serviceMode(!!config.ServiceMode)
        , m_privacyPolicyUrl(config.PrivacyPolicyUrl)
    {
    }

    ~UI()
    {
        CloseProgressWindow(true);
    }

    void OnProgress(SIZE_T total, SIZE_T sent) override
    {
        if (m_progressWindow)
            m_progressWindow->PostMessage(WM_USER, total, sent);
    }

    void ShowInitialProgressWindow(bool wasAssert)
    {
        if (m_serviceMode)
            return;

        CloseProgressWindow();
        m_progressWindowThread.Attach(CreateThread(NULL, 0, wasAssert ? SendAssertReportDlgThread : SendReportDlgThread, this, 0, NULL));
        if (!m_progressWindowThread)
            throw runtime_error("Failed to create thread");
    }

    void ShowFullDumpUploadProgressWindow()
    {
        if (m_serviceMode)
            return;

        CloseProgressWindow();
        m_progressWindowThread.Attach(CreateThread(NULL, 0, SendFullDumpDlgThread, this, 0, NULL));
        if (!m_progressWindowThread)
            throw runtime_error("Failed to create thread");
        Sleep(100); // Give time to draw a dialog before writing a dump (it will freeze a dialog for unknown reasons)
    }

    bool AskSendFullDump()
    {
        if (m_serviceMode)
            return true;

        CloseProgressWindow();

        if (!m_additionalInfoAlreadyApproved && IDCANCEL == CAskSendFullDumpDlg(m_translator, m_privacyPolicyUrl).DoModal())
        {
            m_additionalInfoRejected = true;
            return false;
        }

        m_additionalInfoAlreadyApproved = true;

        return true;
    }

    bool IsSendingAdditionalInfoRejected()
    {
        return m_additionalInfoRejected;
    }

    bool AskGetSolution(CSolutionDlg::Type type)
    {
        if (m_serviceMode)
            return true;

        CloseProgressWindow();
        return IDOK == CSolutionDlg(m_translator, type).DoModal();
    }

    void Test()
    {
        ShowInitialProgressWindow(false);
        while (!g_cancel)
            Sleep(10);
        g_cancel = false;

        ShowInitialProgressWindow(true);
        while (!g_cancel)
            Sleep(10);
        g_cancel = false;

        AskSendFullDump();

        ShowFullDumpUploadProgressWindow();
        Sleep(3000);
        for (int i = 0; m_progressWindow && i < 100; ++i)
        {
            m_progressWindow->PostMessage(WM_USER, 100, i);
            Sleep(30);
        }

        AskGetSolution(CSolutionDlg::Read);

        AskGetSolution(CSolutionDlg::Install);
    }

private:
    void SendReportDlg()
    {
        CSendReportDlg dlg(m_translator);
        assert(!m_progressWindow);
        m_progressWindow = &dlg;
        if (dlg.DoModal() == IDCANCEL)
            g_cancel = true;
        m_progressWindow = NULL;
    }

    static DWORD WINAPI SendReportDlgThread(LPVOID param)
    {
        static_cast<UI*>(param)->SendReportDlg();
        return 0;
    }

    void SendAssertReportDlg()
    {
        CSendAssertReportDlg dlg(m_translator);
        assert(!m_progressWindow);
        m_progressWindow = &dlg;
        if (dlg.DoModal() == IDCANCEL)
            g_cancel = true;
        m_progressWindow = NULL;
    }

    static DWORD WINAPI SendAssertReportDlgThread(LPVOID param)
    {
        static_cast<UI*>(param)->SendAssertReportDlg();
        return 0;
    }

    void SendFullDumpDlg()
    {
        CSendFullDumpDlg dlg(m_translator);
        assert(!m_progressWindow);
        m_progressWindow = &dlg;
        if (dlg.DoModal() == IDCANCEL)
            g_cancel = true;
        m_progressWindow = NULL;
    }

    static DWORD WINAPI SendFullDumpDlgThread(LPVOID param)
    {
        static_cast<UI*>(param)->SendFullDumpDlg();
        return 0;
    }

    void CloseProgressWindow(bool cancel = false)
    {
        if (!m_progressWindowThread)
            return;
        Sleep(100); // It could be that m_pProgressDlg->DoModal not yet created a window
        if (m_progressWindow)
            m_progressWindow->PostMessage(WM_CLOSE);
        WaitForSingleObject(m_progressWindowThread, INFINITE);
        m_progressWindowThread.Close();
        if (!cancel)
            g_cancel = false;
    }
};

class CrashProcessor
{
    Log& m_log;
    Config& m_config;

    unique_ptr<CrashInfo> m_crashInfo;
    CStringW m_workingFolder;
    CStringW m_miniDumpFile;
    CStringW m_miniDumpZipFile;
    CStringW m_fullDumpFile;
    CStringW m_fullDumpZipFile;
    CStringW m_infoDll;
    CStringW m_infoFile;
    CStringW m_patch;
    DumpWriter m_dumpWriter;
    doctor_dump::DumpUploaderWebServiceEx m_dumpUploader;

public:
    CrashProcessor(Log& log, Config& config)
        : m_log(log)
        , m_config(config)
        , m_dumpWriter(log)
        , m_dumpUploader(log)
    {
    }

    ~CrashProcessor()
    {
        if (!m_fullDumpFile.IsEmpty())
            DeleteFile(m_fullDumpFile);
        if (!m_miniDumpFile.IsEmpty())
            DeleteFile(m_miniDumpFile);
        if (!m_config.LeaveDumpFilesInTempFolder)
        {
            if (!m_fullDumpZipFile.IsEmpty())
                DeleteFile(m_fullDumpZipFile);
            if (!m_miniDumpZipFile.IsEmpty())
                DeleteFile(m_miniDumpZipFile);
            if (!m_workingFolder.IsEmpty())
                RemoveDirectory(m_workingFolder);
        }
    }

    void InitPathes()
    {
        GetTempPath(MAX_PATH, m_workingFolder.GetBuffer(MAX_PATH));
        m_workingFolder.ReleaseBuffer();

        SYSTEMTIME st;
        GetLocalTime(&st);

        CStringW t;
        t.Format(L"%ls_%d.%d.%d.%d_%04d%02d%02d_%02d%02d%02d",
            (LPCWSTR)m_config.Prefix,
            m_config.V[0], m_config.V[1], m_config.V[2], m_config.V[3],
            (int)st.wYear, (int)st.wMonth, (int)st.wDay, (int)st.wHour, (int)st.wMinute, (int)st.wSecond);

        m_workingFolder += t;
        m_miniDumpFile = m_workingFolder + _T("\\") + t + _T(".mini.dmp");
        m_miniDumpZipFile = m_workingFolder + _T("\\doctor_dump_mini.zip");
        m_fullDumpFile = m_workingFolder + _T("\\") + t + _T(".full.dmp");
        m_fullDumpZipFile = m_workingFolder + _T("\\doctor_dump_full.zip");
        m_infoDll = m_workingFolder + _T("\\info.dll");
        m_infoFile = m_workingFolder + _T("\\info.zip");
        m_patch = m_workingFolder + _T("\\solution.exe");

        if (!CreateDirectory(m_workingFolder, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
            throw std::runtime_error("failed to create temp directory");
    }

    int GetUserPCID() const
    {
        int PCID = 12345;
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
                PCID = D1 ^ MAKELONG(D3, D2) ^ MAKELONG(D5, D4) ^ D6;
            }
            RegCloseKey(hCryptography);
        }
        m_log.Info(_T("Machine ID 0x%08X"), PCID);
        return PCID;
    }

    void AppendUserInfo(const CStringW& dumpFile, Zip& zip)
    {
        CStringW crashInfoFile = m_workingFolder + L"\\crashinfo.xml";
        if (m_crashInfo->GetCrashInfoFile(crashInfoFile))
            m_config.FilesToAttach.push_back(std::make_pair<CStringW, CStringW>(CStringW(crashInfoFile), L"crashinfo.xml"));

        CStringW crashUserInfoFile = m_workingFolder + L"\\crashuserinfo.xml";
        if (m_config.UserInfo.size())
        {
            FILE* f = NULL;
            if (0 == _wfopen_s(&f, crashUserInfoFile, L"wt,ccs=UTF-8"))
            {
                fwprintf_s(f, L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<UserInfo>\n");
                for (auto it = m_config.UserInfo.cbegin(), end = m_config.UserInfo.cend(); it != end; ++it)
                {
                    m_log.Info(_T("Adding UserInfo \"%ls\" as \"%ls\"..."), static_cast<LPCWSTR>(it->first), static_cast<LPCWSTR>(it->second));
                    fwprintf_s(f,
                        L"<%ls>%ls</%ls>\n",
                        static_cast<LPCWSTR>(it->first),
                        static_cast<LPCWSTR>(it->second),
                        static_cast<LPCWSTR>(it->first));
                }
                fwprintf_s(f, L"</UserInfo>");
                fclose(f);
                m_config.FilesToAttach.push_back(std::make_pair<CStringW, CStringW>(CStringW(crashUserInfoFile), L"crashuserinfo.xml"));
            }
        }

        WIN32_FIND_DATAW ff;
        FindClose(FindFirstFileW(dumpFile, &ff));
        __int64 attachedSizeLimit = max(1024*1024I64, (static_cast<__int64>(ff.nFileSizeHigh) << 32) | ff.nFileSizeLow);

        m_log.Info(_T("Adding %d attaches..."), m_config.FilesToAttach.size());
        for (auto it = m_config.FilesToAttach.begin(), end = m_config.FilesToAttach.end(); it != end; ++it)
        {
            m_log.Info(_T("Adding \"%ls\" as \"%ls\"..."), static_cast<LPCWSTR>(it->first), static_cast<LPCWSTR>(it->second));
            WIN32_FIND_DATAW ff;
            HANDLE hFind = FindFirstFileW(it->first, &ff);
            if (hFind == INVALID_HANDLE_VALUE)
            {
                m_log.Warning(_T("File \"%ls\" not found..."), static_cast<LPCWSTR>(it->first));
                continue;
            }
            FindClose(hFind);
            __int64 size = (static_cast<__int64>(ff.nFileSizeHigh) << 32) | ff.nFileSizeLow;
            if (size > attachedSizeLimit)
            {
                m_log.Warning(_T("File \"%ls\" not skipped by size..."), static_cast<LPCWSTR>(it->first));
                continue;
            }
            attachedSizeLimit -= size;

            zip.AddFile(it->first, it->second, &g_cancel);
            m_log.Info(_T("Done."));
        }
        DeleteFile(crashInfoFile);
    }

    void PrepareDump(
        const CString& dumpFile,
        const CString& zipFile,
        HANDLE hProcess,
        DWORD processId,
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo,
        MINIDUMP_TYPE type,
        MINIDUMP_CALLBACK_INFORMATION* pCallback,
        bool attachFiles)
    {
        if (g_cancel)
            throw std::runtime_error("canceled");

        m_log.Info(_T("Writing dump (%s)..."), static_cast<LPCTSTR>(dumpFile));
        if (!m_dumpWriter.WriteMiniDump(hProcess, processId, pExceptInfo, dumpFile, type, pCallback))
        {
            // Frequent errors:
            //      0x80070070 - There is not enough space on the disk
            DWORD err = GetLastError();
            m_log.Error(_T("WriteMiniDump has failed with error %d"), err);
            CStringA text;
            text.Format("WriteMiniDump has failed with error %d", err);
            throw std::runtime_error(text);
        }
        m_log.Info(_T("Dump ready."));

        if (g_cancel)
            throw std::runtime_error("canceled");

        m_log.Info(_T("Zipping dump (%s)..."), static_cast<LPCTSTR>(zipFile));
        {
            Zip zip(zipFile);
            zip.AddFile(dumpFile, dumpFile.Mid(dumpFile.ReverseFind(_T('\\')) + 1));
            if (attachFiles)
                AppendUserInfo(dumpFile, zip);
        }
        m_log.Info(_T("Zipping done."));

        if (g_cancel)
            throw std::runtime_error("canceled");
    }

    void PrepareMiniDump(
        HANDLE hProcess,
        DWORD processId,
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo)
    {
        m_log.Info(_T("Prepare mini dump..."));

        DumpFilter dumpFilter(g_cancel, pExceptInfo ? pExceptInfo->ThreadId : 0);
        PrepareDump(m_miniDumpFile, m_miniDumpZipFile, hProcess, processId, pExceptInfo, MiniDumpFilterMemory, dumpFilter, false);
    }

    void PrepareFullDump(
        HANDLE hProcess,
        DWORD dwProcessId,
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo,
        bool attachFiles)
    {
        m_log.Info(_T("Prepare full dump..."));

        DumpFilter dumpFilter(g_cancel);
        PrepareDump(m_fullDumpFile, m_fullDumpZipFile, hProcess, dwProcessId, pExceptInfo, (MINIDUMP_TYPE)m_config.FullDumpType, dumpFilter, attachFiles);
    }

    typedef bool (WINAPI *pfnCollectInfo)(LPCWSTR file, LPCWSTR cfg, HANDLE process, DWORD processId);
    void PrepareAdditionalInfo(const doctor_dump::NeedMoreInfoResponse& response, HANDLE process, DWORD processId)
    {
        m_log.Info(_T("Prepare additional info..."));
#ifdef REMOTE_CODE_DOWNLOAD_AND_EXECUTION
        HMODULE hInfoDll = NULL;
        try
        {
            CAtlFile hFile(CreateFile(m_infoDll, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
            if (hFile == INVALID_HANDLE_VALUE)
                throw runtime_error("failed to create info.dll file");
            if (FAILED(hFile.Write(&response.infoModule[0], static_cast<DWORD>(response.infoModule.size()))))
                throw runtime_error("failed to write info.dll file");
            hFile.Close();

            hInfoDll = LoadLibraryW(m_infoDll);
            if (!hInfoDll)
                throw runtime_error("failed to load info.dll");

            pfnCollectInfo CollectInfo = (pfnCollectInfo) GetProcAddress(hInfoDll, "CollectInfo");
            if (!CollectInfo)
                throw runtime_error("failed to get CollectInfo from info.dll");

            if (!CollectInfo(m_infoFile, response.infoModuleCfg.c_str(), process, processId))
                throw runtime_error("CollectInfo failed");

            FreeLibrary(hInfoDll);
            DeleteFile(m_infoDll);
        }
        catch (...)
        {
            if (hInfoDll)
                FreeLibrary(hInfoDll);
            DeleteFile(m_infoDll);
            throw;
        }
#endif
    }

    void ProcessSolution(UI& ui, const doctor_dump::HaveSolutionResponse& solution)
    {
        m_log.Info(_T("Process solution..."));
        switch (solution.type)
        {
        case ns4__HaveSolutionResponse_SolutionType__Url:
            if (!solution.askConfirmation || ui.AskGetSolution(CSolutionDlg::Read))
            {
                CAtlStringW url = solution.url.c_str();
                url.Replace(L"{ClientID}", solution.clientID.c_str());
                url.Replace(L"{ProblemID}", ToString(solution.problemID));
                url.Replace(L"{DumpGroupID}", ToString(solution.dumpGroupID));
                url.Replace(L"{DumpID}", ToString(solution.dumpID));
                if (url.Find(L"http://") == 0 || url.Find(L"https://") == 0)
                    ShellExecute(NULL, _T("open"), CW2CT(url), NULL, NULL, SW_SHOWNORMAL);
            }
            break;
#ifdef REMOTE_CODE_DOWNLOAD_AND_EXECUTION
        case ns4__HaveSolutionResponse_SolutionType__Exe:
            if (!solution.askConfirmation || ui.AskGetSolution(CSolutionDlg::Install))
            {
                CAtlFile hFile(CreateFile(m_patch, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
                if (hFile == INVALID_HANDLE_VALUE)
                    throw runtime_error("failed to create solution.exe file");
                if (FAILED(hFile.Write(&solution.exe[0], static_cast<DWORD>(solution.exe.size()))))
                    throw runtime_error("failed to write solution.exe file");
                hFile.Close();

                STARTUPINFO si = {};
                si.cb = sizeof(si);
                PROCESS_INFORMATION pi = {};
                if (!CreateProcess(NULL, m_patch.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                    throw runtime_error("failed to start solution.exe");
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
            break;
#endif
        default:
            throw runtime_error("Unknown SolutionType");
        }
    }

    bool Process(Params& params)
    {
        HANDLE hProcess = params.Process;
        DWORD dwProcessId = params.ProcessId;
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo = &params.ExceptInfo;
        bool wasAssert = !!params.WasAssert;

        m_dumpWriter.Init(m_config.DbgHelpPath);

        // we need to get CrashInfo before writing the dumps, since dumps writing will change WorkingSet
        m_crashInfo.reset(new CrashInfo(hProcess));

        InitPathes();

        UI ui(m_config);

        ui.ShowInitialProgressWindow(wasAssert);

        PrepareMiniDump(hProcess, dwProcessId, pExceptInfo);
        if (m_config.ServiceMode)
        {
            PrepareFullDump(hProcess, dwProcessId, pExceptInfo, true);
            TerminateProcess(hProcess, E_FAIL); // It is necessary for DUMPPARSER to terminate app, because it should process our dump, and it could not do it since it crashes.
            CloseHandle(hProcess);
            hProcess = NULL;
        }

        doctor_dump::Application app;
        app.applicationGUID = m_config.ApplicationGUID;
        app.v[0] = m_config.V[0];
        app.v[1] = m_config.V[1];
        app.v[2] = m_config.V[2];
        app.v[3] = m_config.V[3];
        app.hotfix = m_config.Hotfix;
        app.processName = m_config.ProcessName;
        m_log.Info(_T("App %d.%d.%d.%d %ls"), app.v[0], app.v[1], app.v[2], app.v[3], app.applicationGUID);

        doctor_dump::DumpAdditionalInfo addInfo;
        addInfo.crashDate = time(NULL);
        addInfo.PCID = GetUserPCID();
        addInfo.submitterID = m_config.SubmitterID;
        addInfo.group = CA2W(params.Group);
        addInfo.description = m_config.CustomInfo;

        std::unique_ptr<doctor_dump::Response> response = m_dumpUploader.Hello(app, (LPCWSTR)m_config.AppName, (LPCWSTR)m_config.Company, addInfo);
        while (1)
        {
            switch (response->GetResponseType())
            {
            case doctor_dump::Response::HaveSolutionResponseType:
                if (!ui.IsSendingAdditionalInfoRejected())
                    ProcessSolution(ui, static_cast<doctor_dump::HaveSolutionResponse&>(*response));
                goto finish;

            case doctor_dump::Response::NeedMiniDumpResponseType:
                response = m_dumpUploader.UploadMiniDump(response->context, app, addInfo, (LPCWSTR)m_miniDumpZipFile);
                break;

            case doctor_dump::Response::NeedFullDumpResponseType:
                if (!ui.AskSendFullDump())
                {
                    response = m_dumpUploader.RejectedToSendAdditionalInfo(response->context, app, response->dumpID);
                    break;
                }

                ui.ShowFullDumpUploadProgressWindow();

                if (!m_config.ServiceMode)
                {
                    auto& resp = static_cast<doctor_dump::NeedFullDumpResponse&>(*response);
                    m_config.FullDumpType = m_config.FullDumpType & (~resp.restrictedDumpType);
                    PrepareFullDump(hProcess, dwProcessId, pExceptInfo, resp.attachUserInfo);
                    CloseHandle(hProcess);
                    hProcess = NULL;
                }
                SetEvent(params.ReportReady);
                response = m_dumpUploader.UploadFullDump(response->context, app, response->dumpID, (LPCWSTR)m_fullDumpZipFile, &ui);
                break;

            case doctor_dump::Response::NeedMoreInfoResponseType:
                if (!ui.AskSendFullDump())
                {
                    response = m_dumpUploader.RejectedToSendAdditionalInfo(response->context, app, response->dumpID);
                    break;
                }

                ui.ShowFullDumpUploadProgressWindow();

                PrepareAdditionalInfo(static_cast<doctor_dump::NeedMoreInfoResponse&>(*response), hProcess, dwProcessId);
                response = m_dumpUploader.UploadAdditionalInfo(response->context, app, response->dumpID, (LPCWSTR)m_infoFile, &ui);
                break;

            case doctor_dump::Response::ErrorResponseType:
                throw runtime_error((const char*)CW2A(static_cast<doctor_dump::ErrorResponse&>(*response).error.c_str()));

            case doctor_dump::Response::StopResponseType:
            default:
                goto finish;
            }
        }
finish:

        bool solutionIsUrl = response.get()
            && response->GetResponseType() == doctor_dump::Response::HaveSolutionResponseType
            && static_cast<doctor_dump::HaveSolutionResponse&>(*response).type == ns4__HaveSolutionResponse_SolutionType__Url;

        if (!m_config.ServiceMode
            && !solutionIsUrl
            // Do not confuse the user with report uploaded page since he rejected to send more data and no private data was sent
            && !ui.IsSendingAdditionalInfoRejected()
            && !response->urlToProblem.empty()
            && m_config.OpenProblemInBrowser)
        {
            CAtlStringW url = response->urlToProblem.c_str();
            if (url.Find(L"http://") == 0 || url.Find(L"https://") == 0)
                ShellExecuteW(NULL, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);
        }

        return true;
    }
};

bool IsNetworkProblemException()
{
    try
    {
        throw;
    }
    catch (doctor_dump::SoapException& ex)
    {
        if (ex.IsNetworkProblem())
            return true;
        return false;
    }
    catch (...)
    {
        return false;
    }
}

DWORD SendReportImpl(Log& log, Params& params, Config& config, bool isDebug)
{
    try
    {
        log.Info(_T("SENDRPT was invoked to send report, PID 0x%x"), params.ProcessId);
        CrashProcessor(log, config).Process(params);

        return TRUE;
    }
    catch (std::exception& ex)
    {
        log.Error(_T("Problem occurred: %hs"), ex.what());
        OutputDebugStringA(ex.what());

        if (isDebug)
        {
            ::MessageBoxA(0, ex.what(), "SendRpt: Error", MB_ICONERROR);
        }
        else
        {
            bool statSend = statistics::SendExceptionToGoogleAnalytics("UA-25460132-5", "sendrpt", ex.what(), true);
            if (!statSend && !IsNetworkProblemException())
                ::MessageBoxA(0, ex.what(), "SendRpt: Error", MB_ICONERROR);
        }

        return FALSE;
    }
}

int __cdecl SendReport(int argc, wchar_t* argv[])
{
    Log log(NULL, _T("sendrpt"));

    bool isDebug = false;
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
            isDebug = true;
            CString str;
            ULONG size = 1000;
            if (ERROR_SUCCESS == reg.QueryStringValue(_T("TraceFolder"), str.GetBuffer(size), &size))
            {
                str.ReleaseBuffer(size-1);
                InitializeLog();
                log.SetParams(LogMediaPtr(new FileMedia(str + _T("\\sendrpt.log"), true, false)));
            }
        }
    }

    Params params;
    Config config;

    if (argc >= 2 && wcscmp(argv[1], L"-uitest") == 0)
    {
        config.AppName = "Test App";
        config.Company = "Long Company Name";
        if (argc >= 3)
            config.LangFilePath = argv[2];
        UI ui(config);
        ui.Test();
        return FALSE;
    }

    Serializer ser(argv[1]);
    ser << params << config;
    params.ExceptInfo.ClientPointers = TRUE;

    return SendReportImpl(log, params, config, isDebug);
}