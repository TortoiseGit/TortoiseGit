#include "stdafx.h"
#include "Provider.h"
#include "WaitDialog.h"

CProvider::CProvider()
{
}

HRESULT CProvider::FinalConstruct()
{
    return S_OK;
}

void CProvider::FinalRelease()
{
}

CProvider::parameters_t CProvider::ParseParameters(BSTR parameters) const
{
    CString temp(parameters);

    parameters_t result;

    // TODO: Handle quoting and stuff
    int pos = 0;
    CString token = temp.Tokenize(_T(";"), pos);
    while (token != _T(""))
    {
        int x = token.Find('=');
        CString name = token.Left(x);
        CString value = token.Mid(x + 1);

        result[name] = value;

        token = temp.Tokenize(_T(";"), pos);
    }

    return result;
}

HRESULT STDMETHODCALLTYPE CProvider::ValidateParameters(
    /* [in] */ HWND hParentWnd,
    /* [in] */ BSTR parameters,
    /* [retval][out] */ VARIANT_BOOL *valid)
{
    // Don't bother validating the parameters yet.
    *valid = VARIANT_TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CProvider::GetLinkText(
    /* [in] */ HWND hParentWnd,
    /* [in] */ BSTR parameters,
    /* [retval][out] */ BSTR *linkText)
{
    parameters_t params = ParseParameters(parameters);
    CString prompt = params[CString("Prompt")];
    if (prompt.IsEmpty())
        *linkText = SysAllocString(L"Choose Task...");
    else
        *linkText = prompt.AllocSysString();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CProvider::GetCommitMessage(
    /* [in] */ HWND hParentWnd,
    /* [in] */ BSTR parameters,
    /* [in] */ BSTR commonRoot,
    /* [in] */ SAFEARRAY * pathList,
    /* [in] */ BSTR originalMessage,
    /* [retval][out] */ BSTR *newMessage)
{
    BSTR bugID = SysAllocString(_T(""));
    BSTR * bugIDOut = NULL;
    HRESULT hr = GetCommitMessage2(hParentWnd, parameters, NULL, commonRoot, pathList, originalMessage, bugID, bugIDOut, NULL, NULL, newMessage);
    SysFreeString(bugID);
    return hr;
}

HRESULT STDMETHODCALLTYPE CProvider::GetCommitMessage2(
    /* [in] */ HWND hParentWnd,
    /* [in] */ BSTR parameters,
    /* [in] */ BSTR commonURL,
    /* [in] */ BSTR commonRoot,
    /* [in] */ SAFEARRAY * pathList,
    /* [in] */ BSTR originalMessage,
    /* [in] */ BSTR bugID,
    /* [out]*/ BSTR * bugIDOut,
    /* [out]*/ SAFEARRAY ** revPropNames,
    /* [out]*/ SAFEARRAY ** revPropValues,
    /* [retval][out] */ BSTR *newMessage)
{
    USES_CONVERSION;

    if (commonURL)
    {
        // do something with the common root url
        // if necessary
    }

    parameters_t params = ParseParameters(parameters);
    CString commandLine = params[CString("CommandLine")];

    if (commandLine.IsEmpty())
    {
        MessageBox(hParentWnd, _T("CommandLine parameter is empty"), _T("ExampleAtlPlugin"), MB_OK | MB_ICONERROR);
        return S_OK;
    }

    TCHAR szTempPath[MAX_PATH];
    GetTempPath(_countof(szTempPath), szTempPath);

    // Create a temporary file to hold the path list, and write the list to the file.
    TCHAR szPathListTempFile[MAX_PATH];
    GetTempFileName(szTempPath, _T("svn"), 0, szPathListTempFile);

    DWORD bytesWritten;
    HANDLE hPathListFile = CreateFile(szPathListTempFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

    LONG a, b;
    if (FAILED(SafeArrayGetLBound(pathList, 1, &a)) || (FAILED(SafeArrayGetUBound(pathList, 1, &b))))
        return E_FAIL;

    for (LONG i = a; i <= b; ++i)
    {
        BSTR path = NULL;
        SafeArrayGetElement(pathList, &i, &path);

        CStringA line = OLE2A(path);
        line += "\r\n";

        WriteFile(hPathListFile, line, line.GetLength(), &bytesWritten, NULL);
    }

    CloseHandle(hPathListFile);

    TCHAR szOriginalMessageTempFile[MAX_PATH];
    GetTempFileName(szTempPath, _T("svn"), 0, szOriginalMessageTempFile);

    HANDLE hOriginalMessageFile = CreateFile(szOriginalMessageTempFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

    CStringA temp = OLE2A(originalMessage);
    WriteFile(hOriginalMessageFile, temp, temp.GetLength(), &bytesWritten, NULL);

    CloseHandle(hOriginalMessageFile);

    commandLine.AppendFormat(_T(" \"%s\" \"%ls\" \"%s\""), szPathListTempFile, commonRoot, szOriginalMessageTempFile);

    CString message = originalMessage;
    CWaitDialog dlg(commandLine);
    if (dlg.DoModal() == IDOK)
    {
        HANDLE hOrig = CreateFile(szOriginalMessageTempFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        DWORD cb = GetFileSize(hOrig, NULL);
        BYTE *buffer = (BYTE *)malloc(cb + 1);
        SecureZeroMemory(buffer, cb + 1);
        DWORD bytesRead;
        ReadFile(hOrig, buffer, cb, &bytesRead, NULL);
        CloseHandle(hOrig);

        message = A2T((const char *)buffer);

        /* Please note: revPropNames and revPropValues are ignored by TortoiseGit! However, you must return at least empty arrays */
        CString propName1 = _T("bugtraq:issueIDs");
        CString propName2 = _T("myownproperty");
        CString propValue1 = _T("13, 16, 17");
        CString propValue2 = _T("myownvalue");

        {
            SAFEARRAYBOUND bounds = {2, 0};
            SAFEARRAY* psa = SafeArrayCreate(VT_BSTR, 1, &bounds);
            BSTR* strArray;
            SafeArrayAccessData(psa, reinterpret_cast<void**> (&strArray));
            strArray[0] = propName1.AllocSysString();
            strArray[1] = propName2.AllocSysString();
            SafeArrayUnaccessData(psa);
            *revPropNames = psa;
        }
        {
            SAFEARRAYBOUND bounds = {2, 0};
            SAFEARRAY* psa = SafeArrayCreate(VT_BSTR, 1, &bounds);
            BSTR* strArray;
            SafeArrayAccessData(psa, reinterpret_cast<void**> (&strArray));
            strArray[0] = propValue1.AllocSysString();
            strArray[1] = propValue2.AllocSysString();
            SafeArrayUnaccessData(psa);
            *revPropValues = psa;
        }


    }

    DeleteFile(szPathListTempFile);
    DeleteFile(szOriginalMessageTempFile);

    CString bugIDString = _T("12345,4567");
    *bugIDOut = bugIDString.AllocSysString();


    *newMessage = message.AllocSysString();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CProvider::CheckCommit (
     /* [in] */ HWND hParentWnd,
     /* [in] */ BSTR parameters,
     /* [in] */ BSTR commonURL,
     /* [in] */ BSTR commonRoot,
     /* [in] */ SAFEARRAY * pathList,
     /* [in] */ BSTR commitMessage,
     /* [out, retval] */ BSTR * errorMessage)
{
    CString err = _T("Test error string");
    *errorMessage = err.AllocSysString();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CProvider::OnCommitFinished (
    /* [in] */ HWND hParentWnd,
    /* [in] */ BSTR commonRoot,
    /* [in] */ SAFEARRAY * pathList,
    /* [in] */ BSTR logMessage,
    /* [in] */ ULONG revision,
    /* [out, retval] */ BSTR * error)
{
    CString err = _T("Test error string");
    *error = err.AllocSysString();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CProvider::HasOptions(
                   /* [out, retval] */ VARIANT_BOOL *ret)
{
    MessageBox(NULL, _T("test"), _T("test"), MB_ICONERROR);
    *ret = VARIANT_FALSE;
    MessageBox(NULL, _T("test2"), _T("test2"), MB_ICONERROR);
    return S_OK;
}

// this method is called if HasOptions() returned true before.
// Use this to show a custom dialog so the user doesn't have to
// create the parameters string manually
HRESULT STDMETHODCALLTYPE CProvider::ShowOptionsDialog(
                          /* [in] */ HWND hParentWnd,
                          /* [in] */ BSTR parameters,
                          /* [out, retval] */ BSTR * newparameters
                          )
{
    // we don't show an options dialog
    return E_NOTIMPL;
}
