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
#include "resource.h"
#include "CrashHandlerExport.h"
#include "..\SendRpt\Config.h"
#include "..\SendRpt\Serializer.h"
#include <signal.h>
#include <algorithm>

HINSTANCE g_hThisDLL = NULL;
bool g_applicationVerifierPresent = false;
bool g_ownProcess = false;
volatile LONG g_insideCrashHandler = 0;
LPTOP_LEVEL_EXCEPTION_FILTER g_prevTopLevelExceptionFilter = NULL;

// It should not be destroyed on PROCESS_DETACH, because it may be used later if someone will crash on deinitialization
// so it is on heap, but not static (runtime destroy static objects on PROCESS_DETACH)
Config* g_pConfig = new Config();

DWORD  g_tlsPrevTerminatePtr = TLS_OUT_OF_INDEXES;
typedef terminate_function (__cdecl *pfn_set_terminate)(terminate_function);
pfn_set_terminate g_set_terminate = NULL;

enum CrashServerCustomExceptionCodes
{
    CRASHSERVER_EXCEPTION_ASSERTION_VIOLATED = CrashHandler::ExceptionAssertionViolated, // 0xCCE17000
    CRASHSERVER_EXCEPTION_CPP_TERMINATE = 0xCCE17001,
    CRASHSERVER_EXCEPTION_CPP_PURE_CALL = 0xCCE17002,
    CRASHSERVER_EXCEPTION_CPP_INVALID_PARAMETER = 0xCCE17003,
};

