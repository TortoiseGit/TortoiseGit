#pragma once
#include "GitStatus.h"
#include "AtlTime.h"

typedef std::vector<git_revnum_t> GIT_REV_LIST;

#define LOG_REV_AUTHOR_NAME 	_T('0')
#define LOG_REV_AUTHOR_EMAIL 	_T('1')
#define LOG_REV_AUTHOR_DATE 	_T('2')
#define LOG_REV_COMMIT_NAME 	_T('3')
#define LOG_REV_COMMIT_EMAIL	_T('4')
#define LOG_REV_COMMIT_DATE		_T('5')
#define LOG_REV_COMMIT_SUBJECT	_T('6')
#define LOG_REV_COMMIT_BODY		_T('7')
#define LOG_REV_COMMIT_HASH		_T('8')
#define LOG_REV_COMMIT_PARENT   _T('9')
#define LOG_REV_COMMIT_FILE		_T('A')
#define LOG_REV_ITEM_BEGIN		_T('B')
#define LOG_REV_ITEM_END		_T('C')



class GitRev
{
public:
	GitRev(void);
//	GitRev(GitRev &rev);
//	GitRev &operator=(GitRev &rev);
	~GitRev(void);
	enum
	{
		REV_HEAD = -1,			///< head revision
		REV_BASE = -2,			///< base revision
		REV_WC = -3,			///< revision of the working copy
		REV_UNSPECIFIED = -4,	///< unspecified revision
	};
	static CString GetHead(){return CString(_T("HEAD"));};
	static CString GetWorkingCopy(){return CString(GIT_REV_ZERO);};
	CString m_AuthorName;
	CString m_AuthorEmail;
	CTime	m_AuthorDate;
	CString m_CommitterName;
	CString m_CommitterEmail;
	CTime m_CommitterDate;
	CString m_Subject;
	CString m_Body;
	git_revnum_t m_CommitHash;
	GIT_REV_LIST m_ParentHash;
	CTGitPathList m_Files;
	int	m_Action;
	int ParserFromLog(CString &log);
	CTime ConverFromString(CString input);
};
