// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012, 2014-2019 - TortoiseGit
// Copyright (C) 2003-2006, 2009, 2015 - TortoiseSVN

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
#include "Cache.h"
#include "DirFileEnum.h"
#include "CacheInterface.h"
#include <WinInet.h>
#include "CacheDlg.h"
#include <random>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CCacheDlg::CCacheDlg(CWnd* pParent /*=nullptr*/)
: CDialog(CCacheDlg::IDD, pParent)
, m_hPipe(INVALID_HANDLE_VALUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCacheDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ROOTPATH, m_sRootPath);
}

BEGIN_MESSAGE_MAP(CCacheDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_WATCHTESTBUTTON, OnBnClickedWatchtestbutton)
END_MESSAGE_MAP()


// CCacheDlg message handlers

BOOL CCacheDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCacheDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCacheDlg::OnQueryDragIcon()
{
	return m_hIcon;
}

void CCacheDlg::OnBnClickedOk()
{
	UpdateData();
	AfxBeginThread(TestThreadEntry, this);
}
UINT CCacheDlg::TestThreadEntry(LPVOID pVoid)
{
	return static_cast<CCacheDlg*>(pVoid)->TestThread();
}

//this is the thread function which calls the subversion function
UINT CCacheDlg::TestThread()
{
	CDirFileEnum direnum(m_sRootPath);
	m_filelist.RemoveAll();
	CString filepath;
	bool bIsDir = false;
	while (direnum.NextFile(filepath, &bIsDir))
		if (filepath.Find(L".git") < 0)
			m_filelist.Add(filepath);

	CTime starttime = CTime::GetCurrentTime();
	GetDlgItem(IDC_STARTTIME)->SetWindowText(starttime.Format(L"%H:%M:%S"));

	ULONGLONG startticks = GetTickCount64();

	CString sNumber;
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<INT_PTR> dist(0, max(INT_PTR(0), m_filelist.GetCount() - 1));
	std::uniform_int_distribution<INT_PTR> dist2(0, 9);
	for (int i=0; i < 1; ++i)
	{
		CString filepath2;
		//do {
			filepath2 = m_filelist.GetAt(dist(mt));
		//}while(filepath.Find(L".git") >= 0);
		GetDlgItem(IDC_FILEPATH)->SetWindowText(filepath2);
		GetStatusFromRemoteCache(CTGitPath(filepath2), true);
		sNumber.Format(L"%d", i);
		GetDlgItem(IDC_DONE)->SetWindowText(sNumber);
		if ((GetTickCount64()%10)==1)
			Sleep(10);
		if (dist2(mt) == 3)
			RemoveFromCache(filepath2);
	}
	CTime endtime = CTime::GetCurrentTime();
	CString sEnd = endtime.Format(L"%H:%M:%S");

	ULONGLONG endticks = GetTickCount64();

	CString sEndText;
	sEndText.Format(L"%s  - %I64u ms", static_cast<LPCTSTR>(sEnd), endticks - startticks);

	GetDlgItem(IDC_ENDTIME)->SetWindowText(sEndText);

	return 0;
}


bool CCacheDlg::EnsurePipeOpen()
{
	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		return true;
	}

	m_hPipe = CreateFile(
		GetCachePipeName(),   // pipe name
		GENERIC_READ |					// read and write access
		GENERIC_WRITE,
		0,								// no sharing
		nullptr,						// default security attributes
		OPEN_EXISTING,				// opens existing pipe
		FILE_FLAG_OVERLAPPED,			// default attributes
		nullptr);						// no template file

	if (m_hPipe == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PIPE_BUSY)
	{
		// TSVNCache is running but is busy connecting a different client.
		// Do not give up immediately but wait for a few milliseconds until
		// the server has created the next pipe instance
		if (WaitNamedPipe(GetCachePipeName(), 50))
		{
			m_hPipe = CreateFile(
				GetCachePipeName(),   // pipe name
				GENERIC_READ |					// read and write access
				GENERIC_WRITE,
				0,								// no sharing
				nullptr,						// default security attributes
				OPEN_EXISTING,				// opens existing pipe
				FILE_FLAG_OVERLAPPED,			// default attributes
				nullptr);						// no template file
		}
	}


	if (m_hPipe != INVALID_HANDLE_VALUE)
	{
		// The pipe connected; change to message-read mode.
		DWORD dwMode;

		dwMode = PIPE_READMODE_MESSAGE;
		if(!SetNamedPipeHandleState(
			m_hPipe,    // pipe handle
			&dwMode,  // new pipe mode
			nullptr,  // don't set maximum bytes
			nullptr)) // don't set maximum time
		{
			ATLTRACE("SetNamedPipeHandleState failed");
			CloseHandle(m_hPipe);
			m_hPipe = INVALID_HANDLE_VALUE;
			return false;
		}
		// create an unnamed (=local) manual reset event for use in the overlapped structure
		m_hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (m_hEvent)
			return true;
		ATLTRACE("CreateEvent failed");
		ClosePipe();
		return false;
	}

	return false;
}


