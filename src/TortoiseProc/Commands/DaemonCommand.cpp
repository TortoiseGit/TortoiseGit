// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2016, 2018-2019 - TortoiseGit

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
	if (!GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOGITREPO, IDS_APPNAME, MB_ICONERROR);
		return false;
	}
	if (CMessageBox::ShowCheck(GetExplorerHWND(), IDS_DAEMON_SECURITY_WARN, IDS_APPNAME, 2, IDI_EXCLAMATION, IDS_PROCEEDBUTTON, IDS_ABORTBUTTON, NULL, L"DaemonNoSecurityWarning", IDS_MSGBOX_DONOTSHOWAGAIN) == 2)
	{
		CMessageBox::RemoveRegistryKey(L"DaemonNoSecurityWarning"); // only store answer if it is "Proceed"
		return false;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		MessageBox(GetExplorerHWND(), L"WSAStartup failed!", L"TortoiseGit", MB_OK | MB_ICONERROR);
		return false;
	}
	SCOPE_EXIT { WSACleanup(); };

	char hostName[128] = { 0 };
	if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
	{
		MessageBox(GetExplorerHWND(), L"gethostname failed!", L"TortoiseGit", MB_OK | MB_ICONERROR);
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
		if (WSAAddressToString(ptr->ai_addr, static_cast<DWORD>(ptr->ai_addrlen), nullptr, CStrBuf(ip, ipbufferlength), &ipbufferlength))
			continue;

		if (ptr->ai_family == AF_INET6)
		{
			if (CStringUtils::StartsWith(ip, L"fe80:")) // strip % interface number at the end
				ip = ip.Left(ip.Find(L'%'));
			ip = L'[' + ip; // IPv6 addresses needs to be enclosed within braces
			ip += L']';
		}
		ips.push_back(ip);
	}
	if (result)
		FreeAddrInfoA(result);

	CString basePath(g_Git.m_CurrentDir);
	basePath.TrimRight(L'\\');
	if (basePath.GetLength() == 2)
		basePath += L"\\.";

	CString cmd;
	cmd.Format(L"git.exe daemon --verbose --export-all --base-path=\"%s\"", static_cast<LPCTSTR>(basePath));
	CProgressDlg progDlg;
	theApp.m_pMainWnd = &progDlg;
	progDlg.m_GitCmd = cmd;
	if (ips.empty())
		progDlg.m_PreText = L"git://localhost/";
	else
	{
		for (const auto& ip : ips)
		{
			progDlg.m_PreText += L"git://";
			progDlg.m_PreText += ip;
			progDlg.m_PreText += L"/\n";
		}
	}
	progDlg.DoModal();
	return true;
}
