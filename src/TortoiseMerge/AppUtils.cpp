// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "StdAfx.h"
#include "Registry.h"
#include "AppUtils.h"
#include "UnicodeUtils.h"
#include "ProgressDlg.h"

#include "svn_pools.h"
#include "svn_io.h"
#include "svn_path.h"
#include "svn_diff.h"
#include "svn_string.h"
#include "svn_utf.h"

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

BOOL CAppUtils::GetVersionedFile(CString sPath, CString sVersion, CString sSavePath, CProgressDlg * progDlg, HWND hWnd /*=NULL*/)
{
	CString sSCMPath = CRegString(_T("Software\\TortoiseMerge\\SCMPath"), _T(""));
	if (sSCMPath.IsEmpty())
	{
		// no path set, so use TortoiseSVN as default
		sSCMPath = CRegString(_T("Software\\TortoiseSVN\\ProcPath"), _T(""), false, HKEY_LOCAL_MACHINE);
		if (sSCMPath.IsEmpty())
		{
			TCHAR pszSCMPath[MAX_PATH];
			GetModuleFileName(NULL, pszSCMPath, MAX_PATH);
			sSCMPath = pszSCMPath;
			sSCMPath = sSCMPath.Left(sSCMPath.ReverseFind('\\'));
			sSCMPath += _T("\\TortoiseProc.exe");
		}
		sSCMPath += _T(" /command:cat /path:\"%1\" /revision:%2 /savepath:\"%3\" /hwnd:%4");
	}
	CString sTemp;
	sTemp.Format(_T("%d"), hWnd);
	sSCMPath.Replace(_T("%1"), sPath);
	sSCMPath.Replace(_T("%2"), sVersion);
	sSCMPath.Replace(_T("%3"), sSavePath);
	sSCMPath.Replace(_T("%4"), sTemp);
	// start the external SCM program to fetch the specific version of the file
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	if (CreateProcess(NULL, (LPTSTR)(LPCTSTR)sSCMPath, NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		MessageBox(NULL, (LPCTSTR)lpMsgBuf, _T("TortoiseMerge"), MB_OK | MB_ICONERROR);
		LocalFree( lpMsgBuf );
	}
	DWORD ret = 0;
	do
	{
		ret = WaitForSingleObject(process.hProcess, 100);
	} while ((ret == WAIT_TIMEOUT) && (!progDlg->HasUserCancelled()));
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	
	if (progDlg->HasUserCancelled())
	{
		return FALSE;
	}
	if (!PathFileExists(sSavePath))
		return FALSE;
	return TRUE;
}

bool CAppUtils::CreateUnifiedDiff(const CString& orig, const CString& modified, const CString& output, bool bShowError)
{
	apr_file_t * outfile = NULL;
	apr_pool_t * pool = svn_pool_create(NULL);

	svn_error_t * err = svn_io_file_open (&outfile, svn_path_canonicalize(CUnicodeUtils::GetUTF8(output), pool),
		APR_WRITE | APR_CREATE | APR_BINARY | APR_TRUNCATE,
		APR_OS_DEFAULT, pool);
	if (err == NULL)
	{
		svn_stream_t * stream = svn_stream_from_aprfile2(outfile, false, pool);
		if (stream)
		{
			svn_diff_t * diff = NULL;
			svn_diff_file_options_t * opts = svn_diff_file_options_create(pool);
			opts->ignore_eol_style = false;
			opts->ignore_space = svn_diff_file_ignore_space_none;
			err = svn_diff_file_diff_2(&diff, svn_path_canonicalize(CUnicodeUtils::GetUTF8(orig), pool), 
				svn_path_canonicalize(CUnicodeUtils::GetUTF8(modified), pool), opts, pool);
			if (err == NULL)
			{
				err = svn_diff_file_output_unified(stream, diff, svn_path_canonicalize(CUnicodeUtils::GetUTF8(orig), pool), 
					svn_path_canonicalize(CUnicodeUtils::GetUTF8(modified), pool),
					NULL, NULL, pool);
				svn_stream_close(stream);
			}
		}
		apr_file_close(outfile);
	}
	if (err)
	{
		if (bShowError)
			AfxMessageBox(CAppUtils::GetErrorString(err), MB_ICONERROR);
		svn_error_clear(err);
		svn_pool_destroy(pool);
		return false;
	}
	svn_pool_destroy(pool);
	return true;
}

CString CAppUtils::GetErrorString(svn_error_t * Err)
{
	CString msg;
	CString temp;
	char errbuf[256];

	if (Err != NULL)
	{
		svn_error_t * ErrPtr = Err;
		if (ErrPtr->message)
			msg = CUnicodeUtils::GetUnicode(ErrPtr->message);
		else
		{
			/* Is this a Subversion-specific error code? */
			if ((ErrPtr->apr_err > APR_OS_START_USEERR)
				&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
				msg = svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf));
			/* Otherwise, this must be an APR error code. */
			else
			{
				svn_error_t *temp_err = NULL;
				const char * err_string = NULL;
				temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
				if (temp_err)
				{
					svn_error_clear (temp_err);
					msg = _T("Can't recode error string from APR");
				}
				else
				{
					msg = CUnicodeUtils::GetUnicode(err_string);
				}
			}
		}
		while (ErrPtr->child)
		{
			ErrPtr = ErrPtr->child;
			msg += _T("\n");
			if (ErrPtr->message)
				temp = CUnicodeUtils::GetUnicode(ErrPtr->message);
			else
			{
				/* Is this a Subversion-specific error code? */
				if ((ErrPtr->apr_err > APR_OS_START_USEERR)
					&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
					temp = svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf));
				/* Otherwise, this must be an APR error code. */
				else
				{
					svn_error_t *temp_err = NULL;
					const char * err_string = NULL;
					temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
					if (temp_err)
					{
						svn_error_clear (temp_err);
						temp = _T("Can't recode error string from APR");
					}
					else
					{
						temp = CUnicodeUtils::GetUnicode(err_string);
					}
				}
			}
			msg += temp;
		}
		return msg;
	}
	return _T("");
}

bool CAppUtils::HasClipboardFormat(UINT format)
{
	if (OpenClipboard(NULL))
	{
		UINT enumFormat = 0;
		do 
		{
			if (enumFormat == format)
			{
				CloseClipboard();
				return true;
			}
		} while((enumFormat = EnumClipboardFormats(enumFormat))!=0);
		CloseClipboard();
	}
	return false;
}




