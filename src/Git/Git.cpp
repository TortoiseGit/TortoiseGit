#include "StdAfx.h"
#include "Git.h"
#include "atlconv.h"
#include "GitRev.h"
#include "registry.h"
#include "GitConfig.h"
#include <map>
#include "UnicodeUtils.h"


static LPTSTR nextpath(LPCTSTR src, LPTSTR dst, UINT maxlen)
{
	LPCTSTR orgsrc;

	while (*src == _T(';'))
		src++;

	orgsrc = src;

	if (!--maxlen)
		goto nullterm;

	while (*src && *src != _T(';'))
	{
		if (*src != _T('"'))
		{
			*dst++ = *src++;
			if (!--maxlen)
			{
				orgsrc = src;
				goto nullterm;
			}
		}
		else
		{
			src++;
			while (*src && *src != _T('"'))
			{
				*dst++ = *src++;
				if (!--maxlen)
				{
					orgsrc = src;
					goto nullterm;
				}
			}

			if (*src)
				src++;
		}
	}

	while (*src == _T(';'))
		src++;

nullterm:

	*dst = 0;

	return (orgsrc != src) ? (LPTSTR)src : NULL;
}

static inline BOOL FileExists(LPCTSTR lpszFileName)
{
	struct _stat st;
	return _tstat(lpszFileName, &st) == 0;
}

static BOOL FindGitPath()
{
	size_t size;
	_tgetenv_s(&size, NULL, 0, _T("PATH"));

	if (!size)
	{
		return FALSE;
	}

	TCHAR *env = (TCHAR*)alloca(size * sizeof(TCHAR));
	_tgetenv_s(&size, env, size, _T("PATH"));

	TCHAR buf[_MAX_PATH];

	// search in all paths defined in PATH
	while ((env = nextpath(env, buf, _MAX_PATH-1)) && *buf)
	{
		TCHAR *pfin = buf + _tcslen(buf)-1;

		// ensure trailing slash
		if (*pfin != _T('/') && *pfin != _T('\\'))
			_tcscpy(++pfin, _T("\\"));

		const int len = _tcslen(buf);

		if ((len + 7) < _MAX_PATH)
			_tcscpy(pfin+1, _T("git.exe"));
		else
			break;

		if ( FileExists(buf) )
		{
			// dir found
			pfin[1] = 0;
			CGit::ms_LastMsysGitDir = buf;
			return TRUE;
		}
	}

	return FALSE;
}


#define MAX_DIRBUFFER 1000
#define CALL_OUTPUT_READ_CHUNK_SIZE 1024

CString CGit::ms_LastMsysGitDir;
CGit g_Git;

// contains system environment that should be used by child processes (RunAsync)
// initialized by CheckMsysGitDir
static LPTSTR l_processEnv = NULL;



CGit::CGit(void)
{
	GetCurrentDirectory(MAX_DIRBUFFER,m_CurrentDir.GetBuffer(MAX_DIRBUFFER));
	m_CurrentDir.ReleaseBuffer();

	CheckMsysGitDir();
}

CGit::~CGit(void)
{
}

static char g_Buffer[4096];

