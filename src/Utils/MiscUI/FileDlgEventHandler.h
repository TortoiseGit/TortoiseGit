// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include <atlcom.h>

/**
 * helper class for IFileDialogEvents and IFileDialogControlEvents.
 * use this class as a base class so you only need to implement
 * the methods you actually need and not all of them.
 */
class CFileDlgEventHandler : public CComObjectRootEx<CComSingleThreadModel>,
						 public CComCoClass<CFileDlgEventHandler>,
						 public IFileDialogEvents,
						 public IFileDialogControlEvents
{
public:
	CFileDlgEventHandler();
	~CFileDlgEventHandler();

	BEGIN_COM_MAP(CFileDlgEventHandler)
		COM_INTERFACE_ENTRY(IFileDialogEvents)
		COM_INTERFACE_ENTRY(IFileDialogControlEvents)
	END_COM_MAP()

	// IFileDialogEvents
	virtual STDMETHODIMP OnFileOk(IFileDialog* pfd);
	virtual STDMETHODIMP OnFolderChanging(IFileDialog* pfd, IShellItem* psiFolder);
	virtual STDMETHODIMP OnFolderChange(IFileDialog* pfd);
	virtual STDMETHODIMP OnSelectionChange(IFileDialog* pfd);
	virtual STDMETHODIMP OnShareViolation(IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse);
	virtual STDMETHODIMP OnTypeChange(IFileDialog* pfd);
	virtual STDMETHODIMP OnOverwrite(IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse);

	// IFileDialogControlEvents
	virtual STDMETHODIMP OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem);
	virtual STDMETHODIMP OnButtonClicked(IFileDialogCustomize* pfdc, DWORD dwIDCtl);
	virtual STDMETHODIMP OnCheckButtonToggled(IFileDialogCustomize* pfdc, DWORD dwIDCtl, BOOL bChecked);
	virtual STDMETHODIMP OnControlActivating(IFileDialogCustomize* pfdc, DWORD dwIDCtl);
};
