// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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


/**
 * \ingroup TortoiseShell
 * This class factory object creates the main handlers -
 * its constructor says which OLE class it has to make.
 */
class CShellExtClassFactory : public IClassFactory
{
protected:
	ULONG m_cRef;
	/// variable to contain class of object (i.e. not under source control, up to date)
	FileState				m_StateToMake;

public:
	CShellExtClassFactory(FileState state);
	virtual ~CShellExtClassFactory();

	//@{
	/// IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *) override;
	STDMETHODIMP_(ULONG)	AddRef() override;
	STDMETHODIMP_(ULONG)	Release() override;
	//@}

	//@{
	/// IClassFactory members
	STDMETHODIMP			CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *) override;
	STDMETHODIMP			LockServer(BOOL) override;
	//@}
};