int CGit::RunAsync(CString cmd,PROCESS_INFORMATION *piOut,HANDLE *hReadOut,CString *StdioFile)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;
	HANDLE hStdioFile = NULL;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor=NULL;
	sa.bInheritHandle=TRUE;
	if(!CreatePipe(&hRead,&hWrite,&sa,0))
	{
		return GIT_ERROR_OPEN_PIP;
	}
	
	if(StdioFile)
	{
		hStdioFile=CreateFile(*StdioFile,GENERIC_WRITE,FILE_SHARE_READ   |   FILE_SHARE_WRITE,   
			&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);  
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb=sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	si.hStdError=hWrite;
	if(StdioFile)
		si.hStdOutput=hStdioFile;
	else
		si.hStdOutput=hWrite;

	si.wShowWindow=SW_HIDE;
	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;

	LPTSTR pEnv = l_processEnv;
	DWORD dwFlags = pEnv ? CREATE_UNICODE_ENVIRONMENT : 0;
	
	//DETACHED_PROCESS make ssh recognize that it has no console to launch askpass to input password. 
	dwFlags |= DETACHED_PROCESS; 

	if(!CreateProcess(NULL,(LPWSTR)cmd.GetString(), NULL,NULL,TRUE,dwFlags,pEnv,(LPWSTR)m_CurrentDir.GetString(),&si,&pi))
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,NULL);
		return GIT_ERROR_CREATE_PROCESS;
	}
	
	CloseHandle(hWrite);
	if(piOut)
		*piOut=pi;
	if(hReadOut)
		*hReadOut=hRead;
	
	return 0;

}
//Must use sperate function to convert ANSI str to union code string
//Becuase A2W use stack as internal convert buffer. 
void CGit::StringAppend(CString *str,BYTE *p,int code,int length)
{
     //USES_CONVERSION;
	 //str->Append(A2W_CP((LPCSTR)p,code));
	if(str == NULL)
		return ;

	WCHAR * buf;

	int len ;
	if(length<0)
		len= strlen((const char*)p);
	else
		len=length;
	//if (len==0)
	//	return ;
	//buf = new WCHAR[len*4 + 1];
	buf = str->GetBuffer(len*4+1+str->GetLength())+str->GetLength();
	SecureZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(code, 0, (LPCSTR)p, len, buf, len*4);
	str->ReleaseBuffer();
	//str->Append(buf);
	//delete buf;
}	
BOOL CGit::IsInitRepos()
{
	CString cmdout;
	cmdout.Empty();
	if(g_Git.Run(_T("git.exe rev-parse --revs-only HEAD"),&cmdout,CP_UTF8))
	{
	//	CMessageBox::Show(NULL,cmdout,_T("TortoiseGit"),MB_OK);
		return TRUE;
	}
	if(cmdout.IsEmpty())
		return TRUE;

	return FALSE;
}
int CGit::Run(CGitCall* pcall)
{
	PROCESS_INFORMATION pi;
	HANDLE hRead;
	if(RunAsync(pcall->GetCmd(),&pi,&hRead))
		return GIT_ERROR_CREATE_PROCESS;

	DWORD readnumber;
	BYTE data[CALL_OUTPUT_READ_CHUNK_SIZE];
	bool bAborted=false;
	while(ReadFile(hRead,data,CALL_OUTPUT_READ_CHUNK_SIZE,&readnumber,NULL))
	{
		//Todo: when OnOutputData() returns 'true', abort git-command. Send CTRL-C signal?
		if(!bAborted)//For now, flush output when command aborted.
			if(pcall->OnOutputData(data,readnumber))
				bAborted=true;
	}
	if(!bAborted)
		pcall->OnEnd();

	
	CloseHandle(pi.hThread);

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exitcode =0;

	if(!GetExitCodeProcess(pi.hProcess,&exitcode))
	{
		return GIT_ERROR_GET_EXIT_CODE;
	}

	CloseHandle(pi.hProcess);

	CloseHandle(hRead);
	return exitcode;
}
class CGitCall_ByteVector : public CGitCall
{
public:
	CGitCall_ByteVector(CString cmd,BYTE_VECTOR* pvector):CGitCall(cmd),m_pvector(pvector){}
	virtual bool OnOutputData(const BYTE* data, size_t size)
	{
		size_t oldsize=m_pvector->size();
		m_pvector->resize(m_pvector->size()+size);
		memcpy(&*(m_pvector->begin()+oldsize),data,size);
		return false;
	}
	BYTE_VECTOR* m_pvector;

};
int CGit::Run(CString cmd,BYTE_VECTOR *vector)
{
	CGitCall_ByteVector call(cmd,vector);
	return Run(&call);
}
int CGit::Run(CString cmd, CString* output,int code)
{
	BYTE_VECTOR vector;
	int ret;
	ret=Run(cmd,&vector);

	vector.push_back(0);
	
	StringAppend(output,&(vector[0]),code);
	return ret;
}

