// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2017-2018 - TortoiseGit
// Copyright (C) 2007-2008, 2010-2011, 2013 - TortoiseSVN

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

#pragma once

#include <initguid.h>
#include "GitWCRev.h"

/**
 * \ingroup GitWCRev
 * Implements the IGitWCRev interface of the COM object that GitWCRevCOM publishes.
 */
class GitWCRev : public IGitWCRev
{

	// Construction
public:
	GitWCRev();
	~GitWCRev();

	// IUnknown implementation
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) override;
	virtual ULONG __stdcall AddRef() override;
	virtual ULONG __stdcall Release() override;

	// IDispatch implementation
	virtual HRESULT __stdcall GetTypeInfoCount(UINT* pctinfo) override;
	virtual HRESULT __stdcall GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) override;
	virtual HRESULT __stdcall GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) override;
	virtual HRESULT __stdcall Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) override;

	// IGitWCRev implementation
	virtual HRESULT __stdcall GetWCInfo(/*[in]*/ BSTR wcPath, /*[in]*/VARIANT_BOOL ignore_submodules) override;

	virtual HRESULT __stdcall get_Revision(/*[out, retval]*/VARIANT* rev) override;

	virtual HRESULT __stdcall get_Branch(/*[out, retval]*/VARIANT* branch) override;

	virtual HRESULT __stdcall get_Date(/*[out, retval]*/VARIANT* date) override;

	virtual HRESULT __stdcall get_Author(/*[out, retval]*/VARIANT* author) override;

	virtual HRESULT __stdcall get_HasModifications(/*[out, retval]*/VARIANT_BOOL* modifications) override;

	virtual HRESULT __stdcall get_HasUnversioned(/*[out, retval]*/VARIANT_BOOL* unversioned) override;

	virtual HRESULT __stdcall get_IsWcTagged(/*[out, retval]*/VARIANT_BOOL* tagged) override;

	virtual HRESULT __stdcall get_IsGitItem(/*[out, retval]*/VARIANT_BOOL* versioned) override;

	virtual HRESULT __stdcall get_IsUnborn(/*[out, retval]*/VARIANT_BOOL* unborn) override;

	virtual HRESULT __stdcall get_HasSubmodule(/*[out, retval]*/VARIANT_BOOL* has_submodule) override;

	virtual HRESULT __stdcall get_HasSubmoduleModifications(/*[out, retval]*/VARIANT_BOOL* modifications) override;

	virtual HRESULT __stdcall get_HasSubmoduleUnversioned(/*[out, retval]*/VARIANT_BOOL* unversioned) override;

	virtual HRESULT __stdcall get_IsSubmoduleUp2Date(/*[out, retval]*/VARIANT_BOOL* up2date) override;

	virtual HRESULT __stdcall get_CommitCount(/*[out, retval]*/VARIANT* rev);

private:
	BOOL CopyDateToString(WCHAR* destbuf, int buflen, __time64_t time);

	HRESULT LoadTypeInfo(ITypeInfo ** pptinfo, const CLSID& libid, const CLSID& iid, LCID lcid);
	static HRESULT BoolToVariantBool(BOOL value, VARIANT_BOOL* result);
	static HRESULT LongToVariant(LONG value, VARIANT* result);
	static HRESULT Utf8StringToVariant(const char* string, VARIANT* result );
	HRESULT __stdcall GetWCInfoInternal(/*[in]*/ BSTR wcPath, /*[in]*/VARIANT_BOOL ignore_submodules);

	// Reference count
	long		m_cRef ;
	LPTYPEINFO	m_ptinfo; // pointer to type-library

	GitWCRev_t GitStat;
};

/**
 * \ingroup GitWCRev
 * Implements the IClassFactory interface of the GitWCRev COM object.
 * Used as global object only - no true reference counting in this class.
 */
class CFactory : public IClassFactory
{
public:
	// IUnknown
	virtual HRESULT	__stdcall QueryInterface(const IID& iid, void** ppv) override;
	virtual ULONG	__stdcall AddRef() override;
	virtual ULONG	__stdcall Release() override;

	// Interface IClassFactory
	virtual HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv) override;
	virtual HRESULT __stdcall LockServer(BOOL bLock) override;

	CFactory() {}
	~CFactory() {}
};
