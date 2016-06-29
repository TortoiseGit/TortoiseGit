// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2007-2008, 2010, 2012-2014 - TortoiseSVN

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
#include "TGitPath.h"
#include "DragDropImpl.h"
#include <vector>
#include <Shldisp.h>

extern CLIPFORMAT CF_FILECONTENTS;
extern CLIPFORMAT CF_FILEDESCRIPTOR;
extern CLIPFORMAT CF_PREFERREDDROPEFFECT;
extern CLIPFORMAT CF_INETURL;
extern CLIPFORMAT CF_SHELLURL;
extern CLIPFORMAT CF_FILE_ATTRIBUTES_ARRAY;


#define GITDATAOBJECT_NUMFORMATS 9

/**
 * \ingroup Git
 * Represents a Git path as an IDataObject.
 * This can be used for drag and drop operations to other applications like
 * the shell itself.
 */
class GitDataObject : public IDataObject, public IDataObjectAsyncCapability
{
public:
	/**
	 * Constructs the GitDataObject.
	 * \param svnpaths a list of paths which must be inside a working tree.
	 * \param rev      the revision
	 */
	GitDataObject(const CTGitPathList& gitpaths, const CGitHash& rev, int stripLength = -1);
	~GitDataObject();

	//IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	//IDataObject
	virtual HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium);
	virtual HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium);
	virtual HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pformatetc);
	virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC* pformatectIn, FORMATETC* pformatetcOut);
	virtual HRESULT STDMETHODCALLTYPE SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease);
	virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc);
	virtual HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection);
	virtual HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection);
	virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA** ppenumAdvise);

	//IDataObjectAsyncCapability
	virtual HRESULT STDMETHODCALLTYPE SetAsyncMode(BOOL fDoOpAsync);
	virtual HRESULT STDMETHODCALLTYPE GetAsyncMode(BOOL* pfIsOpAsync);
	virtual HRESULT STDMETHODCALLTYPE StartOperation(IBindCtx* pbcReserved);
	virtual HRESULT STDMETHODCALLTYPE InOperation(BOOL* pfInAsyncOp);
	virtual HRESULT STDMETHODCALLTYPE EndOperation(HRESULT hResult, IBindCtx* pbcReserved, DWORD dwEffects);

	HRESULT SetDropDescription(DROPIMAGETYPE image, LPCTSTR format, LPCTSTR insert);

private:
	void CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc);

private:
	CTGitPathList				m_gitPaths;
	CGitHash					m_revision;
	std::vector<CTGitPath>		m_allPaths;
	int							m_iStripLength;
	long						m_cRefCount;
	bool						m_containsExistingFiles;
	BOOL						m_bInOperation;
	BOOL						m_bIsAsync;
	std::vector<FORMATETC*>		m_vecFormatEtc;
	std::vector<STGMEDIUM*>		m_vecStgMedium;
};


/**
 * Helper class for the GitDataObject class: implements the enumerator
 * for the supported clipboard formats of the GitDataObject class.
 */
class CGitEnumFormatEtc : public IEnumFORMATETC
{
public:
	CGitEnumFormatEtc(const std::vector<FORMATETC*>& vec, bool localonly, bool containsExistingFiles);
	CGitEnumFormatEtc(const std::vector<FORMATETC>& vec, bool localonly, bool containsExistingFiles);
	//IUnknown members
	STDMETHOD(QueryInterface)(REFIID, void**);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	//IEnumFORMATETC members
	STDMETHOD(Next)(ULONG, LPFORMATETC, ULONG*);
	STDMETHOD(Skip)(ULONG);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumFORMATETC**);
private:
	void						Init(bool localonly, bool containsExistingFiles);
private:
	std::vector<FORMATETC>		m_vecFormatEtc;
	FORMATETC					m_formats[GITDATAOBJECT_NUMFORMATS];
	ULONG						m_cRefCount;
	size_t						m_iCur;
	bool						m_localonly;
	bool						m_containsExistingFiles;
};