CString CGit::GetUserName(void)
{
	CString UserName;
	Run(_T("git.exe config user.name"),&UserName,CP_UTF8);
	return UserName;
}
CString CGit::GetUserEmail(void)
{
	CString UserName;
	Run(_T("git.exe config user.email"),&UserName,CP_UTF8);
	return UserName;
}

CString CGit::GetCurrentBranch(void)
{
	CString output;
	//Run(_T("git.exe branch"),&branch);

	int ret=g_Git.Run(_T("git.exe branch"),&output,CP_UTF8);
	if(!ret)
	{		
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			//i++;
			one=output.Tokenize(_T("\n"),pos);
			//list.push_back(one.Right(one.GetLength()-2));
			if(one[0] == _T('*'))
				return one.Right(one.GetLength()-2);
		}
	}
	return CString("");
}

int CGit::GetCurrentBranchFromFile(const CString &sProjectRoot, CString &sBranchOut)
{
	// read current branch name like git-gui does, by parsing the .git/HEAD file directly

	if ( sProjectRoot.IsEmpty() )
		return -1;

	CString sHeadFile = sProjectRoot + _T("\\") + g_GitAdminDir.GetAdminDirName() + _T("\\HEAD");

	FILE *pFile;
	_tfopen_s(&pFile, sHeadFile.GetString(), _T("r"));

	if (!pFile)
	{
		return -1;
	}

	char s[256] = {0};
    fgets(s, sizeof(s), pFile);

	fclose(pFile);

	const char *pfx = "ref: refs/heads/";
	const int len = 16;//strlen(pfx)

	if ( !strncmp(s, pfx, len) )
	{
		//# We're on a branch.  It might not exist.  But
		//# HEAD looks good enough to be a branch.
		sBranchOut = s + len;
		sBranchOut.TrimRight(_T(" \r\n\t"));

		if ( sBranchOut.IsEmpty() )
			return -1;
	}
	else
	{
		//# Assume this is a detached head.
		sBranchOut = "HEAD";

		return 1;
	}

	return 0;
}

int CGit::BuildOutputFormat(CString &format,bool IsFull)
{
	CString log;
	log.Format(_T("#<%c>%%x00"),LOG_REV_ITEM_BEGIN);
	format += log;
	if(IsFull)
	{
		log.Format(_T("#<%c>%%an%%x00"),LOG_REV_AUTHOR_NAME);
		format += log;
		log.Format(_T("#<%c>%%ae%%x00"),LOG_REV_AUTHOR_EMAIL);
		format += log;
		log.Format(_T("#<%c>%%ai%%x00"),LOG_REV_AUTHOR_DATE);
		format += log;
		log.Format(_T("#<%c>%%cn%%x00"),LOG_REV_COMMIT_NAME);
		format += log;
		log.Format(_T("#<%c>%%ce%%x00"),LOG_REV_COMMIT_EMAIL);
		format += log;
		log.Format(_T("#<%c>%%ci%%x00"),LOG_REV_COMMIT_DATE);
		format += log;
		log.Format(_T("#<%c>%%s%%x00"),LOG_REV_COMMIT_SUBJECT);
		format += log;
		log.Format(_T("#<%c>%%b%%x00"),LOG_REV_COMMIT_BODY);
		format += log;
	}
	log.Format(_T("#<%c>%%m%%H%%x00"),LOG_REV_COMMIT_HASH);
	format += log;
	log.Format(_T("#<%c>%%P%%x00"),LOG_REV_COMMIT_PARENT);
	format += log;

	if(IsFull)
	{
		log.Format(_T("#<%c>%%x00"),LOG_REV_COMMIT_FILE);
		format += log;
	}
	return 0;
}

int CGit::GetLog(BYTE_VECTOR& logOut, CString &hash,  CTGitPath *path ,int count,int mask,CString *from,CString *to)
{
	CGitCall_ByteVector gitCall(CString(),&logOut);
	return GetLog(&gitCall,hash,path,count,mask,from,to);
}

