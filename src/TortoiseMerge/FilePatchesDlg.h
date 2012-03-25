// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006, 2008, 2010-2011 - TortoiseSVN

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
#pragma once
#include "StandAloneDlg.h"

class CPatch;

/**
 * \ingroup TortoiseMerge
 * Virtual class providing the callback interface which
 * is used by CFilePatchesDlg.
 */
class CPatchFilesDlgCallBack
{
public:
	/**
	 * Callback function. Called when the user double clicks on a
	 * specific file to patch. The framework then has to process
	 * the patching/viewing.
	 * \param sFilePath the full path to the file to patch
	 * \param sVersion the revision number of the file to patch
	 * \return TRUE if patching was successful
	 */
	virtual BOOL PatchFile(const int nIndex, bool bAutoPatch = false, bool bIsReview = true) = 0;
	
	/**
	 * Callback function. Called when the user double clicks on a
	 * specific file to diff. The framework then has to fetch the two
	 * files from the URL's and revisions specified in the callback function and
	 * show them in the diff viewer.
	 * \param sURL1 the URL of the first file to diff
	 * \param sURL2 the URL of the second file to diff
	 * \param sRev1 the revision of the first file
	 * \param sRev2 the revision of the second file
	 */
	virtual BOOL DiffFiles(CString sURL1, CString sRev1, CString sURL2, CString sRev2) = 0;
};

#define	FPDLG_FILESTATE_GOOD		0x0000
#define	FPDLG_FILESTATE_CONFLICTED	0x0001
#define FPDLG_FILESTATE_PATCHED		0x0002
#define FPDLG_FILESTATE_NEW			0x0003
#define FPDLG_FILESTATE_DELETE		0x0004
#define FPDLG_FILESTATE_RENAME		0x0005

#define ID_PATCHALL					1
#define ID_PATCHSELECTED			2
#define ID_PATCHPREVIEW				3
#define ID_PATCH_REVIEW				4
/**
 * \ingroup TortoiseMerge
 *
 * Dialog class, showing all files to patch from a unified diff file.
 */
class CFilePatchesDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CFilePatchesDlg)

public:
	CFilePatchesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFilePatchesDlg();

	/**
	 * Call this method to initialize the dialog.
	 * \param pPatch The CPatch object used to get the filenames from the unified diff file
	 * \param pCallBack The Object providing the callback interface (CPatchFilesDlgCallBack)
	 * \param sPath The path to the "parent" folder where the patch gets applied to
	 * \return TRUE if successful
	 */
	BOOL	Init(CPatch * pPatch, CPatchFilesDlgCallBack * pCallBack, CString sPath, CWnd * pParent);

	BOOL	SetFileStatusAsPatched(CString sPath);
	bool	HasFiles() {return m_cFileList.GetItemCount()>0;}
	enum { IDD = IDD_FILEPATCHES };
protected:
	CPatch *					m_pPatch;
	CPatchFilesDlgCallBack *	m_pCallBack;
	CString						m_sPath;
	CListCtrl					m_cFileList;
	CDWordArray					m_arFileStates;
	CImageList					m_ImgList;
	BOOL						m_bMinimized;
	int							m_nWindowHeight;
	CWnd *						m_pMainFrame;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnBnClickedPatchselectedbutton();
	afx_msg void OnBnClickedPatchallbutton();

	DECLARE_MESSAGE_MAP()

	void SetTitleWithPath(int width);
	void PatchAll();
	void PatchSelected();
};
