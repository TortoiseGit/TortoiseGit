#pragma once

class GitConfig
{
public:
	GitConfig(void);
	~GitConfig(void);
};

#define REG_MSYSGIT_PATH _T("Software\\TortoiseGit\\MSysGit")
#define REG_MSYSGIT_EXTRA_PATH _T("Software\\TortoiseGit\\MSysGitExtra")

#ifndef WIN64
#define REG_MSYSGIT_INSTALL _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1\\InstallLocation")
#else
#define REG_MSYSGIT_INSTALL	_T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1\\InstallLocation")
#endif