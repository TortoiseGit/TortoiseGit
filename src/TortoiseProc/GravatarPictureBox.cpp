// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit

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
#include "stdafx.h"
#include "GravatarPictureBox.h"
#include "Picture.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "Git.h"
#include "LoglistCommonResource.h"
#include "resource.h"

static CString CalcMD5(CString text)
{
	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return _T("");

	HCRYPTHASH hHash = 0;
	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
	{
		CryptReleaseContext(hProv, 0);
		return _T("");
	}

	CStringA textA = CUnicodeUtils::GetUTF8(text);
	if (!CryptHashData(hHash, (LPBYTE)textA.GetBuffer(), textA.GetLength(), 0))
	{
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		return _T("");
	}

	CString hash;
	BYTE rgbHash[16];
	DWORD cbHash = _countof(rgbHash);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
		return _T("");
	for (DWORD i = 0; i < cbHash; i++)
	{
		BYTE hi = rgbHash[i] >> 4;
		BYTE lo = rgbHash[i]  & 0xf;
		hash.AppendChar(hi + (hi > 9 ? 87 : 48));
		hash.AppendChar(lo + (lo > 9 ? 87 : 48));
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	return hash;
}

BEGIN_MESSAGE_MAP(CGravatar, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

CGravatar::CGravatar()
	: CStatic()
	, m_gravatarEvent(INVALID_HANDLE_VALUE)
	, m_gravatarThread(nullptr)
	, m_gravatarExit(false)
	, m_bEnableGravatar(false)
{
	m_gravatarLock.Init();
}

CGravatar::~CGravatar()
{
	SafeTerminateGravatarThread();
	m_gravatarLock.Term();
	if (m_gravatarEvent)
		CloseHandle(m_gravatarEvent);
}

void CGravatar::Init()
{
	if (m_bEnableGravatar)
	{
		if (m_gravatarEvent == INVALID_HANDLE_VALUE)
			m_gravatarEvent = ::CreateEvent(nullptr, FALSE, TRUE, nullptr);
		if (m_gravatarThread == nullptr)
		{
			m_gravatarThread = AfxBeginThread([] (LPVOID lpVoid) -> UINT { ((CGravatar *)lpVoid)->GravatarThread(); return 0; }, this, THREAD_PRIORITY_BELOW_NORMAL);
			if (m_gravatarThread == nullptr)
			{
				CMessageBox::Show(nullptr, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
				return;
			}
		}
	}
}

void CGravatar::LoadGravatar(const CString& email)
{
	if (m_gravatarThread == nullptr)
		return;

	if (email.IsEmpty())
	{
		m_gravatarLock.Lock();
		if (!m_filename.IsEmpty())
		{
			m_filename = _T("");
			m_gravatarLock.Unlock();
			Invalidate();
		}
		else
			m_gravatarLock.Unlock();
		return;
	}

	m_gravatarLock.Lock();
	bool diff = m_email != email;
	m_email = email;
	m_gravatarLock.Unlock();
	if (diff)
		::SetEvent(m_gravatarEvent);
}

void CGravatar::GravatarThread()
{
	CString gravatarBaseUrl = CRegString(_T("Software\\TortoiseGit\\GravatarUrl"), _T("http://www.gravatar.com/avatar/%HASH%"));

	CString hostname;
	CString baseUrlPath;
	URL_COMPONENTS urlComponents = {0};
	urlComponents.dwStructSize = sizeof(urlComponents);
	urlComponents.lpszHostName = hostname.GetBufferSetLength(INTERNET_MAX_HOST_NAME_LENGTH);
	urlComponents.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
	urlComponents.lpszUrlPath = baseUrlPath.GetBufferSetLength(INTERNET_MAX_PATH_LENGTH);
	urlComponents.dwUrlPathLength = INTERNET_MAX_PATH_LENGTH;
	if (!InternetCrackUrl(gravatarBaseUrl, gravatarBaseUrl.GetLength(), 0, &urlComponents))
	{
		m_filename.Empty();
		return;
	}
	hostname.ReleaseBuffer();
	baseUrlPath.ReleaseBuffer();

	HINTERNET hOpenHandle = InternetOpen(L"TortoiseGit", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	HINTERNET hConnectHandle = InternetConnect(hOpenHandle, hostname, urlComponents.nPort, nullptr, nullptr, urlComponents.nScheme, 0, 0);
	if (!hConnectHandle)
	{
		InternetCloseHandle(hOpenHandle);
		m_filename.Empty();
		return;
	}

	while (!m_gravatarExit)
	{
		::WaitForSingleObject(m_gravatarEvent, INFINITE);
		while (!m_gravatarExit)
		{
			m_gravatarLock.Lock();
			CString email = m_email;
			m_gravatarLock.Unlock();
			if (email.IsEmpty())
				break;

			Sleep(500);
			if (m_gravatarExit)
				break;
			m_gravatarLock.Lock();
			bool diff = email != m_email;
			m_gravatarLock.Unlock();
			if (diff)
				continue;

			CString md5 = CalcMD5(email);
			if (md5.IsEmpty())
				continue;

			CString gravatarUrl = baseUrlPath;
			gravatarUrl.Replace(_T("%HASH%"), md5);
			CString tempFile;
			GetTempPath(tempFile);
			tempFile += md5;
			if (PathFileExists(tempFile))
			{
				m_gravatarLock.Lock();
				m_filename = tempFile;
				m_email = _T("");
				m_gravatarLock.Unlock();
			}
			else
			{
				BOOL ret = DownloadToFile(hConnectHandle, gravatarUrl, tempFile);
				if (ret)
				{
					DeleteFile(tempFile);
					m_gravatarLock.Lock();
					m_filename.Empty();
					m_gravatarLock.Unlock();
				}
				if (m_gravatarExit)
					break;
				m_gravatarLock.Lock();
				if (m_email == email && !ret)
				{
					HANDLE hFile = CreateFile(tempFile, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					FILETIME creationTime = {};
					GetFileTime(hFile, &creationTime, nullptr, nullptr);
					uint64_t delta = 7 * 24 * 60 * 60 * 10000000LL;
					DWORD low = creationTime.dwLowDateTime;
					creationTime.dwLowDateTime += delta & 0xffffffff;
					creationTime.dwHighDateTime += delta >> 32;
					if (creationTime.dwLowDateTime < low)
						creationTime.dwHighDateTime++;
					SetFileTime(hFile, &creationTime, nullptr, nullptr);
					CloseHandle(hFile);
					m_filename = tempFile;
					m_email = _T("");
				}
				else
					m_filename = _T("");
				m_gravatarLock.Unlock();
			}
			Invalidate();
		}
	}
	InternetCloseHandle(hConnectHandle);
	InternetCloseHandle(hOpenHandle);
}

BOOL CGravatar::DownloadToFile(const HINTERNET hConnectHandle, const CString& urlpath, const CString& dest)
{
	HINTERNET hResourceHandle = HttpOpenRequest(hConnectHandle, nullptr, urlpath, nullptr, nullptr, nullptr, INTERNET_FLAG_KEEP_CONNECTION, 0);
	if (!hResourceHandle)
		return -1;

resend:
	BOOL httpsendrequest = HttpSendRequest(hResourceHandle, nullptr, 0, nullptr, 0);

	DWORD dwError = InternetErrorDlg(GetSafeHwnd(), hResourceHandle, ERROR_SUCCESS, FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA, nullptr);

	if (dwError == ERROR_INTERNET_FORCE_RETRY)
		goto resend;
	else if (!httpsendrequest)
	{
		InternetCloseHandle(hResourceHandle);
		return INET_E_DOWNLOAD_FAILURE;
	}

	DWORD statusCode = 0;
	DWORD length = sizeof(statusCode);
	if (!HttpQueryInfo(hResourceHandle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&statusCode, &length, NULL) || statusCode != 200)
	{
		InternetCloseHandle(hResourceHandle);
		if (statusCode == 404)
			return ERROR_FILE_NOT_FOUND;
		else if (statusCode == 403)
			return ERROR_ACCESS_DENIED;
		return INET_E_DOWNLOAD_FAILURE;
	}

	CFile destinationFile(dest, CFile::modeCreate | CFile::modeWrite);
	DWORD downloadedSum = 0; // sum of bytes downloaded so far
	do
	{
		DWORD size; // size of the data available
		if (!InternetQueryDataAvailable(hResourceHandle, &size, 0, 0))
		{
			InternetCloseHandle(hResourceHandle);
			return INET_E_DOWNLOAD_FAILURE;
		}

		DWORD downloaded; // size of the downloaded data
		LPTSTR lpszData = new TCHAR[size + 1];
		if (!InternetReadFile(hResourceHandle, (LPVOID)lpszData, size, &downloaded))
		{
			delete[] lpszData;
			InternetCloseHandle(hResourceHandle);
			return INET_E_DOWNLOAD_FAILURE;
		}

		if (downloaded == 0)
		{
			delete[] lpszData;
			break;
		}

		lpszData[downloaded] = '\0';
		destinationFile.Write(lpszData, downloaded);
		delete[] lpszData;

		downloadedSum += downloaded;
	}
	while (!m_gravatarExit);
	destinationFile.Close();
	InternetCloseHandle(hResourceHandle);
	if (downloadedSum == 0)
		return INET_E_DOWNLOAD_FAILURE;

	return 0;
}

void CGravatar::SafeTerminateGravatarThread()
{
	m_gravatarExit = true;
	if (m_gravatarThread)
	{
		::SetEvent(m_gravatarEvent);
		::WaitForSingleObject(m_gravatarThread, 1000);
		m_gravatarThread = nullptr;
	}
}

void CGravatar::OnPaint()
{
	CPaintDC dc(this);
	RECT rect;
	GetClientRect(&rect);
	dc.FillSolidRect(&rect, GetSysColor(COLOR_BTNFACE));
	m_gravatarLock.Lock();
	CString filename = m_filename;
	m_gravatarLock.Unlock();
	if (filename.IsEmpty()) return;
	CPicture picture;
	picture.Load(filename.GetString());
	picture.Show(dc, rect);
}
