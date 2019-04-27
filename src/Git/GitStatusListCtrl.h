// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2003-2008, 2014 - TortoiseSVN

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
#include "TGitPath.h"
#include "GitStatus.h"
#include "Colors.h"
#include "ResizableColumnsListCtrl.h"
#include "DragDropImpl.h"
#include "ReaderWriterLock.h"

#define GIT_WC_ENTRY_WORKING_SIZE_UNKNOWN (-1)

// these defines must be in the order the columns are inserted!
#define GITSLC_COLFILENAME			0x000000002
#define GITSLC_COLEXT				0x000000004
#define GITSLC_COLSTATUS			0x000000008
//#define SVNSLC_COLAUTHOR			0x000000040
//#define	SVNSLC_COLREVISION			0x000000080
//#define	SVNSLC_COLDATE				0x000000100
#define GITSLC_COLADD				0x000000010
#define GITSLC_COLDEL				0x000000020
#define GITSLC_COLMODIFICATIONDATE	0x000000040
#define	GITSLC_COLSIZE				0x000000080
#define GITSLC_NUMCOLUMNS			8

//#define SVNSLC_COLURL				0x000000200
//#define SVNSLC_COLCOPYFROM			0x000020000

#define GITSLC_SHOWUNVERSIONED	CTGitPath::LOGACTIONS_UNVER
#define GITSLC_SHOWNORMAL		0x00000000
#define GITSLC_SHOWMODIFIED		(CTGitPath::LOGACTIONS_MODIFIED)
#define GITSLC_SHOWADDED		(CTGitPath::LOGACTIONS_ADDED|CTGitPath::LOGACTIONS_COPY)
#define GITSLC_SHOWREMOVED		CTGitPath::LOGACTIONS_DELETED
#define GITSLC_SHOWCONFLICTED	CTGitPath::LOGACTIONS_UNMERGED
#define GITSLC_SHOWMISSING		0x00000000
#define GITSLC_SHOWREPLACED		CTGitPath::LOGACTIONS_REPLACED
#define GITSLC_SHOWMERGED		CTGitPath::LOGACTIONS_MERGED
#define GITSLC_SHOWIGNORED		CTGitPath::LOGACTIONS_IGNORE
#define GITSLC_SHOWOBSTRUCTED	0x00000000
#define GITSLC_SHOWEXTERNAL		0x00000000
#define GITSLC_SHOWINCOMPLETE	0x00000000
#define GITSLC_SHOWINEXTERNALS	0x00000000
#define GITSLC_SHOWREMOVEDANDPRESENT 0x00000000
#define GITSLC_SHOWDIRECTFILES	0x04000000
#define GITSLC_SHOWDIRECTFOLDER 0x00000000
#define GITSLC_SHOWEXTERNALFROMDIFFERENTREPO 0x00000000
#define GITSLC_SHOWSWITCHED		0x00000000
#define GITSLC_SHOWINCHANGELIST 0x00000000
#define GITSLC_SHOWASSUMEVALID  CTGitPath::LOGACTIONS_ASSUMEVALID
#define GITSLC_SHOWSKIPWORKTREE CTGitPath::LOGACTIONS_SKIPWORKTREE

#define GITSLC_SHOWDIRECTS		(GITSLC_SHOWDIRECTFILES | GITSLC_SHOWDIRECTFOLDER)

#define GITSLC_SHOWFILES		0x01000000
#define GITSLC_SHOWSUBMODULES	0x02000000
#define GITSLC_SHOWEVERYTHING	0xffffffff

#define GITSLC_SHOWVERSIONED (GITSLC_SHOWNORMAL|GITSLC_SHOWMODIFIED|\
GITSLC_SHOWADDED|GITSLC_SHOWREMOVED|GITSLC_SHOWCONFLICTED|GITSLC_SHOWMISSING|\
GITSLC_SHOWREPLACED|GITSLC_SHOWMERGED|GITSLC_SHOWIGNORED|GITSLC_SHOWOBSTRUCTED|\
GITSLC_SHOWEXTERNAL|GITSLC_SHOWINCOMPLETE|GITSLC_SHOWINEXTERNALS|\
GITSLC_SHOWEXTERNALFROMDIFFERENTREPO)

#define GITSLC_SHOWVERSIONEDBUTNORMAL (GITSLC_SHOWMODIFIED|GITSLC_SHOWADDED|\
GITSLC_SHOWREMOVED|GITSLC_SHOWCONFLICTED|GITSLC_SHOWMISSING|\
GITSLC_SHOWREPLACED|GITSLC_SHOWMERGED|GITSLC_SHOWIGNORED|GITSLC_SHOWOBSTRUCTED|\
GITSLC_SHOWEXTERNAL|GITSLC_SHOWINCOMPLETE|GITSLC_SHOWINEXTERNALS|\
GITSLC_SHOWEXTERNALFROMDIFFERENTREPO)

