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

static UINT WM_GRAVATAR_REFRESH = RegisterWindowMessage(L"TORTOISEGIT_GRAVATAR_REFRESH");

static CString CalcHash(HCRYPTPROV hProv, const CString& text)
{
	static bool useMD5 = CRegDWORD(L"Software\\TortoiseGit\\GravatarUseMD5", FALSE) != FALSE;

	HCRYPTHASH hHash = 0;
	if (!CryptCreateHash(hProv, useMD5 ? CALG_MD5 : CALG_SHA_256, 0, 0, &hHash))
		return L"";
	SCOPE_EXIT { CryptDestroyHash(hHash); };

	CStringA textA = CUnicodeUtils::GetUTF8(text);
	if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(static_cast<LPCSTR>(textA)), textA.GetLength(), 0))
		return L"";

	DWORD hashLen;
	DWORD hashLenLen = sizeof(DWORD);
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashLen), &hashLenLen, 0))
		return L"";

	CString hash;
	auto pHash = std::make_unique<BYTE[]>(hashLen);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, pHash.get(), &hashLen, 0))
		return L"";

	for (const auto* it = pHash.get(); it < pHash.get() + hashLen; ++it)
		hash.AppendFormat(L"%02x", *it);

	return hash;
}

BEGIN_MESSAGE_MAP(CGravatar, CStatic)
	ON_REGISTERED_MESSAGE(WM_GRAVATAR_REFRESH, OnGravatarRefresh)
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
	CString gravatarBaseUrl = CRegString(L"Software\\TortoiseGit\\GravatarUrl", L"https://gravatar.com/avatar/%HASH%?d=identicon");

	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Failed to acquire crypto context\n");
		return;
	}
	SCOPE_EXIT { CryptReleaseContext(hProv, 0); };

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
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Gravatar URL seems to be invalid\n");
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
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": InternetConnect failed\n");
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

			CString hash = CalcHash(hProv, email);
			if (hash.IsEmpty())
				continue;

			CString gravatarUrl = baseUrlPath;
			gravatarUrl.Replace(L"%HASH%", hash);
			CString tempFile;
			GetTempPath(tempFile);
			tempFile += hash;
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
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Gravatar download for \"%s\" failed: %d\n", static_cast<LPCWSTR>(gravatarUrl), ret);
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
			PostMessage(WM_GRAVATAR_REFRESH);
		}
	}
}

int CGravatar::DownloadToFile(bool* gravatarExit, const HINTERNET hConnectHandle, bool isHttps, const CString& urlpath, const CString& dest) const
{
	if (!hConnectHandle)
		return ERROR_CANCELLED;

	HINTERNET hResourceHandle = HttpOpenRequest(hConnectHandle, nullptr, urlpath, nullptr, nullptr, nullptr, INTERNET_FLAG_KEEP_CONNECTION | (isHttps ? INTERNET_FLAG_SECURE : 0), 0);
	if (!hResourceHandle)
		return static_cast<int>(INET_E_DOWNLOAD_FAILURE);

	SCOPE_EXIT { InternetCloseHandle(hResourceHandle); };
resend:
	if (*gravatarExit)
		return ERROR_CANCELLED;

	BOOL httpsendrequest = HttpSendRequest(hResourceHandle, nullptr, 0, nullptr, 0);

	const DWORD dwError = InternetErrorDlg(GetSafeHwnd(), hResourceHandle, ERROR_SUCCESS, FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA, nullptr);

	if (dwError == ERROR_INTERNET_FORCE_RETRY)
		goto resend;
	else if (!httpsendrequest)
		return static_cast<int>(INET_E_DOWNLOAD_FAILURE);
	else if (*gravatarExit)
		return ERROR_CANCELLED;

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

LRESULT CGravatar::OnGravatarRefresh(WPARAM, LPARAM)
{
	Invalidate();
	return 0;
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