//int CGit::GetLog(CGitCall* pgitCall, CString &hash,  CTGitPath *path ,int count,int mask)
int CGit::GetLog(CGitCall* pgitCall, CString &hash, CTGitPath *path, int count, int mask,CString *from,CString *to)
{

	CString cmd;
	CString log;
	CString num;
	CString since;

	CString file;

	if(path)
		file.Format(_T(" -- \"%s\""),path->GetGitPathString());
	
	if(count>0)
		num.Format(_T("-n%d"),count);

	CString param;

	if(mask& LOG_INFO_STAT )
		param += _T(" --numstat ");
	if(mask& LOG_INFO_FILESTATE)
		param += _T(" --raw ");

	if(mask& LOG_INFO_FULLHISTORY)
		param += _T(" --full-history ");

	if(mask& LOG_INFO_BOUNDARY)
		param += _T(" --left-right --boundary ");

	if(mask& CGit::LOG_INFO_ALL_BRANCH)
		param += _T(" --all ");

	if(mask& CGit::LOG_INFO_DETECT_COPYRENAME)
		param += _T(" -C ");
	
	if(mask& CGit::LOG_INFO_DETECT_RENAME )
		param += _T(" -M ");

	if(mask& CGit::LOG_INFO_FIRST_PARENT )
		param += _T(" --first-parent ");
	
	if(mask& CGit::LOG_INFO_NO_MERGE )
		param += _T(" --no-merges ");

	if(mask& CGit::LOG_INFO_FOLLOW)
		param += _T(" --follow ");

	if(from != NULL && to != NULL)
	{
		CString range;
		range.Format(_T(" %s..%s "),*from,*to);
		param += range;
	}
	param+=hash;

	cmd.Format(_T("git.exe log %s -z --topo-order %s --parents --pretty=format:\""),
				num,param);

	BuildOutputFormat(log,!(mask&CGit::LOG_INFO_ONLY_HASH));

	cmd += log;
	cmd += CString(_T("\"  "))+hash+file;

	pgitCall->SetCmd(cmd);

	return Run(pgitCall);
//	return Run(cmd,&logOut);
}

#if 0
int CGit::GetShortLog(CString &logOut,CTGitPath * path, int count)
{
	CString cmd;
	CString log;
	int n;
	if(count<0)
		n=100;
	else
		n=count;
	cmd.Format(_T("git.exe log --left-right --boundary --topo-order -n%d --pretty=format:\""),n);
	BuildOutputFormat(log,false);
	cmd += log+_T("\"");
	if (path)
		cmd+= _T("  -- \"")+path->GetGitPathString()+_T("\"");
	//cmd += CString(_T("\" HEAD~40..HEAD"));
	return Run(cmd,&logOut);
}
#endif

#define BUFSIZE 512
void GetTempPath(CString &path)
{
	TCHAR lpPathBuffer[BUFSIZE];
	DWORD dwRetVal;
	DWORD dwBufSize=BUFSIZE;
	dwRetVal = GetTempPath(dwBufSize,     // length of the buffer
                           lpPathBuffer); // buffer for path 
    if (dwRetVal > dwBufSize || (dwRetVal == 0))
    {
        path=_T("");
    }
	path.Format(_T("%s"),lpPathBuffer);
}
CString GetTempFile()
{
	TCHAR lpPathBuffer[BUFSIZE];
	DWORD dwRetVal;
    DWORD dwBufSize=BUFSIZE;
	TCHAR szTempName[BUFSIZE];  
	UINT uRetVal;

	dwRetVal = GetTempPath(dwBufSize,     // length of the buffer
                           lpPathBuffer); // buffer for path 
    if (dwRetVal > dwBufSize || (dwRetVal == 0))
    {
        return _T("");
    }
	 // Create a temporary file. 
    uRetVal = GetTempFileName(lpPathBuffer, // directory for tmp files
                              TEXT("Patch"),  // temp file name prefix 
                              0,            // create unique name 
                              szTempName);  // buffer for name 


    if (uRetVal == 0)
    {
        return _T("");
    }

	return CString(szTempName);

}