#define GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS (GITSLC_SHOWMODIFIED|\
GITSLC_SHOWADDED|GITSLC_SHOWREMOVED|GITSLC_SHOWCONFLICTED|GITSLC_SHOWMISSING|\
GITSLC_SHOWREPLACED|GITSLC_SHOWMERGED|GITSLC_SHOWIGNORED|GITSLC_SHOWOBSTRUCTED|\
GITSLC_SHOWINCOMPLETE|GITSLC_SHOWEXTERNAL|GITSLC_SHOWINEXTERNALS)

#define GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS (GITSLC_SHOWMODIFIED|\
	GITSLC_SHOWADDED|GITSLC_SHOWREMOVED|GITSLC_SHOWCONFLICTED|GITSLC_SHOWMISSING|\
	GITSLC_SHOWREPLACED|GITSLC_SHOWMERGED|GITSLC_SHOWIGNORED|GITSLC_SHOWOBSTRUCTED|\
	GITSLC_SHOWINCOMPLETE)

#define GITSLC_SHOWALL (GITSLC_SHOWVERSIONED|GITSLC_SHOWUNVERSIONED)

#define GITSLC_POPALL					0xFFFFFFFFFFFFFFFF
#define GITSLC_POPCOMPAREWITHBASE		CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMPARE)
#define GITSLC_POPCOMPARE				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMPAREWC)
#define GITSLC_POPGNUDIFF				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_GNUDIFF1)
#define GITSLC_POPREVERT				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_REVERT)
#define GITSLC_POPSHOWLOG				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_LOG)
#define GITSLC_POPSHOWLOGSUBMODULE		CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_LOGSUBMODULE)
#define GITSLC_POPSHOWLOGOLDNAME		CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_LOGOLDNAME)
#define GITSLC_POPOPEN					CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_OPEN)
#define GITSLC_POPDELETE				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_DELETE)
#define GITSLC_POPADD					CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_ADD)
#define GITSLC_POPIGNORE				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_IGNORE)
#define GITSLC_POPCONFLICT				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_EDITCONFLICT)
#define GITSLC_POPRESOLVE				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_RESOLVECONFLICT)
#define GITSLC_POPEXPLORE				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_EXPLORE)
#define GITSLC_POPCOMMIT				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMMIT)
#define GITSLC_POPCHANGELISTS			CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_CHECKGROUP)
#define GITSLC_POPBLAME					CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_BLAME)
#define GITSLC_POPSAVEAS				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_SAVEAS)
#define GITSLC_POPCOMPARETWOFILES		CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMPARETWOFILES)
#define GITSLC_POPRESTORE				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_POPRESTORE)
#define GITSLC_POPASSUMEVALID			CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_ASSUMEVALID)
#define GITSLC_POPSKIPWORKTREE			CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_SKIPWORKTREE)
#define GITSLC_POPEXPORT				CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_EXPORT)
#define GITSLC_POPUNSETIGNORELOCALCHANGES CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_UNSETIGNORELOCALCHANGES)
#define GITSLC_POPPREPAREDIFF			CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_PREPAREDIFF)

#define GITSLC_IGNORECHANGELIST			L"ignore-on-commit"

#define OVL_RESTORE			1

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

class CGitStatusListCtrlDropTarget;

/**
* \ingroup TortoiseProc
* Simple utility class that defines the sort column order.
*/
class CSorter
{
public:

	CSorter ( ColumnManager* columnManager
		, int sortedColumn
		, bool ascending);

	bool operator() ( const CTGitPath* entry1
		, const CTGitPath* entry2) const;

	static int A2L(const CString &str)
	{
		if (str == L"-")
			return -1;

		return _wtol(str);
	}

private:

	ColumnManager* columnManager;
	int sortedColumn;
	bool ascending;
	bool s_bSortLogical;
};

/**
 * \ingroup SVN
 * A List control, based on the MFC CListCtrl which shows a list of
 * files with their git status. The control also provides a context
 * menu to do some git tasks on the selected files.
 *
 * This is the main control used in many dialogs to show a list of files to
 * work on.
 */
