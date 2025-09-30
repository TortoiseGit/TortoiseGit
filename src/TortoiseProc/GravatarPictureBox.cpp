// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2017, 2019, 2021, 2023, 2025 - TortoiseGit

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
#include <WinCrypt.h>
#include "SmartHandle.h"

using CAutoLocker = CComCritSecLock<CComCriticalSection>;

static CString CalcMD5(CString text)
{
	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return L"";
	SCOPE_EXIT { CryptReleaseContext(hProv, 0); };

	HCRYPTHASH hHash = 0;
	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
		return L"";
	SCOPE_EXIT { CryptDestroyHash(hHash); };

	CStringA textA = CUnicodeUtils::GetUTF8(text);
	if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(static_cast<LPCSTR>(textA)), textA.GetLength(), 0))
		return L"";

	CString hash;
	BYTE rgbHash[16];
	DWORD cbHash = _countof(rgbHash);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
		return L"";

	for (DWORD i = 0; i < cbHash; i++)
	{
		BYTE hi = rgbHash[i] >> 4;
		BYTE lo = rgbHash[i]  & 0xf;
		hash.AppendChar(hi + (hi > 9 ? 87 : 48));
		hash.AppendChar(lo + (lo > 9 ? 87 : 48));
	}

	return hash;
}

BEGIN_MESSAGE_MAP(CGravatar, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

CGravatar::CGravatar()
	: CStatic()
{
}

CGravatar::~CGravatar()
{
	SafeTerminateGravatarThread();
	if (m_gravatarEvent != INVALID_HANDLE_VALUE)
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
			m_gravatarExit = new bool(false);
			m_gravatarThread = AfxBeginThread([](LPVOID lpVoid) -> UINT { reinterpret_cast<CGravatar*>(lpVoid)->GravatarThread(); return 0; }, this, THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
			if (m_gravatarThread == nullptr)
			{
				CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
				delete m_gravatarExit;
				m_gravatarExit = nullptr;
				return;
			}
			m_gravatarThread->m_bAutoDelete = FALSE;
			m_gravatarThread->ResumeThread();
		}
	}
}

void CGravatar::LoadGravatar(CString email)
{
	if (m_gravatarThread == nullptr)
		return;

	if (email.IsEmpty())
	{
		m_gravatarLock.Lock();
		m_email.Empty();
		if (!m_filename.IsEmpty())
		{
			m_filename.Empty();
			m_gravatarLock.Unlock();
			Invalidate();
		}
		else
			m_gravatarLock.Unlock();
		return;
	}

	email.Trim();
	email.MakeLower();
	m_gravatarLock.Lock();
	bool diff = m_email != email;
	m_email = email;
	m_gravatarLock.Unlock();
	if (diff)
		::SetEvent(m_gravatarEvent);
}

