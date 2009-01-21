#pragma once

class GitConfig
{
public:
	GitConfig(void);
	~GitConfig(void);
};

#define REG_MSYSGIT_PATH _T("Software\\TortoiseGit\\MSysGit")
#define REG_MSYSGIT_INSTALL _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1\\InstallLocation")