int CGit::RunLogFile(CString cmd,CString &filename)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb=sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	SECURITY_ATTRIBUTES   psa={sizeof(psa),NULL,TRUE};;   
	psa.bInheritHandle=TRUE;   
    
	HANDLE   houtfile=CreateFile(filename,GENERIC_WRITE,FILE_SHARE_READ   |   FILE_SHARE_WRITE,   
			&psa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);   


	si.wShowWindow=SW_HIDE;
	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdOutput   =   houtfile; 
	
	if(!CreateProcess(NULL,(LPWSTR)cmd.GetString(), NULL,NULL,TRUE,NULL,NULL,(LPWSTR)m_CurrentDir.GetString(),&si,&pi))
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,NULL);
		return GIT_ERROR_CREATE_PROCESS;
	}
	
	WaitForSingleObject(pi.hProcess,INFINITE);   
  	
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(houtfile);
	return GIT_SUCCESS;
//	return 0;
}

git_revnum_t CGit::GetHash(CString &friendname)
{
	CString cmd;
	CString out;
	cmd.Format(_T("git.exe rev-parse %s" ),friendname);
	Run(cmd,&out,CP_UTF8);
//	int pos=out.ReverseFind(_T('\n'));
	int pos=out.FindOneOf(_T("\r\n"));
	if(pos>0)
		return out.Left(pos);
	return out;
}

int CGit::GetTagList(STRING_VECTOR &list)
{
	int ret;
	CString cmd,output;
	cmd=_T("git.exe tag -l");
	int i=0;
	ret=g_Git.Run(cmd,&output,CP_UTF8);
	if(!ret)
	{		
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			i++;
			one=output.Tokenize(_T("\n"),pos);
			list.push_back(one);
		}
	}
	return ret;
}

int CGit::GetBranchList(STRING_VECTOR &list,int *current,BRANCH_TYPE type)
{
	int ret;
	CString cmd,output;
	cmd=_T("git.exe branch");

	if(type==(BRANCH_LOCAL|BRANCH_REMOTE))
		cmd+=_T(" -a");
	else if(type==BRANCH_REMOTE)
		cmd+=_T(" -r");

	int i=0;
	ret=g_Git.Run(cmd,&output,CP_UTF8);
	if(!ret)
	{		
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			one=output.Tokenize(_T("\n"),pos);
			list.push_back(one.Right(one.GetLength()-2));
			if(one[0] == _T('*'))
				if(current)
					*current=i;
			i++;
		}
	}
	return ret;
}

int CGit::GetRemoteList(STRING_VECTOR &list)
{
	int ret;
	CString cmd,output;
	cmd=_T("git.exe config  --get-regexp remote.*.url");
	ret=g_Git.Run(cmd,&output,CP_UTF8);
	if(!ret)
	{
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			one=output.Tokenize(_T("\n"),pos);
			int start=one.Find(_T("."),0);
			if(start>0)
			{
				CString url;
				url=one.Right(one.GetLength()-start-1);
				one=url;
				one=one.Left(one.Find(_T("."),0));
				list.push_back(one);
			}
		}
	}
	return ret;
}

int CGit::GetRefList(STRING_VECTOR &list)
{
	int ret;
	CString cmd,output;
	cmd=_T("git show-ref -d");
	ret=g_Git.Run(cmd,&output,CP_UTF8);
	if(!ret)
	{
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			one=output.Tokenize(_T("\n"),pos);
			int start=one.Find(_T(" "),0);
			if(start>0)
			{
				CString name;
				name=one.Right(one.GetLength()-start-1);
				list.push_back(name);
			}
		}
	}
	return ret;
}
int CGit::GetMapHashToFriendName(MAP_HASH_NAME &map)
{
	int ret;
	CString cmd,output;
	cmd=_T("git show-ref -d");
	ret=g_Git.Run(cmd,&output,CP_UTF8);
	if(!ret)
	{
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			one=output.Tokenize(_T("\n"),pos);
			int start=one.Find(_T(" "),0);
			if(start>0)
			{
				CString name;
				name=one.Right(one.GetLength()-start-1);

				CString hash;
				hash=one.Left(start);

				map[hash].push_back(name);
			}
		}
	}
	return ret;
}

