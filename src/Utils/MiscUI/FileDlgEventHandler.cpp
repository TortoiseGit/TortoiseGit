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
#include "stdafx.h"
#include "resource.h"
#include "FileDlgEventHandler.h"

CFileDlgEventHandler::CFileDlgEventHandler()
{
}

CFileDlgEventHandler::~CFileDlgEventHandler()
{
}


/////////////////////////////////////////////////////////////////////////////
// IFileDialogEvents methods

STDMETHODIMP CFileDlgEventHandler::OnFileOk ( IFileDialog* /*pfd*/ )
{
	return S_OK;	// allow the dialog to close
}

STDMETHODIMP CFileDlgEventHandler::OnFolderChanging ( IFileDialog* /*pfd*/, IShellItem* /*psiFolder*/ )
{
	return S_OK;	// allow the change
}

STDMETHODIMP CFileDlgEventHandler::OnFolderChange ( IFileDialog* /*pfd*/ )
{
	return S_OK;
}

STDMETHODIMP CFileDlgEventHandler::OnSelectionChange ( IFileDialog* /*pfd*/ )
{
	return S_OK;
}

STDMETHODIMP CFileDlgEventHandler::OnShareViolation ( IFileDialog* /*pfd*/, IShellItem* /*psi*/, FDE_SHAREVIOLATION_RESPONSE* /*pResponse*/ )
{
	return S_OK;
}

STDMETHODIMP CFileDlgEventHandler::OnTypeChange ( IFileDialog* /*pfd*/ )
{
	return S_OK;
}

STDMETHODIMP CFileDlgEventHandler::OnOverwrite ( IFileDialog* /*pfd*/, IShellItem* /*psi*/, FDE_OVERWRITE_RESPONSE* /*pResponse*/ )
{
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IFileDialogControlEvents methods

STDMETHODIMP CFileDlgEventHandler::OnItemSelected ( IFileDialogCustomize* /*pfdc*/, DWORD /*dwIDCtl*/, DWORD /*dwIDItem*/ )
{
	return S_OK;
}

STDMETHODIMP CFileDlgEventHandler::OnButtonClicked ( IFileDialogCustomize* /*pfdc*/, DWORD /*dwIDCtl*/ )
{
	return S_OK;
}

STDMETHODIMP CFileDlgEventHandler::OnCheckButtonToggled ( IFileDialogCustomize* /*pfdc*/, DWORD /*dwIDCtl*/, BOOL /*bChecked*/ )
{
	return S_OK;
}

STDMETHODIMP CFileDlgEventHandler::OnControlActivating ( IFileDialogCustomize* /*pfdc*/, DWORD /*dwIDCtl*/ )
{
	return S_OK;
}