void CCacheDlg::ClosePipe()
{
	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hPipe);
		CloseHandle(m_hEvent);
		m_hPipe = INVALID_HANDLE_VALUE;
		m_hEvent = INVALID_HANDLE_VALUE;
	}
}

bool CCacheDlg::GetStatusFromRemoteCache(const CTGitPath& Path, bool bRecursive)
{
	if(!EnsurePipeOpen())
	{
		STARTUPINFO startup = { 0 };
		PROCESS_INFORMATION process = { 0 };
		startup.cb = sizeof(startup);

		CString sCachePath = L"TGitCache.exe";
		if (CreateProcess(sCachePath.GetBuffer(sCachePath.GetLength() + 1), L"", nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process) == 0)
		{
			// It's not appropriate to do a message box here, because there may be hundreds of calls
			sCachePath.ReleaseBuffer();
			ATLTRACE("Failed to start cache\n");
			return false;
		}
		sCachePath.ReleaseBuffer();

		// Wait for the cache to open
		ULONGLONG endTime = GetTickCount64()+1000;
		while(!EnsurePipeOpen())
		{
			if((GetTickCount64() - endTime) > 0)
			{
				return false;
			}
		}
	}


	DWORD nBytesRead;
	TGITCacheRequest request;
	request.flags = TGITCACHE_FLAGS_NONOTIFICATIONS;
	if(bRecursive)
	{
		request.flags |= TGITCACHE_FLAGS_RECUSIVE_STATUS;
	}
	wcsncpy_s(request.path, Path.GetWinPath(), MAX_PATH);
	SecureZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));
	m_Overlapped.hEvent = m_hEvent;
	// Do the transaction in overlapped mode.
	// That way, if anything happens which might block this call
	// we still can get out of it. We NEVER MUST BLOCK THE SHELL!
	// A blocked shell is a very bad user impression, because users
	// who don't know why it's blocked might find the only solution
	// to such a problem is a reboot and therefore they might loose
	// valuable data.
	// Sure, it would be better to have no situations where the shell
	// even can get blocked, but the timeout of 5 seconds is long enough
	// so that users still recognize that something might be wrong and
	// report back to us so we can investigate further.

	TGITCacheResponse ReturnedStatus;
	BOOL fSuccess = TransactNamedPipe(m_hPipe,
		&request, sizeof(request),
		&ReturnedStatus, sizeof(ReturnedStatus),
		&nBytesRead, &m_Overlapped);

	if (!fSuccess)
	{
		if (GetLastError()!=ERROR_IO_PENDING)
		{
			ClosePipe();
			return false;
		}

		// TransactNamedPipe is working in an overlapped operation.
		// Wait for it to finish
		DWORD dwWait = WaitForSingleObject(m_hEvent, INFINITE);
		if (dwWait == WAIT_OBJECT_0)
		{
			GetOverlappedResult(m_hPipe, &m_Overlapped, &nBytesRead, FALSE);
			return TRUE;
		}
	}

	ClosePipe();
	return false;
}

void CCacheDlg::RemoveFromCache(const CString& path)
{
	// if we use the external cache, we tell the cache directly that something
	// has changed, without the detour via the shell.
	HANDLE hPipe = CreateFile(
		GetCacheCommandPipeName(),	// pipe name
		GENERIC_READ |					// read and write access
		GENERIC_WRITE,
		0,								// no sharing
		nullptr,						// default security attributes
		OPEN_EXISTING,					// opens existing pipe
		FILE_FLAG_OVERLAPPED,			// default attributes
		nullptr);						// no template file


	if (hPipe != INVALID_HANDLE_VALUE)
	{
		// The pipe connected; change to message-read mode.
		DWORD dwMode;

		dwMode = PIPE_READMODE_MESSAGE;
		if(SetNamedPipeHandleState(
			hPipe,    // pipe handle
			&dwMode,  // new pipe mode
			NULL,     // don't set maximum bytes
			NULL))    // don't set maximum time
		{
			DWORD cbWritten;
			TGITCacheCommand cmd;
			cmd.command = TGITCACHECOMMAND_CRAWL;
			wcsncpy_s(cmd.path, path, MAX_PATH);
			BOOL fSuccess = WriteFile(
				hPipe,			// handle to pipe
				&cmd,			// buffer to write from
				sizeof(cmd),	// number of bytes to write
				&cbWritten,		// number of bytes written
				NULL);			// not overlapped I/O

			if (! fSuccess || sizeof(cmd) != cbWritten)
			{
				DisconnectNamedPipe(hPipe);
				CloseHandle(hPipe);
				hPipe = INVALID_HANDLE_VALUE;
			}
			if (hPipe != INVALID_HANDLE_VALUE)
			{
				// now tell the cache we don't need it's command thread anymore
				DWORD cbWritten2;
				TGITCacheCommand cmd2;
				cmd2.command = TGITCACHECOMMAND_END;
				WriteFile(
					hPipe,			// handle to pipe
					&cmd2,			// buffer to write from
					sizeof(cmd2),	// number of bytes to write
					&cbWritten2,	// number of bytes written
					NULL);			// not overlapped I/O
				DisconnectNamedPipe(hPipe);
				CloseHandle(hPipe);
				hPipe = INVALID_HANDLE_VALUE;
			}
		}
		else
		{
			ATLTRACE("SetNamedPipeHandleState failed");
			CloseHandle(hPipe);
		}
	}
}
void CCacheDlg::OnBnClickedWatchtestbutton()
{
	UpdateData();
	AfxBeginThread(WatchTestThreadEntry, this);
}

