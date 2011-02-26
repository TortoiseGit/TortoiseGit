#pragma once

#include "hwsmtp.h"
#include "TGitPath.h"

#define REG_SMTP_SERVER _T("Software\\TortoiseGit\\EMAIL\\SMTPSERVER")
#define REG_SMTP_PORT _T("Software\\TortoiseGit\\EMAIL\\PORT")
#define REG_SMTP_ISAUTH _T("Software\\TortoiseGit\\EMAIL\\ISAUTH")
#define REG_SMTP_USER _T("Software\\TortoiseGit\\EMAIL\\USER")
#define REG_SMTP_PASSWORD _T("Software\\TortoiseGit\\EMAIL\\PASSWORD")

class CPatch
{
protected:
	void GetNameAddress(CString &in, CString &name,CString &address);

public:
	CPatch();
	~CPatch(void);
	int Parser(CString &pathfile);
	int Send(CString &pathfile,CString &To, CString &CC,bool bAttachment, bool useMAPI);

	static int SendPatchesCombined(CTGitPathList &list,CString &To,CString &CC, CString &subject,bool bAttachment, bool useMAPI,CString *errortext);

	CString		m_LastError;
	CString		m_Author;
	CString		m_Date;
	CString		m_Subject;
	CString		m_PathFile;
	CStringA	m_Body;
	CString		m_strBody;

private:
	void ConvertToArray(CString &to, CStringArray &Array);
	static int SendMail(CString &To, CString &CC, CString &subject, CString &body, CStringArray &attachments, bool useMAPI, CString *errortext);
};