BOOL CGit::CheckMsysGitDir()
{
	static BOOL bInitialized = FALSE;

	if (bInitialized)
	{
		return TRUE;
	}

	TCHAR *oldpath,*home;
	size_t size;

	// set HOME if not set already
	_tgetenv_s(&size, NULL, 0, _T("HOME"));
	if (!size)
	{
		_tdupenv_s(&home,&size,_T("USERPROFILE")); 
		_tputenv_s(_T("HOME"),home);
		free(home);
	}

	//setup ssh client
	CString sshclient=CRegString(_T("Software\\TortoiseGit\\SSH"));

	if(!sshclient.IsEmpty())
	{
		_tputenv_s(_T("GIT_SSH"),sshclient);
	}else
	{
		TCHAR sPlink[MAX_PATH];
		GetModuleFileName(NULL, sPlink, _countof(sPlink));
		LPTSTR ptr = _tcsrchr(sPlink, _T('\\'));
		if (ptr) {
			_tcscpy(ptr + 1, _T("TortoisePlink.exe"));
			_tputenv_s(_T("GIT_SSH"), sPlink);
		}
	}

	{
		TCHAR sAskPass[MAX_PATH];
		GetModuleFileName(NULL, sAskPass, _countof(sAskPass));
		LPTSTR ptr = _tcsrchr(sAskPass, _T('\\'));
		if (ptr) 
		{
			_tcscpy(ptr + 1, _T("SshAskPass.exe"));
			_tputenv_s(_T("DISPLAY"),_T(":9999"));
			_tputenv_s(_T("SSH_ASKPASS"),sAskPass);
		}
	}
	// search PATH if git/bin directory is alredy present
	if ( FindGitPath() )
	{
		bInitialized = TRUE;
		return TRUE;
	}

	// add git/bin path to PATH

	CRegString msysdir=CRegString(REG_MSYSGIT_PATH,_T(""),FALSE,HKEY_LOCAL_MACHINE);
	CString str=msysdir;
	if(str.IsEmpty())
	{
		CRegString msysinstalldir=CRegString(REG_MSYSGIT_INSTALL,_T(""),FALSE,HKEY_LOCAL_MACHINE);
		str=msysinstalldir;
		if ( !str.IsEmpty() )
		{
			str += (str[str.GetLength()-1] != '\\') ? "\\bin" : "bin";
			msysdir=str;
			msysdir.write();
		}
		else
		{
			return false;
		}
	}
	//CGit::m_MsysGitPath=str;

	//set path

	_tdupenv_s(&oldpath,&size,_T("PATH")); 

	CString path;
	path.Format(_T("%s;%s"),oldpath,str);

	_tputenv_s(_T("PATH"),path);

	CString sOldPath = oldpath;
	free(oldpath);


    if( !FindGitPath() )
	{
		return false;
	}
	else
	{
#ifdef _TORTOISESHELL
		l_processEnv = GetEnvironmentStrings();
		// updated environment is now duplicated for use in CreateProcess, restore original PATH for current process
		_tputenv_s(_T("PATH"),sOldPath);
#endif

		bInitialized = TRUE;
		return true;
	}
}


class CGitCall_EnumFiles : public CGitCall
{
public:
	CGitCall_EnumFiles(const TCHAR *pszProjectPath, const TCHAR *pszSubPath, unsigned int nFlags, WGENUMFILECB *pEnumCb, void *pUserData)
	:	m_pszProjectPath(pszProjectPath),
		m_pszSubPath(pszSubPath),
		m_nFlags(nFlags),
		m_pEnumCb(pEnumCb),
		m_pUserData(pUserData)
	{
	}