class CGitStatusListCtrl :
	public CResizableColumnsListCtrl<CListCtrl>
{
public:
	enum
	{
		IDGITLC_REVERT = 1,
		/** Compare with base version. when current version is zero (i.e. working tree changes), compare working tree and HEAD */
		IDGITLC_COMPARE,
		IDGITLC_OPEN,
		IDGITLC_DELETE,
		IDGITLC_IGNORE,
		/** Compare with base version and generate unified diff. when current version is zero (i.e. working tree changes), compare working tree and HEAD  */
		IDGITLC_GNUDIFF1		 ,
		IDGITLC_LOG              ,
		IDGITLC_LOGOLDNAME,
		IDGITLC_LOGSUBMODULE,
		IDGITLC_EDITCONFLICT     ,
		IDGITLC_IGNOREMASK	    ,
		IDGITLC_IGNOREFOLDER    ,
		IDGITLC_ADD			    ,
		IDGITLC_RESOLVECONFLICT ,
		IDGITLC_OPENWITH		,
		IDGITLC_EXPLORE			,
		IDGITLC_RESOLVETHEIRS	,
		IDGITLC_RESOLVEMINE		,
		IDGITLC_REMOVE			,
		IDGITLC_COMMIT			,
		IDGITLC_COPYRELPATHS	,
		IDGITLC_COPYEXT			,
		IDGITLC_REMOVEFROMCS	,
		IDGITLC_CREATECS		,
		IDGITLC_CREATEIGNORECS	,
		IDGITLC_CHECKGROUP		,
		IDGITLC_UNCHECKGROUP	,
		/** Compare current version and working tree */
		IDGITLC_COMPAREWC		,
		IDGITLC_BLAME			,
		IDGITLC_SAVEAS			,
		IDGITLC_REVERTTOREV		,
		IDGITLC_REVERTTOPARENT	,
		IDGITLC_VIEWREV			,
		IDGITLC_FINDENTRY       ,
		/** used in sync dlg, compare in/out file changes; in combination with m_Rev1 and m_Rev2 */
		IDGITLC_COMPARETWOREVISIONS,
		/** used in sync dlg, compare in/out file changes; in combination with m_Rev1 and m_Rev2 */
		IDGITLC_GNUDIFF2REVISIONS,
		/** Compare two selected files */
		IDGITLC_COMPARETWOFILES	,
		IDGITLC_POPRESTORE		,
		IDGITLC_CREATERESTORE	,
		IDGITLC_RESTOREPATH		,
		IDGITLC_ASSUMEVALID		,
		IDGITLC_SKIPWORKTREE	,
		IDGITLC_EXPORT			,
		IDGITLC_UNSETIGNORELOCALCHANGES,
		IDGITLC_PREPAREDIFF		,
		IDGITLC_PREPAREDIFF_COMPARE,
		IDGITLC_COPYCOL			,
		IDGITLC_COPYFULL		,
		IDGITLC_COPYFILENAMES	,
		IDGITLC_ADD_EXE			,
		IDGITLC_ADD_LINK		,
// the IDSVNLC_MOVETOCS *must* be the last index, because it contains a dynamic submenu where
// the submenu items get command ID's sequent to this number
		IDGITLC_MOVETOCS		,
	};
	static_assert(IDGITLC_MOVETOCS < 64, "IDs must be <64 in order to be usable in a bitfield");
	int GetColumnIndex(int colmask);
	static inline unsigned __int64 GetContextMenuBit(int i) { return static_cast<unsigned __int64>(0x1) << i; }
	/**
	 * Sent to the parent window (using ::SendMessage) after a context menu
	 * command has finished if the item count has changed.
	 */
	static const UINT GITSLNM_ITEMCOUNTCHANGED;
	/**
	 * Sent to the parent window (using ::SendMessage) when the control needs
	 * to be refreshed. Since this is done usually in the parent window using
	 * a thread, this message is used to tell the parent to do exactly that.
	 */
	static const UINT GITSLNM_NEEDSREFRESH;

	/**
	 * Sent to the parent window (using ::SendMessage) when the user drops
	 * files on the control. The LPARAM is a pointer to a TCHAR string
	 * containing the dropped path.
	 */
	static const UINT GITSLNM_ADDFILE;

	/**
	 * Sent to the parent window (using ::SendMessage) when the user checks/unchecks
	 * one or more items in the control. The WPARAM contains the number of
	 * checked items in the control.
	 */
	static const UINT GITSLNM_CHECKCHANGED;

	static const UINT GITSLNM_ITEMCHANGED;

	CGitStatusListCtrl(void);
	~CGitStatusListCtrl(void);

	HWND GetParentHWND();

	CGitHash m_Rev1;
	CGitHash m_Rev2;

	/**
	 * \ingroup TortoiseProc
	 * Helper class for CGitStatusListCtrl which represents
	 * the data for each file shown.
	 */
#if 0
	class FileEntry
	{
	public:
		FileEntry() : status(git_wc_status_unversioned)
//			, copyfrom_rev(GIT_REV_ZERO)
			, last_commit_date(0)
			, last_commit_rev(GIT_REV_ZERO)
//			, remoterev(GIT_REV_ZERO)
			, textstatus(git_wc_status_unversioned)
			, propstatus(git_wc_status_unversioned)
//			, remotestatus(git_wc_status_unversioned)
//			, remotetextstatus(git_wc_status_unversioned)
//			, remotepropstatus(git_wc_status_unversioned)
			, copied(false)
			, switched(false)
			, checked(false)
			, inunversionedfolder(false)
			, inexternal(false)
			, differentrepo(false)
			, direct(false)
			, isfolder(false)
			, isNested(false)
			, Revision(GIT_REV_ZERO)
			, isConflicted(false)
//			, present_props()
///			, working_size(SVN_WC_ENTRY_WORKING_SIZE_UNKNOWN)
//			, depth(git_depth_unknown)
		{
		}
		const CTGitPath& GetPath() const
		{
			return path;
		}
		const bool IsChecked() const
		{
			return checked;
		}
		CString GetRelativeGitPath() const
		{
			if (path.IsEquivalentTo(basepath))
				return path.GetGitPathString();
			return path.GetGitPathString().Mid(basepath.GetGitPathString().GetLength()+1);
		}
		const bool IsFolder() const
		{
			return isfolder;
		}
		const bool IsInExternal() const
		{
			return inexternal;
		}
		const bool IsNested() const
		{
			return isNested;
		}
		const bool IsFromDifferentRepository() const
		{
			return differentrepo;
		}
		CString GetDisplayName() const
		{
			CString const& chopped = path.GetDisplayString(&basepath);
			if (!chopped.IsEmpty())
			{
				return chopped;
			}
			else
			{
				// "Display name" must not be empty.
				return path.GetFileOrDirectoryName();
			}
		}
		CString GetChangeList() const
		{
			return changelist;
		}
//		CString GetURL() const
//		{
//			return url;
//		}
	public:
		git_wc_status_kind		status;					///< local status
		git_wc_status_kind		textstatus;				///< local text status
		git_wc_status_kind		propstatus;				///< local property status

	private:
		CTGitPath				path;					///< full path of the file
		CTGitPath				basepath;				///< common ancestor path of all files

		CString					changelist;				///< the name of the changelist the item belongs to

		CString					last_commit_author;		///< the author which last committed this item
		CTime					last_commit_date;		///< the date when this item was last committed
		git_revnum_t			last_commit_rev;		///< the revision where this item was last committed

		git_revnum_t			remoterev;				///< the revision in HEAD of the repository
		bool					copied;					///< if the file/folder is added-with-history
		bool					switched;				///< if the file/folder is switched to another url
		bool					checked;				///< if the file is checked in the list control
		bool					inunversionedfolder;	///< if the file is inside an unversioned folder
		bool					inexternal;				///< if the item is in an external folder
		bool					differentrepo;			///< if the item is from a different repository than the rest
		bool					direct;					///< directly included (TRUE) or just a child of a folder
		bool					isfolder;				///< TRUE if entry refers to a folder
		bool					isNested;				///< TRUE if the folder from a different repository and/or path
		bool					isConflicted;			///< TRUE if a file entry is conflicted, i.e. if it has the conflicted paths set
		git_revnum_t			Revision;				///< the base revision
//		PropertyList			present_props;			///< cacheable properties present in BASE
		git_depth_t				depth;					///< the depth of this entry
		friend class CGitStatusListCtrl;
		friend class CGitStatusListCtrlDropTarget;
		friend class CSorter;
	};
#endif

	/**
	 * Initializes the control, sets up the columns.
	 * \param dwColumns mask of columns to show. Use the GitSLC_COLxxx defines.
	 * \param sColumnInfoContainer Name of a registry key
	 *                             where the position and visibility of each column
	 *                             is saved and used from. If the registry key
	 *                             doesn't exist, the default order is used
	 *                             and dwColumns tells which columns are visible.
	 * \param dwContextMenus mask of context menus to be active, not all make sense for every use of this control.
	 *                       Use the GitSLC_POPxxx defines.
	 * \param bHasCheckboxes TRUE if the control should show check boxes on the left of each file entry.
	 * \param bHasWC TRUE if the reporisty is not a bare repository (hides wc related items on the contextmenu)
	 */
	void Init(DWORD dwColumns, const CString& sColumnInfoContainer, unsigned __int64 dwContextMenus, bool bHasCheckboxes = true, bool bHasWC = true, DWORD allowedColumns = 0xffffffff);
	/**
	 * Sets a background image for the list control.
	 * The image is shown in the right bottom corner.
	 * \param nID the resource ID of the bitmap to use as the background
	 */
	bool SetBackgroundImage(UINT nID);
private:
	UINT m_nBackgroundImageID;
public:
	/**
	 * Makes the 'ignore' context menu only ignore the files and not add the
	 * folder which gets the Git:ignore property changed to the list.
	 * This is needed e.g. for the Add-dialog, where the modified folder
	 * showing up would break the resulting "add" command.
	 */
	void SetIgnoreRemoveOnly(bool bRemoveOnly = true) {m_bIgnoreRemoveOnly = bRemoveOnly;}
	/**
	 * The unversioned items are by default shown after all other files in the list.
	 * If that behavior should be changed, set this value to false.
	 */
	void PutUnversionedLast(bool bLast) {m_bUnversionedLast = bLast;}
	/**
	 * Fetches the git status of all files and stores the information
	 * about them in an internal array.
	 * \param sFilePath path to a file which contains a list of files and/or folders for which to
	 *                  fetch the status, separated by newlines.
	 * \param bUpdate TRUE if the remote status is requested too.
	 * \return TRUE on success.
	 */
	BOOL GetStatus (const CTGitPathList* pathList = nullptr
				   , bool bUpdate = false
				   , bool bShowIgnores = false
				   , bool bShowUnRev = false
				   , bool bShowLocalChangesIgnored = false);

	/**
	 * Populates the list control with the previously (with GetStatus) gathered status information.
	 * \param dwShow mask of file types to show. Use the GitSLC_SHOWxxx defines.
	 * \param dwCheck mask of file types to check. Use GitLC_SHOWxxx defines. Default (0) means 'use the entry's stored check status'
	 */
	void Show(unsigned int dwShow, unsigned int dwCheck = 0, bool bShowFolders = true,BOOL updateStatusList=FALSE, bool UseStoredCheckStatus=false);

	/**
	 * Copies the selected entries in the control to the clipboard. The entries
	 * are separated by newlines.
	 * \param dwCols the columns to copy. Each column is separated by a tab.
	 */
	bool CopySelectedEntriesToClipboard(DWORD dwCols, int cmd);

	/**
	 * If unversioned files are found (but not necessarily shown) TRUE is returned.
	 */
	BOOL HasUnversionedItems() {return m_bHasUnversionedItems;}

	/**
	 * If there are any change lists defined in the working copy, TRUE is returned
	 */
	BOOL HasChangeLists() const {return m_bHasChangeLists;}

	/**
	 * Returns the file entry data for the list control index.
	 */
	CTGitPath* GetListEntry(int index);

	/**
	 * Returns the file entry data for the specified path.
	 * \note The entry might not be shown in the list control.
	 */
	//CGitStatusListCtrl::FileEntry * GetListEntry(const CTGitPath& path);

	/**
	 * Returns the file entry data for the specified path in the list control.
	 */
	//CGitStatusListCtrl::FileEntry * GetVisibleListEntry(const CTGitPath& path);

	/**
	 * Returns a String containing some statistics like number of modified, normal, deleted,...
	 * files.
	 */
	CString GetStatisticsString(bool simple=false);

	/**
	 * Set a static control which will be updated automatically with
	 * the number of selected and total files shown in the list control.
	 */
	void SetStatLabel(CWnd * pStatLabel){m_pStatLabel = pStatLabel;};

	/**
	 * Set a tri-state checkbox which is updated automatically if the
	 * user checks/unchecks file entries in the list control to indicate
	 * if all files are checked, none are checked or some are checked.
	 */
	void SetSelectButton(CButton * pButton) {m_pSelectButton = pButton;}

	/**
	 * Set a button which is de-/activated automatically. The button is
	 * only set active if at least one item is selected.
	 */
	void SetConfirmButton(CButton * pButton) {m_pConfirmButton = pButton;}

	/**
	 * Select/unselect all entries in the list control.
	 * \param bSelect TRUE to check, FALSE to uncheck.
	 */
	void SelectAll(bool bSelect, bool bIncludeNoCommits = false);

	/**
	 * Checks or unchecks all specified items
	 * \param dwCheck GITLC_SHOWxxx defines
	 * \param check if true matching items will be selected, false unchecks matching items
	 */
	void Check(DWORD dwCheck, bool check = true);

	/** Set a checkbox on an entry in the listbox
	 * Keeps the listctrl checked state and the FileEntry's checked flag in sync
	 */
	void SetEntryCheck(CTGitPath* pEntry, int listboxIndex, bool bCheck);

	void ResetChecked(const CTGitPath& entry);

	/** Write a list of the checked items' paths into a path list
	 */
	void WriteCheckedNamesToPathList(CTGitPathList& pathList);

	/**
	 * Returns the parent directory of all entries in the control.
	 * if \a bStrict is set to false, then the paths passed to the control
	 * to fetch the status (in GetStatus()) are used if possible.
	 */
	CString GetCommonDirectory(bool bStrict);

	/**
	 * Sets a pointer to a boolean variable which is checked periodically
	 * during the status fetching. As soon as the variable changes to true,
	 * the operations stops.
	 */
	void SetCancelBool(bool * pbCanceled) {m_pbCanceled = pbCanceled;}

	/**
	 * Sets the string shown in the control while the status is fetched.
	 * If not set, it defaults to "please wait..."
	 */
	void SetBusyString(const CString& str) {m_sBusy = str;}
	void SetBusyString(UINT id) {m_sBusy.LoadString(id);}

	/**
	 * Sets the string shown in the control if no items are shown. This
	 * can happen for example if there's nothing modified and the unversioned
	 * files aren't shown either, so there's nothing to commit.
	 * If not set, it defaults to "file list is empty".
	 */
	void SetEmptyString(const CString& str) {m_sEmpty = str;}
	void SetEmptyString(UINT id) {m_sEmpty.LoadString(id);}

	/**
	 * Returns the number of selected items
	 */
	LONG GetSelected(){return m_nSelected;};

	/**
	 * Enables dropping of files on the control.
	 */
	bool EnableFileDrop();

	/**
	 * Checks if the path already exists in the list.
	 */
	bool HasPath(const CTGitPath& path);

	/**
	 * Forces the children to be checked when the parent folder is checked,
	 * and the parent folder to be unchecked if one of its children is unchecked.
	 */
	void CheckChildrenWithParent(bool bCheck) {m_bCheckChildrenWithParent = bCheck;}

	/**
	 * Allows checking the items if change lists are present. If set to false,
	 * items are not checked if at least one changelist is available.
	 */
	void CheckIfChangelistsArePresent(bool bCheck) {m_bCheckIfGroupsExist = bCheck;}
	/**
	 * Returns the currently used show flags passed to the Show() method.
	 */
	DWORD GetShowFlags() {return m_dwShow;}

public:
	CString GetLastErrorMessage() {return m_sLastError;}

	void BusyCursor(bool bBusy) { m_bWaitCursor = bBusy; }

	LONG GetUnversionedCount() { return m_nShownUnversioned; }
	LONG GetModifiedCount() { return m_nShownModified; }
	LONG GetAddedCount() { return m_nShownAdded; }
	LONG GetDeletedCount() { return m_nShownDeleted; }
	LONG GetConflictedCount() { return m_nShownConflicted; }
	LONG GetFileCount() { return m_nShownFiles; }
	LONG GetSubmoduleCount() { return m_nShownSubmodules; }

	CAutoReadLock AcquireReadLock() { return CAutoReadLock(m_guard); }
	CAutoReadWeakLock AcquireReadWeakLock(DWORD timeout) { return CAutoReadWeakLock(m_guard, timeout); }

	LONG						m_nTargetCount;		///< number of targets in the file passed to GetStatus()

	CString						m_sURL;				///< the URL of the target or "(multiple targets)"

	bool						m_amend;			///< if true show the changes to the revision before the last commit

	bool						m_bIncludedStaged;

	CString						m_sUUID;			///< the UUID of the associated repository

	CString						m_sDisplayedBranch; ///< When on LogDialog, what is the current displayed branch

	CWnd						*m_hwndLogicalParent;

	DECLARE_MESSAGE_MAP()

public:
	void SetBusy(bool b) {m_bBusy = b; Invalidate();}
	bool IsBusy() const { return m_bBusy; }
	void SetHasCheckboxes(bool bHasCheckboxes)
	{
		m_bHasCheckboxes = bHasCheckboxes;
		DWORD exStyle = GetExtendedStyle();
		if (bHasCheckboxes)
			exStyle |= LVS_EX_CHECKBOXES;
		else
			exStyle &= ~LVS_EX_CHECKBOXES;
		SetExtendedStyle(exStyle);
	}

private:
	CString GetCellText(int listIndex, int column);    ///< get the text for a certain grid cell
	//void AddEntry(FileEntry * entry, WORD langID, int listIndex);	///< add an entry to the control
	void RemoveListEntry(int index);	///< removes an entry from the listcontrol and both arrays
	void BuildStatistics();	///< build the statistics and correct the case of files/folders
	void StartDiff(int fileindex);	///< start the external diff program
	void StartDiffWC(int fileindex);	///< start the external diff program
	void StartDiffTwo(int fileindex);

	void SetGitIndexFlagsForSelectedFiles(UINT message, BOOL assumevalid, BOOL skipworktree);

	CString m_sMarkForDiffFilename;
	CString m_sMarkForDiffVersion;

	/* while rebasing, Their and My versios are swapped. */
	bool						m_bIsRevertTheirMy;

	enum
	{
		ALTERNATIVEEDITOR,
		OPEN,
		OPEN_WITH,
	};
	void OpenFile(CTGitPath *path,int mode);

	/// Clear the status vector (contains custodial pointers)
	void ClearStatusArray();

	/// Sort predicate function - Compare the paths of two entries without regard to case
	//static bool EntryPathCompareNoCase(const FileEntry* pEntry1, const FileEntry* pEntry2);

	/// Predicate used to build a list of only the versioned entries of the FileEntry array
	//static bool IsEntryVersioned(const FileEntry* pEntry1);

	/// Look up the relevant show flags for a particular Git status value
	DWORD GetShowFlagsFromGitStatus(git_wc_status_kind status);

	/// Adjust the checkbox-state on all descendants of a specific item
	//void SetCheckOnAllDescendentsOf(const FileEntry* parentEntry, bool bCheck);

	/// Build a path list of all the selected items in the list (NOTE - SELECTED, not CHECKED)
	void FillListOfSelectedItemPaths(CTGitPathList& pathList, bool bNoIgnored = false);

	/// Enables/Disables group view and adds all groups to the list control.
	/// If bForce is true, then group view is enabled and the 'null' group is added.
	bool PrepareGroups(bool bForce = false);
	/// Returns the group number to which the group header belongs
	/// If the point is not over a group header, -1 is returned
	int GetGroupFromPoint(POINT * ppt);

	/// Returns the number of change lists the selection has
	bool HasChangelistInSelection();
	void MoveToChangelist(const CString& name);
	void RemoveFromChangelist();
	void LoadChangelists();

public:
	void SaveChangelists();
	void PruneChangelists();

private:
	int GetChangeListIdForPath(const CTGitPath* pGitPath);

	// Determines group and puts the item to the group
	bool SetItemGroup(int item, const CTGitPath* pGitPath);

	void CheckEntry(int index, int nListItems);
	void UncheckEntry(int index, int nListItems);

	/// sends an GitSLNM_CHECKCHANGED notification to the parent
	void NotifyCheck();
	CWnd* GetLogicalParent() { return m_hwndLogicalParent ? m_hwndLogicalParent : this->GetParent(); }

	void OnContextMenuList(CWnd * pWnd, CPoint point);
	void OnContextMenuGroup(CWnd * pWnd, CPoint point);
	bool CheckMultipleDiffs();

	void DeleteSelectedFiles();

	virtual void PreSubclassWindow() override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
	virtual ULONG GetGestureStatus(CPoint ptTouch) override;
	afx_msg void OnSysColorChange();
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnNMReturn(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();

	void FilesExport();
	void FileSaveAs(CTGitPath *path);
	int RevertSelectedItemToVersion(bool parent = false);

private:
	bool *						m_pbCanceled;
	bool					    m_bAscending;		///< sort direction
	int					        m_nSortedColumn;	///< which column to sort
	bool						m_bHasCheckboxes;
	bool						m_bHasWC;
	bool						m_bUnversionedLast;
	BOOL						m_bHasUnversionedItems;
	bool						m_bHasChangeLists;
	//typedef std::vector<FileEntry*> FileEntryVector;
	//FileEntryVector				m_arStatusArray;
	std::vector<const CTGitPath*>	m_arStatusArray;
	std::vector<size_t>			m_arListArray;
	std::map<CString, int>		m_changelists; // maps changelist to group index
	std::map<CString, CString>	m_pathToChangelist; // maps gitpath to changelist
	bool						m_bHasIgnoreGroup;
	CTGitPathList				m_StatusFileList;
	CTGitPathList				m_UnRevFileList;
	CTGitPathList				m_IgnoreFileList;
	CTGitPathList				m_LocalChangesIgnoredFileList; // assume valid & skip worktree
	CString						m_sLastError;

	LONG						m_nUnversioned;
	LONG						m_nNormal;
	LONG						m_nModified;
	LONG						m_nAdded;
	LONG						m_nDeleted;
	LONG						m_nConflicted;
	LONG						m_nTotal;
	LONG						m_nSelected;
	LONG						m_nLineAdded;
	LONG						m_nLineDeleted;
	LONG						m_nRenamed;

	LONG						m_nShownUnversioned;
	LONG						m_nShownModified;
	LONG						m_nShownAdded;
	LONG						m_nShownDeleted;
	LONG						m_nShownConflicted;
	LONG						m_nShownFiles;
	LONG						m_nShownSubmodules;

	DWORD						m_dwDefaultColumns;
	DWORD						m_dwShow;
	bool						m_bShowFolders;
	bool						m_bShowIgnores;
	bool						m_bUpdate;
	unsigned __int64			m_dwContextMenus;
	bool						m_bBusy;
	bool						m_bWaitCursor;
	bool						m_bEmpty;
	bool						m_bIgnoreRemoveOnly;
	bool						m_bCheckIfGroupsExist;
	bool						m_bFileDropsEnabled;
	bool						m_bOwnDrag;

	int							m_nIconFolder;
	int							m_nRestoreOvl;

	CWnd *						m_pStatLabel;
	CButton *					m_pSelectButton;
	CButton *					m_pConfirmButton;
	CColors						m_Colors;

	CString						m_sEmpty;
	CString						m_sBusy;

	bool						m_bCheckChildrenWithParent;
	std::unique_ptr<CGitStatusListCtrlDropTarget> m_pDropTarget;
	volatile LONG				m_nBlockItemChangeHandler;
	std::map<CString,bool>		m_mapFilenameToChecked; ///< Remember de-/selected items
	std::set<CString>			m_setDirectFiles;

	friend class CGitStatusListCtrlDropTarget;
public:
	enum
	{
		FILELIST_MODIFY= 0x1,
		FILELIST_UNVER = 0x2,
		FILELIST_IGNORE =0x4,
		FILELIST_LOCALCHANGESIGNORED = 0x8, // assume valid & skip worktree files
	};
private:
	int UpdateFileList(const CTGitPathList* list = nullptr);
public:
	int UpdateFileList(int mask, bool once = true, const CTGitPathList* list = nullptr);
	int UpdateUnRevFileList(CTGitPathList &list);
	int UpdateUnRevFileList(const CTGitPathList* list = nullptr);
	int UpdateIgnoreFileList(const CTGitPathList* list = nullptr);
	int UpdateLocalChangesIgnoredFileList(const CTGitPathList* list = nullptr);

	int UpdateWithGitPathList(CTGitPathList &list);

	void AddEntry(size_t arStatusArrayIndex, CTGitPath* path, WORD langID, int ListIndex);
	void Clear();
	int m_FileLoaded;
	CGitHash m_CurrentVersion;
	bool m_bDoNotAutoselectSubmodules;
	bool m_bNoAutoselectMissing;
	std::map<CString, CString>	m_restorepaths;
	mutable CReaderWriterLock	m_guard;

	HFONT			m_uiFont;

	HMENU			m_hShellMenu;
	LPCONTEXTMENU	m_pContextMenu;

	void			StoreScrollPos();

private:
	void			RestoreScrollPos();
	struct ScrollPos
	{
		bool enabled = false;
		int nTopIndex;
		int selMark;
		int nSelectedEntry;
	} m_sScrollPos;
};

class CGitStatusListCtrlDropTarget : public CIDropTarget
{
public:
	CGitStatusListCtrlDropTarget(CGitStatusListCtrl* pGitStatusListCtrl) : CIDropTarget(pGitStatusListCtrl->m_hWnd) { m_pGitStatusListCtrl = pGitStatusListCtrl; }

	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* pdwEffect, POINTL pt) override;
	virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect) override;
private:
	CGitStatusListCtrl* m_pGitStatusListCtrl;
};

class ScopedInDecrement
{
public:
	ScopedInDecrement(volatile LONG& counter) : m_counter(counter) { InterlockedIncrement(&m_counter); }
	~ScopedInDecrement() { InterlockedDecrement(&m_counter); }

	ScopedInDecrement(const ScopedInDecrement&) = delete;
	void operator=(const ScopedInDecrement&) = delete;

private:
	volatile LONG& m_counter;
};
