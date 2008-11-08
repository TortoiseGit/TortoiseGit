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
#include "StdAfx.h"
#include "ExportCommand.h"

#include "ExportDlg.h"
#include "SVNProgressDlg.h"
#include "SVNAdminDir.h"
#include "ProgressDlg.h"
#include "BrowseFolder.h"
#include "DirFileEnum.h"
#include "MessageBox.h"
#include "SVNStatus.h"

bool ExportCommand::Execute()
{
	bool bRet = false;
	// When the user clicked on a working copy, we know that the export should
	// be done from that. We then have to ask where the export should go to.
	// If however the user clicked on an unversioned folder, we assume that
	// this is where the export should go to and have to ask from where
	// the export should be done from.
	TCHAR saveto[MAX_PATH];
	bool bURL = !!SVN::PathIsURL(cmdLinePath);
	svn_wc_status_kind s = SVNStatus::GetAllStatus(cmdLinePath);
	if ((bURL)||(s == svn_wc_status_unversioned)||(s == svn_wc_status_none))
	{
		// ask from where the export has to be done
		CExportDlg dlg;
		if (bURL)
			dlg.m_URL = cmdLinePath.GetSVNPathString();
		else
			dlg.m_strExportDirectory = cmdLinePath.GetWinPathString();
		if (parser.HasKey(_T("revision")))
		{
			SVNRev Rev = SVNRev(parser.GetVal(_T("revision")));
			dlg.Revision = Rev;
		}
		if (dlg.DoModal() == IDOK)
		{
			CTSVNPath exportPath(dlg.m_strExportDirectory);

			CSVNProgressDlg progDlg;
			theApp.m_pMainWnd = &progDlg;
			progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Export);
			if (parser.HasVal(_T("closeonend")))
				progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
			DWORD options = dlg.m_bNoExternals ? ProgOptIgnoreExternals : ProgOptNone;
			if (dlg.m_eolStyle.CompareNoCase(_T("CRLF"))==0)
				options |= ProgOptEolCRLF;
			if (dlg.m_eolStyle.CompareNoCase(_T("CR"))==0)
				options |= ProgOptEolCR;
			if (dlg.m_eolStyle.CompareNoCase(_T("LF"))==0)
				options |= ProgOptEolLF;
			progDlg.SetOptions(options);
			progDlg.SetPathList(CTSVNPathList(exportPath));
			progDlg.SetUrl(dlg.m_URL);
			progDlg.SetRevision(dlg.Revision);
			progDlg.SetDepth(dlg.m_depth);
			progDlg.DoModal();
			bRet = !progDlg.DidErrorsOccur();
		}
	}
	else
	{
		// ask where the export should go to.
		CBrowseFolder folderBrowser;
		CString strTemp;
		strTemp.LoadString(IDS_PROC_EXPORT_1);
		folderBrowser.SetInfo(strTemp);
		folderBrowser.m_style = BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
		strTemp.LoadString(IDS_PROC_EXPORT_2);
		folderBrowser.SetCheckBoxText(strTemp);
		strTemp.LoadString(IDS_PROC_OMMITEXTERNALS);
		folderBrowser.SetCheckBoxText2(strTemp);
		folderBrowser.DisableCheckBox2WhenCheckbox1IsEnabled(true);
		CRegDWORD regExtended = CRegDWORD(_T("Software\\TortoiseSVN\\ExportExtended"), FALSE);
		CBrowseFolder::m_bCheck = regExtended;
		if (folderBrowser.Show(hwndExplorer, saveto, MAX_PATH)==CBrowseFolder::OK)
		{
			CString saveplace = CString(saveto);

			if (cmdLinePath.IsEquivalentTo(CTSVNPath(saveplace)))
			{
				// exporting to itself:
				// remove all svn admin dirs, effectively unversion the 'exported' folder.
				CString msg;
				msg.Format(IDS_PROC_EXPORTUNVERSION, (LPCTSTR)saveplace);
				if (CMessageBox::Show(hwndExplorer, msg, _T("TortoiseSVN"), MB_ICONQUESTION|MB_YESNO) == IDYES)
				{
					CProgressDlg progress;
					progress.SetTitle(IDS_PROC_UNVERSION);
					progress.SetAnimation(IDR_MOVEANI);
					progress.FormatNonPathLine(1, IDS_SVNPROGRESS_EXPORTINGWAIT);
					progress.SetTime(true);
					progress.ShowModeless(hwndExplorer);
					std::vector<CTSVNPath> removeVector;

					CDirFileEnum lister(saveplace);
					CString srcFile;
					bool bFolder = false;
					while (lister.NextFile(srcFile, &bFolder))
					{
						CTSVNPath item(srcFile);
						if ((bFolder)&&(g_SVNAdminDir.IsAdminDirName(item.GetFileOrDirectoryName())))
						{
							removeVector.push_back(item);
						}
					}
					DWORD count = 0;
					for (std::vector<CTSVNPath>::iterator it = removeVector.begin(); (it != removeVector.end()) && (!progress.HasUserCancelled()); ++it)
					{
						progress.FormatPathLine(1, IDS_SVNPROGRESS_UNVERSION, (LPCTSTR)it->GetWinPath());
						progress.SetProgress(count, removeVector.size());
						count++;
						it->Delete(false);
					}
					progress.Stop();
					bRet = true;
				}
				else
					return false;
			}
			else
			{
				saveplace += _T("\\") + cmdLinePath.GetFileOrDirectoryName();
				TRACE(_T("export %s to %s\n"), (LPCTSTR)cmdLinePath.GetUIPathString(), (LPCTSTR)saveto);
				SVN svn;
				if (!svn.Export(cmdLinePath, CTSVNPath(saveplace), bURL ? SVNRev::REV_HEAD : SVNRev::REV_WC, 
					bURL ? SVNRev::REV_HEAD : SVNRev::REV_WC, FALSE, folderBrowser.m_bCheck2, svn_depth_infinity,
					hwndExplorer, folderBrowser.m_bCheck))
				{
					CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
					bRet = false;
				}
				else
					bRet = true;
				regExtended = CBrowseFolder::m_bCheck;
			}
		}
	}
	return bRet;
}
