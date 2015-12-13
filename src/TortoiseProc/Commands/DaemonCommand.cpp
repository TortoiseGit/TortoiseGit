// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2015 - TortoiseGit

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
#include "DaemonCommand.h"
#include "ProgressDlg.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"


bool DaemonCommand::Execute()
{
	if (CMessageBox::ShowCheck(NULL, IDS_DAEMON_SECURITY_WARN, IDS_APPNAME, 2, IDI_EXCLAMATION, IDS_PROCEEDBUTTON, IDS_ABORTBUTTON, NULL, _T("DaemonNoSecurityWarning"), IDS_MSGBOX_DONOTSHOWAGAIN) == 2)
	{
		CMessageBox::RemoveRegistryKey(_T("DaemonNoSecurityWarning")); // only store answer if it is "Proceed"
		return false;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		MessageBox(NULL, _T("WSAStartup failed!"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return false;
	}
	SCOPE_EXIT { WSACleanup(); };

	char hostName[128] = { 0 };
	if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("gethostname failed!"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return false;
	}

	STRING_VECTOR ips;
	ADDRINFOA addrinfo = { 0 };
	addrinfo.ai_family = AF_UNSPEC;
	PADDRINFOA result = nullptr;
	GetAddrInfoA(hostName, nullptr, &addrinfo, &result);
	for (auto ptr = result; ptr != nullptr; ptr = ptr->ai_next)
	{
		if (ptr->ai_family != AF_INET && ptr->ai_family != AF_INET6)
			continue;

		DWORD ipbufferlength = 46;
		CString ip;
		if (WSAAddressToString(ptr->ai_addr, (DWORD)ptr->ai_addrlen, nullptr, CStrBuf(ip, ipbufferlength), &ipbufferlength))
			continue;

		if (ptr->ai_family == AF_INET6)
		{
			if (ip.Left(5) == _T("fe80:")) // strip % interface number at the end
				ip = ip.Left(ip.Find(_T('%')));
			ip = _T('[') + ip; // IPv6 addresses needs to be enclosed within braces
			ip += _T(']');
		}
		ips.push_back(ip);
	}
	if (result)
		FreeAddrInfoA(result);

	CString basePath(g_Git.m_CurrentDir);
	basePath.TrimRight(L"\\");
	if (basePath.GetLength() == 2)
		basePath += L"\\.";

	CString cmd;
	cmd.Format(_T("git.exe daemon --verbose --export-all --base-path=\"%s\""), (LPCTSTR)basePath);
	CProgressDlg progDlg;
	progDlg.m_GitCmd = cmd;
	if (ips.empty())
		progDlg.m_PreText = _T("git://localhost/");
	else
	{
		for (const auto& ip : ips)
		{
			progDlg.m_PreText += _T("git://");
			progDlg.m_PreText += ip;
			progDlg.m_PreText += _T("/\n");
		}
	}
	progDlg.DoModal();
	return true;
}