UINT CCacheDlg::WatchTestThreadEntry(LPVOID pVoid)
{
	return static_cast<CCacheDlg*>(pVoid)->WatchTestThread();
}

//this is the thread function which calls the subversion function
UINT CCacheDlg::WatchTestThread()
{
	CDirFileEnum direnum(m_sRootPath);
	m_filelist.RemoveAll();
	CString filepath;
	bool bIsDir = false;
	while (direnum.NextFile(filepath, &bIsDir))
		m_filelist.Add(filepath);

	CTime starttime = CTime::GetCurrentTime();
	GetDlgItem(IDC_STARTTIME)->SetWindowText(starttime.Format(L"%H:%M:%S"));

	ULONGLONG startticks = GetTickCount64();

	CString sNumber;
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<INT_PTR> dist(0, max(INT_PTR(0), m_filelist.GetCount() - 1));
	filepath = m_filelist.GetAt(dist(mt));
	GetStatusFromRemoteCache(CTGitPath(m_sRootPath), false);
	for (int i=0; i < 10000; ++i)
	{
		filepath = m_filelist.GetAt(dist(mt));
		GetDlgItem(IDC_FILEPATH)->SetWindowText(filepath);
		TouchFile(filepath);
		CopyRemoveCopy(filepath);
		sNumber.Format(L"%d", i);
		GetDlgItem(IDC_DONE)->SetWindowText(sNumber);
	}

	// create dummy directories and remove them again several times
	for (int outer = 0; outer<100; ++outer)
	{
		for (int i=0; i<10; ++i)
		{
			filepath.Format(L"__MyDummyFolder%d", i);
			CreateDirectory(m_sRootPath + L'\\' + filepath, nullptr);
			HANDLE hFile = CreateFile(m_sRootPath + L'\\' + filepath + L"\\file", GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			CloseHandle(hFile);
			SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, m_sRootPath + L'\\' + filepath + L"\\file", NULL);
		}
		Sleep(500);
		for (int i=0; i<10; ++i)
		{
			filepath.Format(L"__MyDummyFolder%d", i);
			DeleteFile(m_sRootPath + L'\\' + filepath + L"\\file");
			RemoveDirectory(m_sRootPath + L'\\' + filepath);
		}
		sNumber.Format(L"%d", outer);
		GetDlgItem(IDC_DONE)->SetWindowText(sNumber);
	}

	CTime endtime = CTime::GetCurrentTime();
	CString sEnd = endtime.Format(L"%H:%M:%S");

	ULONGLONG endticks = GetTickCount64();

	CString sEndText;
	sEndText.Format(L"%s  - %I64u ms", static_cast<LPCTSTR>(sEnd), endticks - startticks);

	GetDlgItem(IDC_ENDTIME)->SetWindowText(sEndText);

	return 0;
}

void CCacheDlg::TouchFile(const CString& path)
{
	SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);
	HANDLE hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	FILETIME ft;
	SYSTEMTIME st;

	GetSystemTime(&st);              // gets current time
	SystemTimeToFileTime(&st, &ft);  // converts to file time format
	SetFileTime(hFile,           // sets last-write time for file
		nullptr, nullptr, &ft);

	CloseHandle(hFile);
}

void CCacheDlg::CopyRemoveCopy(const CString& path)
{
	if (PathIsDirectory(path))
		return;
	if (path.Find(L".git") >= 0)
		return;
	if (CopyFile(path, path+L".tmp", FALSE))
	{
		if (DeleteFile(path))
		{
			if (MoveFile(path+L".tmp", path))
				return;
			else
				MessageBox(L"could not move file!", path);
		}
		else
			MessageBox(L"could not delete file!", path);
	}
	else
		MessageBox(L"could not copy file!", path);
}
