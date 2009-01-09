// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "SettingsLogCaches.h"
#include "MessageBox.h"
#include "Git.h"
#include "SVNError.h"
#include "LogCachePool.h"
#include "LogCacheStatistics.h"
#include "LogCacheStatisticsDlg.h"
#include "ProgressDlg.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"
#include "CSVWriter.h"
#include "XPTheme.h"

using namespace LogCache;

IMPLEMENT_DYNAMIC(CSettingsLogCaches, ISettingsPropPage)

#define WM_REFRESH_REPOSITORYLIST (WM_APP + 110)

CSettingsLogCaches::CSettingsLogCaches()
	: ISettingsPropPage(CSettingsLogCaches::IDD)
    , progress(NULL)
{
}

CSettingsLogCaches::~CSettingsLogCaches()
{
}

// update cache list

BOOL CSettingsLogCaches::OnSetActive()
{
    FillRepositoryList();

    return ISettingsPropPage::OnSetActive();
}

void CSettingsLogCaches::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_REPOSITORYLIST, m_cRepositoryList);
}


BEGIN_MESSAGE_MAP(CSettingsLogCaches, ISettingsPropPage)
	ON_BN_CLICKED(IDC_CACHEDETAILS, OnBnClickedDetails)
	ON_BN_CLICKED(IDC_CACHEUPDATE, OnBnClickedUpdate)
	ON_BN_CLICKED(IDC_CACHEEXPORT, OnBnClickedExport)
	ON_BN_CLICKED(IDC_CACHEDELETE, OnBnClickedDelete)

	ON_MESSAGE(WM_REFRESH_REPOSITORYLIST, OnRefeshRepositoryList)
	ON_NOTIFY(NM_DBLCLK, IDC_REPOSITORYLIST, &CSettingsLogCaches::OnNMDblclkRepositorylist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_REPOSITORYLIST, &CSettingsLogCaches::OnLvnItemchangedRepositorylist)
END_MESSAGE_MAP()

BOOL CSettingsLogCaches::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

    // repository list (including header text update when language changed)

	m_cRepositoryList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	m_cRepositoryList.DeleteAllItems();
    int c = m_cRepositoryList.GetHeaderCtrl()->GetItemCount()-1;
	while (c>=0)
		m_cRepositoryList.DeleteColumn(c--);

    CString temp;
    temp.LoadString(IDS_SETTINGS_REPOSITORY_URL);
	m_cRepositoryList.InsertColumn (0, temp, LVCFMT_LEFT, 289);
	temp.LoadString(IDS_SETTINGS_REPOSITORY_SIZE);
	m_cRepositoryList.InsertColumn (1, temp, LVCFMT_RIGHT, 95);

	CXPTheme theme;
	theme.SetWindowTheme(m_cRepositoryList.GetSafeHwnd(), L"Explorer", NULL);

    FillRepositoryList();

    // tooltips

	m_tooltips.Create(this);

	m_tooltips.AddTool(IDC_REPOSITORYLIST, IDS_SETTINGS_LOGCACHE_CACHELIST);

	m_tooltips.AddTool(IDC_CACHEDETAILS, IDS_SETTINGS_LOGCACHE_DETAILS);
	m_tooltips.AddTool(IDC_CACHEUPDATE, IDS_SETTINGS_LOGCACHE_UPDATE);
	m_tooltips.AddTool(IDC_CACHEEXPORT, IDS_SETTINGS_LOGCACHE_EXPORT);
	m_tooltips.AddTool(IDC_CACHEDELETE, IDS_SETTINGS_LOGCACHE_DELETE);

	return TRUE;
}

BOOL CSettingsLogCaches::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSettingsLogCaches::OnBnClickedDetails()
{
    TRepo repo = GetSelectedRepo();
    if (!repo.first.IsEmpty())
    {
        CLogCacheStatistics staticstics 
            (*SVN().GetLogCachePool(), repo.second, repo.first);

        CLogCacheStatisticsDlg dialog (staticstics.GetData(), this);
        dialog.DoModal();
    }
}

void CSettingsLogCaches::OnBnClickedUpdate()
{
	AfxBeginThread (WorkerThread, this);
}

void CSettingsLogCaches::OnBnClickedExport()
{
    TRepo repo = GetSelectedRepo();
    if (!repo.first.IsEmpty())
    {
        CFileDialog dialog (FALSE);
        if (dialog.DoModal() == IDOK)
        {
            SVN svn;
            CCachedLogInfo* cache 
                = svn.GetLogCachePool()->GetCache (repo.second, repo.first);
            CCSVWriter writer;
            writer.Write (*cache, (LPCTSTR)dialog.GetFileName());
        }
    }
}

