#pragma once

class GitAdminDir
{
public:
	GitAdminDir(void);
	~GitAdminDir(void);
	/**
	 * Initializes the global object. Call this after apr is initialized but
	 * before using any other methods of this class.
	 */
	bool Init();
	/**
	 * Clears the memory pool. Call this before you clear *all* pools
	 * with apr_pool_terminate(). If you don't use apr_pool_terminate(), then
	 * this method doesn't need to be called, because the deconstructor will
	 * do the same too.
	 */
	bool Close();
	
	/// Returns true if \a name is the admin dir name
	bool IsAdminDirName(const CString& name) const;
	
	/// Returns true if the path points to or below an admin directory
	bool IsAdminDirPath(const CString& path) const;
	
	/// Returns true if the path (file or folder) has an admin directory 
	/// associated, i.e. if the path is in a working copy.
	bool HasAdminDir(const CString& path) const;
	bool HasAdminDir(const CString& path, bool bDir) const;
	
	/// Returns true if the admin dir name is set to "_svn".
	bool IsVSNETHackActive() const {return m_bVSNETHack;}
	
	CString GetAdminDirName() const {return _T(".git");}
	CString GetVSNETAdminDirName() const {return _T("_git");}
private:
	bool m_bVSNETHack;
	int m_nInit;

};

extern GitAdminDir g_GitAdminDir;