void CGravatar::GravatarThread()
{
	bool *gravatarExit = m_gravatarExit;
	CString gravatarBaseUrl = CRegString(L"Software\\TortoiseGit\\GravatarUrl", L"http://www.gravatar.com/avatar/%HASH%?d=identicon");

	CString hostname;
	CString baseUrlPath;
	URL_COMPONENTS urlComponents = {0};
	urlComponents.dwStructSize = sizeof(urlComponents);
	urlComponents.lpszHostName = hostname.GetBuffer(INTERNET_MAX_HOST_NAME_LENGTH);
	urlComponents.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
	urlComponents.lpszUrlPath = baseUrlPath.GetBuffer(INTERNET_MAX_PATH_LENGTH);
	urlComponents.dwUrlPathLength = INTERNET_MAX_PATH_LENGTH;
	if (!InternetCrackUrl(gravatarBaseUrl, gravatarBaseUrl.GetLength(), 0, &urlComponents))
	{
		CAutoLocker lock(m_gravatarLock);
		m_filename.Empty();
		return;
	}
	hostname.ReleaseBuffer();
	baseUrlPath.ReleaseBuffer();

	HINTERNET hOpenHandle = InternetOpen(L"TortoiseGit", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	SCOPE_EXIT { InternetCloseHandle(hOpenHandle); };
	bool isHttps = urlComponents.nScheme == INTERNET_SCHEME_HTTPS;
	m_hConnectHandle = InternetConnect(hOpenHandle, hostname, urlComponents.nPort, nullptr, nullptr, isHttps ? INTERNET_SCHEME_HTTP : urlComponents.nScheme, 0, 0);
	if (!m_hConnectHandle)
	{
		CAutoLocker lock(m_gravatarLock);
		m_filename.Empty();
		return;
	}
	SCOPE_EXIT
	{
		CAutoLocker lock(m_gravatarLock);
		if (m_hConnectHandle)
			InternetCloseHandle(m_hConnectHandle);
		m_hConnectHandle = nullptr;
	};

	while (!*gravatarExit)
	{
		::WaitForSingleObject(m_gravatarEvent, INFINITE);
		while (!*gravatarExit)
		{
			m_gravatarLock.Lock();
			CString email = m_email;
			m_gravatarLock.Unlock();
			if (email.IsEmpty())
				break;

			Sleep(500);
			if (*gravatarExit)
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
			gravatarUrl.Replace(L"%HASH%", md5);
			CString tempFile;
			GetTempPath(tempFile);
			tempFile += md5;
			if (PathFileExists(tempFile))
			{
				CAutoLocker lock(m_gravatarLock);
				if (m_email == email)
				{
					m_filename = tempFile;
					m_email.Empty();
				}
				else
					m_filename.Empty();
			}
			else
			{
				const int ret = DownloadToFile(gravatarExit, [&]() { CAutoLocker lock(m_gravatarLock); return m_hConnectHandle; }(), isHttps, gravatarUrl, tempFile);
				if (ret)
				{
					DeleteFile(tempFile);
					if (*gravatarExit)
						break;
					CAutoLocker lock(m_gravatarLock);
					m_filename.Empty();
				}
				if (*gravatarExit)
					break;
				CAutoLocker lock(m_gravatarLock);
				if (m_email == email && !ret)
				{
					CAutoFile hFile = CreateFile(tempFile, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
					FILETIME creationTime = {};
					GetFileTime(hFile, &creationTime, nullptr, nullptr);
					ULARGE_INTEGER sum;
					sum.LowPart = creationTime.dwLowDateTime;
					sum.HighPart = creationTime.dwHighDateTime;
					sum.QuadPart += 7 * 24 * 60 * 60 * 10000000LL;
					creationTime.dwLowDateTime = sum.LowPart;
					creationTime.dwHighDateTime = sum.HighPart;
					SetFileTime(hFile, &creationTime, nullptr, nullptr);
					m_filename = tempFile;
					m_email.Empty();
				}
				else
					m_filename.Empty();
			}
			if (*gravatarExit)
				break;
			Invalidate();
		}
	}
}

int CGravatar::DownloadToFile(bool* gravatarExit, const HINTERNET hConnectHandle, bool isHttps, const CString& urlpath, const CString& dest)
{
	if (!hConnectHandle)
		return ERROR_CANCELLED;

	HINTERNET hResourceHandle = HttpOpenRequest(hConnectHandle, nullptr, urlpath, nullptr, nullptr, nullptr, INTERNET_FLAG_KEEP_CONNECTION | (isHttps ? INTERNET_FLAG_SECURE : 0), 0);
	if (!hResourceHandle)
		return -1;

	SCOPE_EXIT { InternetCloseHandle(hResourceHandle); };
resend:
	if (*gravatarExit)
		return static_cast<int>(INET_E_DOWNLOAD_FAILURE);

	BOOL httpsendrequest = HttpSendRequest(hResourceHandle, nullptr, 0, nullptr, 0);

	const DWORD dwError = InternetErrorDlg(GetSafeHwnd(), hResourceHandle, ERROR_SUCCESS, FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA, nullptr);

	if (dwError == ERROR_INTERNET_FORCE_RETRY)
		goto resend;
	else if (!httpsendrequest || *gravatarExit)
		return static_cast<int>(INET_E_DOWNLOAD_FAILURE);

	DWORD statusCode = 0;
	DWORD length = sizeof(statusCode);
	if (!HttpQueryInfo(hResourceHandle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &length, nullptr) || statusCode != 200)
	{
		if (statusCode == 404)
			return ERROR_FILE_NOT_FOUND;
		else if (statusCode == 403)
			return ERROR_ACCESS_DENIED;
		return static_cast<int>(INET_E_DOWNLOAD_FAILURE);
	}

	CFile destinationFile;
	if (!destinationFile.Open(dest, CFile::modeCreate | CFile::modeWrite))
		return ERROR_OPEN_FAILED;

	DWORD downloadedSum = 0; // sum of bytes downloaded so far
	constexpr DWORD BUFFER_SIZE = 65536;
	auto buff = std::make_unique<wchar_t[]>(BUFFER_SIZE);
	while (!*gravatarExit)
	{
		DWORD size; // size of the data available
		if (!InternetQueryDataAvailable(hResourceHandle, &size, 0, 0))
			return static_cast<int>(INET_E_DOWNLOAD_FAILURE);

		DWORD downloaded; // size of the downloaded data
		if (!InternetReadFile(hResourceHandle, buff.get(), min(BUFFER_SIZE, size), &downloaded))
			return static_cast<int>(INET_E_DOWNLOAD_FAILURE);

		if (downloaded == 0)
		{
			if (downloadedSum == 0)
				return static_cast<int>(INET_E_DOWNLOAD_FAILURE);
			return 0;
		}

		try
		{
			destinationFile.Write(buff.get(), downloaded);
		}
		catch (CFileException* e)
		{
			auto ret = e->m_lOsError;
			if (e->m_cause == CFileException::diskFull)
				ret = ERROR_DISK_FULL;
			e->Delete();
			return ret;
		}

		if (DWordAdd(downloadedSum, downloaded, &downloadedSum) != S_OK)
			downloadedSum = DWORD_MAX;
	}

	return ERROR_CANCELLED;
}

void CGravatar::SafeTerminateGravatarThread()
{
	if (m_gravatarExit != nullptr)
		*m_gravatarExit = true;
	if (m_gravatarThread)
	{
		::SetEvent(m_gravatarEvent);
		if (::WaitForSingleObject(m_gravatarThread->m_hThread, 1500) == WAIT_TIMEOUT && m_hConnectHandle)
		{
			CAutoLocker lock(m_gravatarLock);
			if (m_hConnectHandle)
				::InternetCloseHandle(m_hConnectHandle);
			m_hConnectHandle = nullptr;
		}
		while (::WaitForSingleObject(m_gravatarThread->m_hThread, 1000) == WAIT_TIMEOUT)
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Waiting for Gravatar download thread to exit...\n");
		delete m_gravatarThread;
		m_gravatarThread = nullptr;
	}
	delete m_gravatarExit;
	m_gravatarExit = nullptr;
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
