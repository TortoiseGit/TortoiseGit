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

void CGravatar::LoadGravatar(CString email)
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
	CoInitialize(nullptr);
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

			CString gravatarUrl = gravatarBaseUrl;
			gravatarUrl.Replace(_T("%HASH%"), md5);
			CString tempFile;
			GetTempPath(tempFile);
			tempFile += md5;
			if (PathFileExists(tempFile))
			{
				m_filename = tempFile;
				m_email = _T("");
			}
			else
			{
				HRESULT res = URLDownloadToFile(nullptr, gravatarUrl, tempFile, 0, nullptr);
				if (m_gravatarExit)
					break;
				m_gravatarLock.Lock();
				if (m_email == email && res == S_OK)
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
	CoUninitialize();
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