	typedef std::map<CStringA,char>	TStrCharMap;

	const TCHAR *	m_pszProjectPath;
	const TCHAR *	m_pszSubPath;
	unsigned int	m_nFlags;
	WGENUMFILECB *	m_pEnumCb;
	void *			m_pUserData;

	BYTE_VECTOR		m_DataCollector;

	virtual bool	OnOutputData(const BYTE* data, size_t size)
	{
		m_DataCollector.append(data,size);
		while(true)
		{
			// lines from igit.exe are 0 terminated
			int found=m_DataCollector.findData((const BYTE*)"",1);
			if(found<0)
				return false;
			OnSingleLine( (LPCSTR)&*m_DataCollector.begin() );
			m_DataCollector.erase(m_DataCollector.begin(), m_DataCollector.begin()+found+1);
		}
		return false;//Should never reach this
	}
	virtual void	OnEnd()
	{
	}

	UINT HexChar(char ch)
	{
		if (ch >= '0' && ch <= '9')
			return (UINT)(ch - '0');
		else if (ch >= 'A' && ch <= 'F')
			return (UINT)(ch - 'A') + 10;
		else if (ch >= 'a' && ch <= 'f')
			return (UINT)(ch - 'a') + 10;
		else
			return 0;
	}

	bool OnSingleLine(LPCSTR line)
	{
		//Parse single line

		wgFile_s fileStatus;

		// file/dir type

		fileStatus.nFlags = 0;
		if (*line == 'D')
			fileStatus.nFlags |= WGFF_Directory;
		else if (*line != 'F')
			// parse error
			return false;
		line += 2;

		// status

		fileStatus.nStatus = WGFS_Unknown;
		switch (*line)
		{
		case 'N': fileStatus.nStatus = WGFS_Normal; break;
		case 'M': fileStatus.nStatus = WGFS_Modified; break;
		case 'S': fileStatus.nStatus = WGFS_Staged; break;
		case 'A': fileStatus.nStatus = WGFS_Added; break;
		case 'C': fileStatus.nStatus = WGFS_Conflicted; break;
		case 'D': fileStatus.nStatus = WGFS_Deleted; break;
		case 'I': fileStatus.nStatus = WGFS_Ignored; break;
		case 'U': fileStatus.nStatus = WGFS_Unversioned; break;
		case 'E': fileStatus.nStatus = WGFS_Empty; break;
		case '?': fileStatus.nStatus = WGFS_Unknown; break;
		default:
			// parse error
			return false;
		}
		line += 2;

		// file sha1

		BYTE sha1[20];
		fileStatus.sha1 = NULL;
		if ( !(fileStatus.nFlags & WGFF_Directory) )
		{
			for (int i=0; i<20; i++)
			{
				sha1[i] = (BYTE)((HexChar(line[0]) << 8) | HexChar(line[1]));
				line += 2;
			}

			line++;
		}

		// filename
		int len = strlen(line);
		if (len && len < 2048)
		{
			WCHAR *buf = (WCHAR*)alloca((len*4+2)*sizeof(WCHAR));
			*buf = 0;
			MultiByteToWideChar(CP_ACP, 0, line, len+1, buf, len*4+1);
			fileStatus.sFileName = buf;

			if (*buf && (*m_pEnumCb)(&fileStatus,m_pUserData))
				return false;
		}

		return true;
	}
};