void CSettingsLogCaches::OnBnClickedDelete()
{
	int nSelCount = m_cRepositoryList.GetSelectedCount();
	CString sQuestion;
	sQuestion.Format(IDS_SETTINGS_CACHEDELETEQUESTION, nSelCount);
	if (CMessageBox::Show(m_hWnd, sQuestion, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		POSITION pos = m_cRepositoryList.GetFirstSelectedItemPosition();
		while (pos)
		{
			int index = m_cRepositoryList.GetNextSelectedItem(pos);
			IT iter = repos.begin();
			std::advance (iter, index);
            SVN().GetLogCachePool()->DropCache (iter->second, iter->first);
		}
		FillRepositoryList();
	}
}

LRESULT CSettingsLogCaches::OnRefeshRepositoryList (WPARAM, LPARAM)
{
    FillRepositoryList();
	return 0L;
}

CSettingsLogCaches::TRepo CSettingsLogCaches::GetSelectedRepo()
{
    int index = m_cRepositoryList.GetSelectionMark();
    if (index == -1)
        return CSettingsLogCaches::TRepo();

    IT iter = repos.begin();
    std::advance (iter, index);

    return *iter;
}

void CSettingsLogCaches::FillRepositoryList()
{
    int count = m_cRepositoryList.GetItemCount();
    while (count > 0)
        m_cRepositoryList.DeleteItem (--count);

    SVN svn;
    CLogCachePool* caches = svn.GetLogCachePool();
    repos = caches->GetRepositoryURLs();

    for (IT iter = repos.begin(), end = repos.end(); iter != end; ++iter, ++count)
    {
        CString url = iter->first;

        m_cRepositoryList.InsertItem (count, url);
        size_t fileSize = caches->FileSize (iter->second, url) / 1024;

        CString sizeText;
        sizeText.Format(TEXT("%d"), fileSize);
        m_cRepositoryList.SetItemText (count, 1, sizeText);
    }
}

// implement ILogReceiver

void CSettingsLogCaches::ReceiveLog ( LogChangedPathArray* 
					                , svn_revnum_t rev
                                    , const StandardRevProps* 
                                    , UserRevPropArray* 
                                    , bool )
{
	// update internal data

    if ((headRevision < (svn_revnum_t)rev) || (headRevision == NO_REVISION))
		headRevision = rev;

	// update progress bar and check for user pressing "Cancel"

	static DWORD lastProgressCall = 0;
	if (lastProgressCall < GetTickCount() - 500)
	{
		lastProgressCall = GetTickCount();

		CString temp;
		temp.LoadString(IDS_REVGRAPH_PROGGETREVS);
		progress->SetLine(1, temp);
        temp.Format(IDS_REVGRAPH_PROGCURRENTREV, rev);
		progress->SetLine(2, temp);

		progress->SetProgress (headRevision - rev, headRevision);
		if (progress->HasUserCancelled())
			throw SVNError (SVN_ERR_CANCELLED, "");
	}
}

UINT CSettingsLogCaches::WorkerThread(LPVOID pVoid)
{
    CoInitialize (NULL);

	CSettingsLogCaches* dialog = (CSettingsLogCaches*)pVoid;

    dialog->progress = new CProgressDlg();
	dialog->progress->SetTitle(IDS_SETTINGS_LOGCACHE_UPDATETITLE);
	dialog->progress->SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
	dialog->progress->SetTime();
	dialog->progress->ShowModal (dialog->m_hWnd);

	// we have to get the log from the repository root

    SVN svn;
	CLogCachePool* caches = svn.GetLogCachePool();
    CRepositoryInfo& info = caches->GetRepositoryInfo();

    TRepo repo = dialog->GetSelectedRepo();
	CTSVNPath urlpath;
    urlpath.SetFromSVN (repo.first);

    dialog->headRevision = info.GetHeadRevision (repo.second, urlpath);
	dialog->progress->SetProgress (0, dialog->headRevision);

	apr_pool_t *pool = svn_pool_create(NULL);

    try
	{
        CSVNLogQuery svnQuery (svn.m_pctx, pool);
		CCacheLogQuery query (caches, &svnQuery);

		query.Log ( CTSVNPathList (urlpath)
				  , dialog->headRevision
				  , dialog->headRevision
				  , SVNRev(0)
				  , 0
				  , false		// strictNodeHistory
				  , dialog
                  , true		// includeChanges
                  , false		// includeMerges
                  , true		// includeStandardRevProps
                  , true		// includeUserRevProps
                  , TRevPropNames());
	}
	catch (SVNError&)
	{
	}

    caches->Flush();
	svn_pool_destroy (pool);

	if (dialog->progress)
	{
		dialog->progress->Stop();
		delete dialog->progress;
		dialog->progress = NULL;
	}

    CoUninitialize();

    dialog->PostMessage (WM_REFRESH_REPOSITORYLIST);

	return 0;
}

void CSettingsLogCaches::OnNMDblclkRepositorylist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	OnBnClickedDetails();
	*pResult = 0;
}

void CSettingsLogCaches::OnLvnItemchangedRepositorylist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	UINT count = m_cRepositoryList.GetSelectedCount();
	GetDlgItem(IDC_CACHEDETAILS)->EnableWindow(count == 1);
	GetDlgItem(IDC_CACHEUPDATE)->EnableWindow(count == 1);
	GetDlgItem(IDC_CACHEEXPORT)->EnableWindow(count == 1);
	GetDlgItem(IDC_CACHEDELETE)->EnableWindow(count >= 1);
	*pResult = 0;
}