void ExtractFileFromResource(HMODULE hModule, DWORD resId, LPCTSTR path)
{
    HRSRC hDbghelpRes = FindResource(hModule, MAKEINTRESOURCE(resId), RT_RCDATA);
    if (!hDbghelpRes)
        throw runtime_error("failed to find file in resources");

    HGLOBAL hDbghelpGlobal = LoadResource(hModule, hDbghelpRes);
    if (!hDbghelpGlobal)
        throw runtime_error("failed to load file from resources");

    CAtlFile hFile(CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
    if (hFile == INVALID_HANDLE_VALUE)
        throw runtime_error("failed to create file");

    if (FAILED(hFile.Write(LockResource(hDbghelpGlobal), SizeofResource(hModule, hDbghelpRes))))
        throw runtime_error("failed to write file");
}

DWORD WINAPI SendReportThread(LPVOID lpParameter)
{
    InterlockedIncrement(&g_insideCrashHandler);

    //::MessageBoxA(0, "SendReportThread", "SendReportThread", 0);
    MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo = (MINIDUMP_EXCEPTION_INFORMATION*)lpParameter;
    try
    {
        WCHAR sendRptExe[MAX_PATH], drive[4], dir[MAX_PATH], fname[MAX_PATH], ext[MAX_PATH];
        GetModuleFileNameW(NULL, sendRptExe, _countof(sendRptExe));
        _wsplitpath_s(sendRptExe, drive, dir, fname, ext);
        _wmakepath_s(sendRptExe, drive, dir, L"SendRpt", L".exe");
        bool localSendRpt = 0 == _waccess_s(sendRptExe, 00/* Existence only */);
        if (!localSendRpt)
        {
            GetTempPathW(_countof(sendRptExe), sendRptExe);
            wcscat_s(sendRptExe, L"SendRpt.exe");

            ExtractFileFromResource(g_hThisDLL, IDR_SENDRPT, sendRptExe);
        }

        CString cmd;
        cmd.Format(_T("\"%s\" "), sendRptExe);

        CHandle hProcess;
        DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &hProcess.m_h, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | THREAD_ALL_ACCESS, TRUE, 0);

        CHandle hReportReady(CreateEvent(NULL, TRUE, FALSE, NULL));
        SetHandleInformation(hReportReady, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

        Params param;
        param.Process = hProcess;
        param.ProcessId = GetCurrentProcessId();
        param.ExceptInfo = *pExceptInfo;
        param.WasAssert = pExceptInfo->ExceptionPointers->ExceptionRecord->ExceptionCode == CRASHSERVER_EXCEPTION_ASSERTION_VIOLATED;
        param.ReportReady = hReportReady;

        Serializer ser;
        ser << param << *g_pConfig;
        cmd.Append(ser.GetHex());

        STARTUPINFO si = {sizeof(si)};
        PROCESS_INFORMATION pi;
        if (!CreateProcess(NULL, cmd.GetBuffer(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
            throw std::runtime_error("failed to start SendRpt");
        CloseHandle(pi.hThread);

        HANDLE handles[] = { pi.hProcess, hReportReady.m_h };
        DWORD res = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);

        CloseHandle(pi.hProcess);

        // if hReportReady event signaled, SendRpt is still working, so delete only when SendRpt has finished.
        if (!localSendRpt && handles[res - WAIT_OBJECT_0] == pi.hProcess)
            DeleteFileW(sendRptExe);

        InterlockedDecrement(&g_insideCrashHandler);

        return TRUE;
    }
    catch (std::exception& ex)
    {
        OutputDebugStringA(ex.what());
    }

    InterlockedDecrement(&g_insideCrashHandler);

    return FALSE;
}

LONG SendReport(EXCEPTION_POINTERS* pExceptionPointers)
{
    if (IsDebuggerPresent())
        return EXCEPTION_CONTINUE_SEARCH;

    // We can enter here because of stack overflow, so lets all local variables
    // be static, because variables on stack can lead to second stack overflow.
    static DWORD dwThreadId;
    static HANDLE hThread;
    static MINIDUMP_EXCEPTION_INFORMATION exceptInfo;
    exceptInfo.ThreadId = GetCurrentThreadId();
    exceptInfo.ExceptionPointers = pExceptionPointers;
    exceptInfo.ClientPointers = FALSE;

    // If stack overflow, do all processing in another thread
    if (pExceptionPointers != NULL
        && pExceptionPointers->ExceptionRecord != NULL
        && pExceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW
        && (hThread = CreateThread(NULL, 0, SendReportThread, &exceptInfo, 0, &dwThreadId)) != NULL)
    {
        WaitForSingleObject(hThread, INFINITE);
    }
    else
    {
        SendReportThread(&exceptInfo);
    }

    if (pExceptionPointers->ExceptionRecord->ExceptionCode == CRASHSERVER_EXCEPTION_ASSERTION_VIOLATED)
        return EXCEPTION_CONTINUE_EXECUTION;

    return g_pConfig->UseWER ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers)
{
    if (pExceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT
        && g_applicationVerifierPresent)
    {
        // This crash has been processed in VectoredExceptionHandler and if we here we want WinQual
        return EXCEPTION_CONTINUE_SEARCH;
    }

    return SendReport(pExceptionPointers);
}

LONG CALLBACK VectoredExceptionHandler(EXCEPTION_POINTERS* pExceptionPointers)
{
    // Probably this is Application Verifier stop
    if (pExceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT
        && g_applicationVerifierPresent)
    {
        LONG res = SendReport(pExceptionPointers);
        if (res == EXCEPTION_EXECUTE_HANDLER)
            ExitProcess(0); // we don't want WinQual from Application Verifier
    }

    // Asserts should be handled in VectoredExceptionHandler since EXCEPTION_CONTINUE_EXECUTION is used
    // and __try { ... } __except(EXCEPTION_EXECUTE_HANDLER) { ... } in the way to TopLevelExceptionFilter
    // may break this logic.
    if (pExceptionPointers->ExceptionRecord->ExceptionCode == CRASHSERVER_EXCEPTION_ASSERTION_VIOLATED)
        return SendReport(pExceptionPointers);

    return EXCEPTION_CONTINUE_SEARCH;
}

FARPROC GetSetUnhandledExceptionFilterPointer()
{
    FARPROC suef = NULL;
    // In Windows 8 SetUnhandledExceptionFilter implementation is in kernelbase
    if (HMODULE kernelbase = GetModuleHandle(_T("kernelbase")))
        suef = GetProcAddress(kernelbase, "SetUnhandledExceptionFilter");
    if (!suef)
    {
        if (HMODULE kernel32 = GetModuleHandle(_T("kernel32")))
            suef = GetProcAddress(kernel32, "SetUnhandledExceptionFilter");
    }
    return suef;
}

void SwitchSetUnhandledExceptionFilter(bool enable)
{
    FARPROC suef = GetSetUnhandledExceptionFilterPointer();
    if (!suef)
        return;

    // newBody is something like that:
    //
    //  LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER)
    //  {
    //   return NULL;
    //  }
#if defined(_M_X64)
    static const BYTE newBody[] =
    {
        0x33, 0xC0, // xor eax,eax
        0xC3        // ret
    };
#elif defined(_M_IX86)
    static const BYTE newBody[] =
    {
        0x8B, 0xFF,         // mov edi,edi <- for hotpatching
        0x33, 0xC0,         // xor eax,eax
        0xC2, 0x04, 0x00    // ret 4
    };
#else
#   error Unsupported architecture
#endif
    const SIZE_T bodySize = sizeof(newBody);

    static BYTE oldBody[bodySize] = { 0 };

    DWORD oldProtection;
    if (!VirtualProtect(suef, bodySize, PAGE_EXECUTE_READWRITE, &oldProtection))
        return;
    if (!enable)
    {
        memcpy(oldBody, suef, bodySize);
        memcpy(suef, newBody, bodySize);
    }
    else if (oldBody[0] != 0)
    {
        memcpy(suef, oldBody, bodySize);
    }

    VirtualProtect(suef, bodySize, oldProtection, &oldProtection);
}

void DisableSetUnhandledExceptionFilter()
{
    SwitchSetUnhandledExceptionFilter(false);
}

void ReenableSetUnhandledExceptionFilter()
{
    SwitchSetUnhandledExceptionFilter(true);
}

// This code should be inplace, so it is a macro
#define SendReportWithCode(code) __try { RaiseException(code, EXCEPTION_NONCONTINUABLE, 0, NULL); } __except(TopLevelExceptionFilter(GetExceptionInformation())) {}

void CrashServer_SendAssertionViolated()
{
    RaiseException(CRASHSERVER_EXCEPTION_ASSERTION_VIOLATED, 0, 0, NULL);
}

void CrashServer_TerminateHandler()
{
    SendReportWithCode(CRASHSERVER_EXCEPTION_CPP_TERMINATE);

    if (g_tlsPrevTerminatePtr != TLS_OUT_OF_INDEXES)
    {
        terminate_function prev_terminate = static_cast<terminate_function>(TlsGetValue(g_tlsPrevTerminatePtr));
        if (prev_terminate)
            prev_terminate();
    }

    if (!g_pConfig->UseWER)
        ExitProcess(0);
}

void CrashServer_PureCallHandler()
{
    SendReportWithCode(CRASHSERVER_EXCEPTION_CPP_PURE_CALL);
    ExitProcess(0);
}

void CrashServer_InvalidParameterHandler(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t)
{
    SendReportWithCode(CRASHSERVER_EXCEPTION_CPP_INVALID_PARAMETER);
    ExitProcess(0);
}

void CrashServer_SigAbrtHandler(int)
{
    SendReportWithCode(CRASHSERVER_EXCEPTION_CPP_TERMINATE);
    ExitProcess(0);
}

void MakePerThreadInitialization()
{
    if (g_tlsPrevTerminatePtr != TLS_OUT_OF_INDEXES && g_set_terminate)
        TlsSetValue(g_tlsPrevTerminatePtr, g_set_terminate(CrashServer_TerminateHandler));
}

void InitCrtErrorHandlers()
{
    LPCTSTR crtModules[] = {
        _T("msvcr70"), _T("msvcr70d"),
        _T("msvcr71"), _T("msvcr71d"),
        _T("msvcr80"), _T("msvcr80d"),
        _T("msvcr90"), _T("msvcr90d"),
        _T("msvcr100"), _T("msvcr100d"),
        _T("msvcr110"), _T("msvcr110d"),
        _T("msvcr120"), _T("msvcr120d"),
    };

    HMODULE hMsvcrDll = NULL;
    for (size_t i = 0; !hMsvcrDll && i < _countof(crtModules); ++i)
        hMsvcrDll = GetModuleHandle(crtModules[i]);

    if (!hMsvcrDll)
        return;

    g_set_terminate = (pfn_set_terminate) GetProcAddress(hMsvcrDll, "?set_terminate@@YAP6AXXZP6AXXZ@Z");

    typedef _purecall_handler (__cdecl *pfn_set_purecall_handler)(_purecall_handler);
    pfn_set_purecall_handler l_set_purecall_handler = (pfn_set_purecall_handler) GetProcAddress(hMsvcrDll, "_set_purecall_handler");
    if (l_set_purecall_handler)
        l_set_purecall_handler(CrashServer_PureCallHandler);

    typedef _invalid_parameter_handler (__cdecl *pfn_set_invalid_parameter_handler)(_invalid_parameter_handler);
    pfn_set_invalid_parameter_handler l_set_invalid_parameter_handler = (pfn_set_invalid_parameter_handler) GetProcAddress(hMsvcrDll, "_set_invalid_parameter_handler");
    if (l_set_invalid_parameter_handler)
        l_set_invalid_parameter_handler(CrashServer_InvalidParameterHandler);

    typedef void (__cdecl *pfn_signal)(int sig, void (__cdecl *func)(int));
    pfn_signal l_signal = (pfn_signal) GetProcAddress(hMsvcrDll, "signal");
    if (l_signal)
        l_signal(SIGABRT, CrashServer_SigAbrtHandler);
}

BOOL InitCrashHandler(ApplicationInfo* applicationInfo, HandlerSettings* handlerSettings, BOOL ownProcess)
{
#define IS_EXIST(field) (FIELD_OFFSET(ApplicationInfo, field) < applicationInfo->ApplicationInfoSize)
    if (applicationInfo == NULL
        || applicationInfo->ApplicationInfoSize == 0
        || applicationInfo->ApplicationInfoSize > sizeof(ApplicationInfo)
        || !IS_EXIST(ApplicationGUID) || applicationInfo->ApplicationGUID == NULL
        || !IS_EXIST(Prefix)  || applicationInfo->Prefix == NULL
        || !IS_EXIST(AppName) || applicationInfo->AppName == NULL
        || !IS_EXIST(Company) || applicationInfo->Company == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    g_pConfig->ApplicationGUID = applicationInfo->ApplicationGUID;
    g_pConfig->Prefix = applicationInfo->Prefix;
    g_pConfig->AppName = applicationInfo->AppName;
    g_pConfig->Company = applicationInfo->Company;
    if (IS_EXIST(V))
        memcpy(g_pConfig->V, applicationInfo->V, sizeof(applicationInfo->V));
    else
        memset(g_pConfig->V, 0, sizeof(g_pConfig->V));
    if (IS_EXIST(Hotfix))
        g_pConfig->Hotfix = applicationInfo->Hotfix;
    else
        g_pConfig->Hotfix = 0;
    if (IS_EXIST(PrivacyPolicyUrl) && applicationInfo->PrivacyPolicyUrl != NULL)
        g_pConfig->PrivacyPolicyUrl = applicationInfo->PrivacyPolicyUrl;
    else
        g_pConfig->PrivacyPolicyUrl.Format(L"http://www.crash-server.com/AppPrivacyPolicy.aspx?AppID=%s", (LPCWSTR)g_pConfig->ApplicationGUID);
#undef IS_EXIST

#define IS_EXIST(field) (handlerSettings != NULL && (FIELD_OFFSET(HandlerSettings, field) < handlerSettings->HandlerSettingsSize))

    if (handlerSettings != NULL
        && (handlerSettings->HandlerSettingsSize == 0 || handlerSettings->HandlerSettingsSize > sizeof(HandlerSettings)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IS_EXIST(LeaveDumpFilesInTempFolder))
    {
        g_pConfig->ServiceMode = handlerSettings->LeaveDumpFilesInTempFolder == 2; // hidden behavior (used for dumpparser)
        g_pConfig->LeaveDumpFilesInTempFolder = handlerSettings->LeaveDumpFilesInTempFolder != FALSE;
    }
    else
    {
        g_pConfig->ServiceMode = 0;
        g_pConfig->LeaveDumpFilesInTempFolder = FALSE;
    }
    if (IS_EXIST(OpenProblemInBrowser))
        g_pConfig->OpenProblemInBrowser = handlerSettings->OpenProblemInBrowser;
    else
        g_pConfig->OpenProblemInBrowser = FALSE;
    if (IS_EXIST(UseWER))
        g_pConfig->UseWER = handlerSettings->UseWER;
    else
        g_pConfig->UseWER = FALSE;
    if (IS_EXIST(SubmitterID))
        g_pConfig->SubmitterID = handlerSettings->SubmitterID;
    else
        g_pConfig->SubmitterID = 0;
#undef IS_EXIST

    WCHAR path[MAX_PATH], drive[MAX_PATH], dir[MAX_PATH], fname[MAX_PATH], ext[MAX_PATH];
    GetModuleFileNameW(NULL, path, _countof(path));
    _wsplitpath_s(path, drive, dir, fname, ext);
    g_pConfig->ProcessName = CStringW(fname) + ext;
    g_pConfig->ProcessName.MakeLower();

    g_ownProcess = ownProcess != FALSE;

    static bool inited = false;
    if (!inited)
    {
        if (g_ownProcess)
        {
            // Application verifier sets its own VectoredExceptionHandler
            // and Application verifier breaks redirected to WinQual.
            // After that TopLevelExceptionFilter works.
            // This behavior looks bad and anyway we don't need WinQual.
            // So we set our VectoredExceptionHandler before AppVerifier and
            // catch problems first.
            g_applicationVerifierPresent = GetModuleHandle(_T("verifier.dll")) != NULL;
            if (g_applicationVerifierPresent)
                AddVectoredExceptionHandler(TRUE, VectoredExceptionHandler);

            g_prevTopLevelExceptionFilter = SetUnhandledExceptionFilter(TopLevelExceptionFilter);
            // There is a lot of libraries that want to set their own wrong UnhandledExceptionFilter in our application.
            // One of these is MSVCRT with __report_gsfailure and _call_reportfault leading to many
            // of MSVCRT error reports to be redirected to Dr. Watson.
            DisableSetUnhandledExceptionFilter();

            InitCrtErrorHandlers();
            MakePerThreadInitialization();
        }

        // Need to lock library in process.
        // Since some crashes appears on process deinitialization and we should be ready to handle it.
        LoadLibrary(_T("crshhndl.dll"));
        inited = true;
    }

    return TRUE;
}

void AddUserInfoToReport(LPCWSTR key, LPCWSTR value)
{
    if (!g_pConfig)
        return;
    g_pConfig->UserInfo.push_back(make_pair(CStringW(key), value));
}

void AddFileToReport(LPCWSTR path, LPCWSTR reportFileName)
{
    if (!g_pConfig)
        return;
    CStringW fileName;
    if (!reportFileName)
    {
        WCHAR drive[MAX_PATH], dir[MAX_PATH], fname[MAX_PATH], ext[MAX_PATH];
        _wsplitpath_s(path, drive, dir, fname, ext);
        fileName = CStringW(fname) + ext;
    }
    else
    {
        fileName = reportFileName;
    }
    g_pConfig->FilesToAttach.push_back(make_pair(CStringW(path), fileName));
}

void RemoveFileFromReport(LPCWSTR path)
{
    if (!g_pConfig)
        return;
/* TODO: needs to be converted to work with VS2008, but atm it's never called
    auto it = std::find_if(g_pConfig->FilesToAttach.begin(), g_pConfig->FilesToAttach.end(),
        [path](std::pair<CStringW, CStringW>& x){ return x.first == path; });
    if (it != g_pConfig->FilesToAttach.end())
        g_pConfig->FilesToAttach.erase(it);
*/
}

BOOL GetVersionFromFile(LPCWSTR path, ApplicationInfo* appInfo)
{
    if (!path || !appInfo)
        return FALSE;

    appInfo->V[0] = 0;
    appInfo->V[1] = 0;
    appInfo->V[2] = 0;
    appInfo->V[3] = 0;

    DWORD size = GetFileVersionInfoSize(path, NULL);
    if (size == 0)
        return FALSE;
    LPVOID pVerInfo = HeapAlloc(GetProcessHeap(), 0, size);
    if (!GetFileVersionInfo(path, 0, size, pVerInfo))
    {
        HeapFree(GetProcessHeap(), 0, pVerInfo);
        return FALSE;
    }
    VS_FIXEDFILEINFO* lpFileinfo = NULL;
    UINT uLen;
    if (!VerQueryValue(pVerInfo, _T("\\"), (LPVOID*)&lpFileinfo, &uLen))
    {
        HeapFree(GetProcessHeap(), 0, pVerInfo);
        return FALSE;
    }
    appInfo->V[0] = HIWORD(lpFileinfo->dwProductVersionMS);
    appInfo->V[1] = LOWORD(lpFileinfo->dwProductVersionMS);
    appInfo->V[2] = HIWORD(lpFileinfo->dwProductVersionLS);
    appInfo->V[3] = LOWORD(lpFileinfo->dwProductVersionLS);
    HeapFree(GetProcessHeap(), 0, pVerInfo);

    return TRUE;
}

BOOL GetVersionFromApp(ApplicationInfo* appInfo)
{
    if (!appInfo)
        return FALSE;

    appInfo->V[0] = 0;
    appInfo->V[1] = 0;
    appInfo->V[2] = 0;
    appInfo->V[3] = 0;

    TCHAR path[MAX_PATH];
    if (!GetModuleFileName(NULL, path, _countof(path)))
        return FALSE;

    return GetVersionFromFile(path, appInfo);
}

BOOL IsReadyToExit()
{
    return g_insideCrashHandler == 0 ? TRUE : FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hThisDLL = hinstDLL;
        g_tlsPrevTerminatePtr = TlsAlloc();
        break;
    case DLL_PROCESS_DETACH:
        if (g_tlsPrevTerminatePtr != TLS_OUT_OF_INDEXES)
        {
            TlsFree(g_tlsPrevTerminatePtr);
            g_tlsPrevTerminatePtr = TLS_OUT_OF_INDEXES;
        }
        if (g_prevTopLevelExceptionFilter)
        {
            ReenableSetUnhandledExceptionFilter();
            SetUnhandledExceptionFilter(g_prevTopLevelExceptionFilter);
        }
        if (g_applicationVerifierPresent)
            RemoveVectoredExceptionHandler(VectoredExceptionHandler);
        break;
    case DLL_THREAD_ATTACH:
        if (g_ownProcess)
        {
            MakePerThreadInitialization();
        }
        break;
    }
    return TRUE;
}