BOOL CGit::EnumFiles(const TCHAR *pszProjectPath, const TCHAR *pszSubPath, unsigned int nFlags, WGENUMFILECB *pEnumCb, void *pUserData)
{
	if(!pszProjectPath || *pszProjectPath=='\0')
		return FALSE;

	CGitCall_EnumFiles W_GitCall(pszProjectPath,pszSubPath,nFlags,pEnumCb,pUserData);
	CString cmd;

/*	char W_szToDir[MAX_PATH];
	strncpy(W_szToDir,pszProjectPath,sizeof(W_szToDir)-1);
	if(W_szToDir[strlen(W_szToDir)-1]!='\\')
		strncat(W_szToDir,"\\",sizeof(W_szToDir)-1);

	SetCurrentDirectoryA(W_szToDir);
	GetCurrentDirectoryA(sizeof(W_szToDir)-1,W_szToDir);
*/
	SetCurrentDir(pszProjectPath);

	CString sMode;
	if (nFlags)
	{
		if (nFlags & WGEFF_NoRecurse) sMode += _T("r");
		if (nFlags & WGEFF_FullPath) sMode += _T("f");
		if (nFlags & WGEFF_DirStatusDelta) sMode += _T("d");
		if (nFlags & WGEFF_DirStatusAll) sMode += _T("D");
		if (nFlags & WGEFF_EmptyAsNormal) sMode += _T("e");
		if (nFlags & WGEFF_SingleFile) sMode += _T("s");
	}
	else
	{
		sMode = _T("-");
	}

	if (pszSubPath)
		cmd.Format(_T("igit.exe \"%s\" status %s \"%s\""), pszProjectPath, sMode, pszSubPath);
	else
		cmd.Format(_T("igit.exe \"%s\" status %s"), pszProjectPath, sMode);

	W_GitCall.SetCmd(cmd);
	// NOTE: should igit get added as a part of msysgit then use below line instead of the above one
	//W_GitCall.SetCmd(CGit::ms_LastMsysGitDir + cmd);

	if ( Run(&W_GitCall) )
		return FALSE;

	return TRUE;
}

BOOL CGit::CheckCleanWorkTree()
{
	CString out;
	CString cmd;
	cmd=_T("git.exe rev-parse --verify HEAD");

	if(g_Git.Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git.exe update-index --ignore-submodules --refresh");
	if(g_Git.Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git.exe diff-files --quiet --ignore-submodules");
	if(g_Git.Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git diff-index --cached --quiet HEAD --ignore-submodules");
	if(g_Git.Run(cmd,&out,CP_UTF8))
		return FALSE;

	return TRUE;
}
int CGit::Revert(CTGitPathList &list,bool keep)
{
	int ret;
	for(int i=0;i<list.GetCount();i++)
	{	
		ret = Revert((CTGitPath&)list[i],keep);
		if(ret)
			return ret;
	}
	return 0;
}
int CGit::Revert(CTGitPath &path,bool keep)
{
	CString cmd, out;
	if(path.m_Action & CTGitPath::LOGACTIONS_ADDED)
	{	//To init git repository, there are not HEAD, so we can use git reset command
		cmd.Format(_T("git.exe rm --cache -- \"%s\""),path.GetGitPathString());
		if(g_Git.Run(cmd,&out,CP_OEMCP))
			return -1;
	}
	else if(path.m_Action & CTGitPath::LOGACTIONS_REPLACED )
	{
		cmd.Format(_T("git.exe mv \"%s\" \"%s\""),path.GetGitPathString(),path.GetGitOldPathString());
		if(g_Git.Run(cmd,&out,CP_OEMCP))
			return -1;
		
		cmd.Format(_T("git.exe checkout HEAD -f -- \"%s\""),path.GetGitOldPathString());
		if(g_Git.Run(cmd,&out,CP_OEMCP))
			return -1;
	}
	else
	{
		cmd.Format(_T("git.exe checkout HEAD -f -- \"%s\""),path.GetGitPathString());
		if(g_Git.Run(cmd,&out,CP_OEMCP))
			return -1;
	}
	return 0;
}

int CGit::ListConflictFile(CTGitPathList &list,CTGitPath *path)
{
	BYTE_VECTOR vector;

	CString cmd;
	if(path)
		cmd.Format(_T("git.exe ls-files -u -t -z -- \"%s\""),path->GetGitPathString());
	else
		cmd=_T("git.exe ls-files -u -t -z");

	if(g_Git.Run(cmd,&vector))
	{
		return -1;
	}

	list.ParserFromLsFile(vector);

	return 0;
}