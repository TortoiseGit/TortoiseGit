// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2019 - TortoiseGit
// Copyright (C) 2007-2014 - TortoiseSVN

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
#include "stdafx.h"
#include "GitDataObject.h"
#include "Git.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "TempFile.h"
#include "StringUtils.h"
#include <strsafe.h>

CLIPFORMAT CF_FILECONTENTS = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILECONTENTS));
CLIPFORMAT CF_FILEDESCRIPTOR = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR));
CLIPFORMAT CF_PREFERREDDROPEFFECT = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT));
CLIPFORMAT CF_INETURL = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_INETURL));
CLIPFORMAT CF_SHELLURL = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_SHELLURL));
CLIPFORMAT CF_FILE_ATTRIBUTES_ARRAY = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILE_ATTRIBUTES_ARRAY));

GitDataObject::GitDataObject(const CTGitPathList& gitpaths, const CGitHash& rev, int stripLength)
	: m_gitPaths(gitpaths)
	, m_revision(rev)
	, m_bInOperation(FALSE)
	, m_bIsAsync(TRUE)
	, m_cRefCount(0)
	, m_iStripLength(stripLength)
	, m_containsExistingFiles(false)
{
	ASSERT((m_revision.IsEmpty() && m_iStripLength == -1) || (!m_revision.IsEmpty() && m_iStripLength >= -1)); // m_iStripLength only possible if rev is set
	for (int i = 0; i < m_gitPaths.GetCount(); ++i)
	{
		if ((m_gitPaths[i].m_Action == 0 || (m_gitPaths[i].m_Action & ~(CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED))) && !m_gitPaths[i].IsDirectory())
		{
			m_containsExistingFiles = true;
			break;
		}
	}
}

GitDataObject::~GitDataObject()
{
	for (size_t i = 0; i < m_vecStgMedium.size(); ++i)
	{
		ReleaseStgMedium(m_vecStgMedium[i]);
		delete m_vecStgMedium[i];
	}

	for (size_t j = 0; j < m_vecFormatEtc.size(); ++j)
		delete m_vecFormatEtc[j];
}

//////////////////////////////////////////////////////////////////////////
// IUnknown
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP GitDataObject::QueryInterface(REFIID riid, void** ppvObject)
{
	if (!ppvObject)
		return E_POINTER;
	*ppvObject = NULL;
	if (IsEqualIID(IID_IUnknown, riid) || IsEqualIID(IID_IDataObject, riid))
		*ppvObject = static_cast<IDataObject*>(this);
	else if (IsEqualIID(riid, IID_IDataObjectAsyncCapability))
		*ppvObject = static_cast<IDataObjectAsyncCapability*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) GitDataObject::AddRef(void)
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) GitDataObject::Release(void)
{
	--m_cRefCount;
	if (m_cRefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_cRefCount;
}

//////////////////////////////////////////////////////////////////////////
// IDataObject
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP GitDataObject::GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
	if (!pformatetcIn)
		return E_INVALIDARG;
	if (!pmedium)
		return E_POINTER;
	pmedium->hGlobal = nullptr;

	if ((pformatetcIn->tymed & TYMED_ISTREAM) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILECONTENTS))
	{
		// supports the IStream format.
		// The lindex param is the index of the file to return
		CString filepath;
		IStream* pIStream = nullptr;

		// Note: we don't get called for directories since those are simply created and don't
		// need to be fetched.

		// Note2: It would be really nice if we could get a stream from the subversion library
		// from where we could read the file contents. But the Subversion lib is not implemented
		// to *read* from a remote stream but so that the library *writes* to a stream we pass.
		// Since we can't get such a read stream, we have to fetch the file in whole first to
		// a temp location and then pass the shell an IStream for that temp file.

		if (m_revision.IsEmpty())
		{
			if (pformatetcIn->lindex >= 0 && pformatetcIn->lindex < static_cast<LONG>(m_allPaths.size()))
				filepath = g_Git.CombinePath(m_allPaths[pformatetcIn->lindex]);
		}
		else
		{
			filepath = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
			if (pformatetcIn->lindex >= 0 && pformatetcIn->lindex < static_cast<LONG>(m_allPaths.size()))
			{
				if (g_Git.GetOneFile(m_revision.ToString(), m_allPaths[pformatetcIn->lindex], filepath))
				{
					DeleteFile(filepath);
					return STG_E_ACCESSDENIED;
				}
			}
		}

		HRESULT res = SHCreateStreamOnFileEx(filepath, STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &pIStream);
		if (res == S_OK)
		{
			// http://blogs.msdn.com/b/oldnewthing/archive/2014/09/18/10558763.aspx
			LARGE_INTEGER liZero = { 0, 0 };
			pIStream->Seek(liZero, STREAM_SEEK_END, nullptr);

			pmedium->pstm = pIStream;
			pmedium->tymed = TYMED_ISTREAM;
			return S_OK;
		}
		return res;
	}
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILEDESCRIPTOR))
	{
		for (int i = 0; i < m_gitPaths.GetCount(); ++i)
		{
			if (m_gitPaths[i].m_Action & (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED) || m_gitPaths[i].IsDirectory())
				continue;
			m_allPaths.push_back(m_gitPaths[i]);
		}

		size_t dataSize = sizeof(FILEGROUPDESCRIPTOR) + ((max(size_t(1), m_allPaths.size()) - 1) * sizeof(FILEDESCRIPTOR));
		HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, dataSize);

		auto files = static_cast<FILEGROUPDESCRIPTOR*>(GlobalLock(data));
		files->cItems = static_cast<UINT>(m_allPaths.size());
		int index = 0;
		for (auto it = m_allPaths.cbegin(); it != m_allPaths.cend(); ++it)
		{
			CString temp(m_iStripLength > 0 ? it->GetWinPathString().Mid(m_iStripLength + 1) : (m_iStripLength == 0 ? it->GetWinPathString() : it->GetUIFileOrDirectoryName()));
			if (temp.GetLength() < MAX_PATH)
				wcscpy_s(files->fgd[index].cFileName, static_cast<LPCTSTR>(temp));
			else
			{
				files->cItems--;
				continue;
			}
			files->fgd[index].dwFlags = FD_ATTRIBUTES | FD_PROGRESSUI | FD_FILESIZE | FD_LINKUI;
			files->fgd[index].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

			// Always set the file size to 0 even if we 'know' the file size (infodata.size64).
			// Because for text files, the file size is too low due to the EOLs getting converted
			// to CRLF (from LF as stored in the repository). And a too low file size set here
			// prevents the shell from reading the full file later - it only reads the stream up
			// to the number of bytes specified here. Which means we would end up with a truncated
			// text file (binary files are still ok though).
			files->fgd[index].nFileSizeLow = 0;
			files->fgd[index].nFileSizeHigh = 0;

			++index;
		}

		GlobalUnlock(data);

		pmedium->hGlobal = data;
		pmedium->tymed = TYMED_HGLOBAL;
		return S_OK;
	}
	// handling CF_PREFERREDDROPEFFECT is necessary to tell the shell that it should *not* ask for the
	// CF_FILEDESCRIPTOR until the drop actually occurs. If we don't handle CF_PREFERREDDROPEFFECT, the shell
	// will ask for the file descriptor for every object (file/folder) the mouse pointer hovers over and is
	// a potential drop target.
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->cfFormat == CF_PREFERREDDROPEFFECT))
	{
		HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, sizeof(DWORD));
		if (!data)
			return E_OUTOFMEMORY;
		auto effect = static_cast<DWORD*>(GlobalLock(data));
		if (!effect)
		{
			GlobalFree(data);
			return E_OUTOFMEMORY;
		}
		(*effect) = DROPEFFECT_COPY;
		GlobalUnlock(data);
		pmedium->hGlobal = data;
		pmedium->tymed = TYMED_HGLOBAL;
		return S_OK;
	}
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_TEXT))
	{
		// caller wants text
		// create the string from the path list
		CString text;
		if (!m_gitPaths.IsEmpty())
		{
			// create a single string where the URLs are separated by newlines
			for (int i = 0; i < m_gitPaths.GetCount(); ++i)
			{
				text += m_gitPaths[i].GetWinPathString();
				text += L"\r\n";
			}
		}
		CStringA texta = CUnicodeUtils::GetUTF8(text);
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GlobalAlloc(GHND, (texta.GetLength() + 1) * sizeof(char));
		if (pmedium->hGlobal)
		{
			auto pMem = static_cast<char*>(GlobalLock(pmedium->hGlobal));
			strcpy_s(pMem, texta.GetLength() + 1, static_cast<LPCSTR>(texta));
			GlobalUnlock(pmedium->hGlobal);
		}
		pmedium->pUnkForRelease = nullptr;
		return S_OK;
	}
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && ((pformatetcIn->cfFormat == CF_UNICODETEXT) || (pformatetcIn->cfFormat == CF_INETURL) || (pformatetcIn->cfFormat == CF_SHELLURL)))
	{
		// caller wants Unicode text
		// create the string from the path list
		CString text;
		if (!m_gitPaths.IsEmpty())
		{
			// create a single string where the URLs are separated by newlines
			for (int i = 0; i < m_gitPaths.GetCount(); ++i)
			{
				if (pformatetcIn->cfFormat == CF_UNICODETEXT)
					text += m_gitPaths[i].GetWinPathString();
				else
					text += g_Git.CombinePath(m_gitPaths[i]);
				text += L"\r\n";
			}
		}
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GlobalAlloc(GHND, (text.GetLength() + 1) * sizeof(TCHAR));
		if (pmedium->hGlobal)
		{
			auto pMem = static_cast<TCHAR*>(GlobalLock(pmedium->hGlobal));
			wcscpy_s(pMem, text.GetLength() + 1, static_cast<LPCTSTR>(text));
			GlobalUnlock(pmedium->hGlobal);
		}
		pmedium->pUnkForRelease = nullptr;
		return S_OK;
	}
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_HDROP))
	{
		int nLength = 0;

		for (int i = 0; i < m_gitPaths.GetCount(); ++i)
		{
			if (m_gitPaths[i].m_Action & (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED) || m_gitPaths[i].IsDirectory())
				continue;

			nLength += g_Git.CombinePath(m_gitPaths[i]).GetLength();
			nLength += 1; // '\0' separator
		}

		int nBufferSize = sizeof(DROPFILES) + (nLength + 1) * sizeof(TCHAR);
		auto pBuffer = std::make_unique<char[]>(nBufferSize);
		SecureZeroMemory(pBuffer.get(), nBufferSize);

		auto df = reinterpret_cast<DROPFILES*>(pBuffer.get());
		df->pFiles = sizeof(DROPFILES);
		df->fWide = 1;

		auto pFilenames = reinterpret_cast<TCHAR*>(pBuffer.get() + sizeof(DROPFILES));
		TCHAR* pCurrentFilename = pFilenames;

		for (int i = 0; i < m_gitPaths.GetCount(); ++i)
		{
			if (m_gitPaths[i].m_Action & (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED) || m_gitPaths[i].IsDirectory())
				continue;
			CString str = g_Git.CombinePath(m_gitPaths[i]);
			wcscpy_s(pCurrentFilename, str.GetLength() + 1, static_cast<LPCWSTR>(str));
			pCurrentFilename += str.GetLength();
			*pCurrentFilename = '\0'; // separator between file names
			pCurrentFilename++;
		}
		*pCurrentFilename = '\0'; // terminate array

		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, nBufferSize);
		if (pmedium->hGlobal)
		{
			LPVOID pMem = ::GlobalLock(pmedium->hGlobal);
			if (pMem)
				memcpy(pMem, pBuffer.get(), nBufferSize);
			GlobalUnlock(pmedium->hGlobal);
		}
		pmedium->pUnkForRelease = nullptr;
		return S_OK;
	}
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILE_ATTRIBUTES_ARRAY))
	{
		int nBufferSize = sizeof(FILE_ATTRIBUTES_ARRAY) + m_gitPaths.GetCount() * sizeof(DWORD);
		auto pBuffer = std::make_unique<char[]>(nBufferSize);
		SecureZeroMemory(pBuffer.get(), nBufferSize);

		auto cf = reinterpret_cast<FILE_ATTRIBUTES_ARRAY*>(pBuffer.get());
		cf->cItems = m_gitPaths.GetCount();
		cf->dwProductFileAttributes = DWORD_MAX;
		cf->dwSumFileAttributes = 0;
		for (int i = 0; i < m_gitPaths.GetCount(); ++i)
		{
			DWORD fileattribs = FILE_ATTRIBUTE_NORMAL;
			cf->rgdwFileAttributes[i] = fileattribs;
			cf->dwProductFileAttributes &= fileattribs;
			cf->dwSumFileAttributes |= fileattribs;
		}

		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, nBufferSize);
		if (pmedium->hGlobal)
		{
			LPVOID pMem = ::GlobalLock(pmedium->hGlobal);
			if (pMem)
				memcpy(pMem, pBuffer.get(), nBufferSize);
			GlobalUnlock(pmedium->hGlobal);
		}
		pmedium->pUnkForRelease = nullptr;
		return S_OK;
	}

	for (size_t i = 0; i < m_vecFormatEtc.size(); ++i)
	{
		if ((pformatetcIn->tymed == m_vecFormatEtc[i]->tymed) &&
			(pformatetcIn->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
			(pformatetcIn->cfFormat == m_vecFormatEtc[i]->cfFormat))
		{
			CopyMedium(pmedium, m_vecStgMedium[i], m_vecFormatEtc[i]);
			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

STDMETHODIMP GitDataObject::GetDataHere(FORMATETC* /*pformatetc*/, STGMEDIUM* /*pmedium*/)
{
	return E_NOTIMPL;
}

STDMETHODIMP GitDataObject::QueryGetData(FORMATETC* pformatetc)
{
	if (!pformatetc)
		return E_INVALIDARG;

	if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
		return DV_E_DVASPECT;

	if ((pformatetc->tymed & TYMED_ISTREAM) &&
		(pformatetc->dwAspect == DVASPECT_CONTENT) &&
		(pformatetc->cfFormat == CF_FILECONTENTS) &&
		m_containsExistingFiles)
	{
		return S_OK;
	}
	if ((pformatetc->tymed & TYMED_HGLOBAL) &&
		(pformatetc->dwAspect == DVASPECT_CONTENT) &&
		((pformatetc->cfFormat == CF_TEXT) || (pformatetc->cfFormat == CF_UNICODETEXT) || (pformatetc->cfFormat == CF_PREFERREDDROPEFFECT)))
	{
		return S_OK;
	}
	if ((pformatetc->tymed & TYMED_HGLOBAL) &&
		(pformatetc->dwAspect == DVASPECT_CONTENT) &&
		(pformatetc->cfFormat == CF_FILEDESCRIPTOR) &&
		!m_revision.IsEmpty() &&
		m_containsExistingFiles)
	{
		return S_OK;
	}
	if ((pformatetc->tymed & TYMED_HGLOBAL) &&
		(pformatetc->dwAspect == DVASPECT_CONTENT) &&
		((pformatetc->cfFormat == CF_HDROP) || (pformatetc->cfFormat == CF_INETURL) || (pformatetc->cfFormat == CF_SHELLURL)) &&
		m_revision.IsEmpty() &&
		m_containsExistingFiles)
	{
		return S_OK;
	}
	if ((pformatetc->tymed & TYMED_HGLOBAL) &&
		(pformatetc->dwAspect == DVASPECT_CONTENT) &&
		(pformatetc->cfFormat == CF_FILE_ATTRIBUTES_ARRAY) &&
		m_containsExistingFiles)
	{
		return S_OK;
	}

	for (size_t i = 0; i < m_vecFormatEtc.size(); ++i)
	{
		if ((pformatetc->tymed == m_vecFormatEtc[i]->tymed) &&
			(pformatetc->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
			(pformatetc->cfFormat == m_vecFormatEtc[i]->cfFormat))
			return S_OK;
	}

	return DV_E_TYMED;
}

STDMETHODIMP GitDataObject::GetCanonicalFormatEtc(FORMATETC* /*pformatectIn*/, FORMATETC* pformatetcOut)
{
	if (!pformatetcOut)
		return E_INVALIDARG;
	return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP GitDataObject::SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease)
{
	if (!pformatetc || !pmedium)
		return E_INVALIDARG;

	FORMATETC* fetc = new (std::nothrow) FORMATETC;
	STGMEDIUM* pStgMed = new (std::nothrow) STGMEDIUM;

	if (!fetc || !pStgMed)
	{
		delete fetc;
		delete pStgMed;
		return E_OUTOFMEMORY;
	}
	SecureZeroMemory(fetc, sizeof(FORMATETC));
	SecureZeroMemory(pStgMed, sizeof(STGMEDIUM));

	// do we already store this format?
	for (size_t i = 0; i < m_vecFormatEtc.size(); ++i)
	{
		if ((pformatetc->tymed == m_vecFormatEtc[i]->tymed) &&
			(pformatetc->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
			(pformatetc->cfFormat == m_vecFormatEtc[i]->cfFormat))
		{
			// yes, this format is already in our object:
			// we have to replace the existing format. To do that, we
			// remove the format we already have
			delete m_vecFormatEtc[i];
			m_vecFormatEtc.erase(m_vecFormatEtc.begin() + i);
			ReleaseStgMedium(m_vecStgMedium[i]);
			delete m_vecStgMedium[i];
			m_vecStgMedium.erase(m_vecStgMedium.begin() + i);
			break;
		}
	}

	*fetc = *pformatetc;
	m_vecFormatEtc.push_back(fetc);

	if (fRelease)
		*pStgMed = *pmedium;
	else
		CopyMedium(pStgMed, pmedium, pformatetc);
	m_vecStgMedium.push_back(pStgMed);

	return S_OK;
}

STDMETHODIMP GitDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
{
	if (!ppenumFormatEtc)
		return E_POINTER;

	*ppenumFormatEtc = nullptr;
	switch (dwDirection)
	{
	case DATADIR_GET:
		*ppenumFormatEtc = new (std::nothrow) CGitEnumFormatEtc(m_vecFormatEtc, m_revision.IsEmpty(), m_containsExistingFiles);
		if (!*ppenumFormatEtc)
			return E_OUTOFMEMORY;
		(*ppenumFormatEtc)->AddRef();
		break;
	default:
		return E_NOTIMPL;
	}
	return S_OK;
}

STDMETHODIMP GitDataObject::DAdvise(FORMATETC* /*pformatetc*/, DWORD /*advf*/, IAdviseSink* /*pAdvSink*/, DWORD* /*pdwConnection*/)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP GitDataObject::DUnadvise(DWORD /*dwConnection*/)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE GitDataObject::EnumDAdvise(IEnumSTATDATA** /*ppenumAdvise*/)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

void GitDataObject::CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
{
	switch (pMedSrc->tymed)
	{
	case TYMED_HGLOBAL:
		pMedDest->hGlobal = static_cast<HGLOBAL>(OleDuplicateData(pMedSrc->hGlobal, pFmtSrc->cfFormat, 0));
		break;
	case TYMED_GDI:
		pMedDest->hBitmap = static_cast<HBITMAP>(OleDuplicateData(pMedSrc->hBitmap, pFmtSrc->cfFormat, 0));
		break;
	case TYMED_MFPICT:
		pMedDest->hMetaFilePict = static_cast<HMETAFILEPICT>(OleDuplicateData(pMedSrc->hMetaFilePict, pFmtSrc->cfFormat, 0));
		break;
	case TYMED_ENHMF:
		pMedDest->hEnhMetaFile = static_cast<HENHMETAFILE>(OleDuplicateData(pMedSrc->hEnhMetaFile, pFmtSrc->cfFormat, 0));
		break;
	case TYMED_FILE:
		pMedSrc->lpszFileName = static_cast<LPOLESTR>(OleDuplicateData(pMedSrc->lpszFileName, pFmtSrc->cfFormat, 0));
		break;
	case TYMED_ISTREAM:
		pMedDest->pstm = pMedSrc->pstm;
		pMedSrc->pstm->AddRef();
		break;
	case TYMED_ISTORAGE:
		pMedDest->pstg = pMedSrc->pstg;
		pMedSrc->pstg->AddRef();
		break;
	case TYMED_NULL:
	default:
		break;
	}
	pMedDest->tymed = pMedSrc->tymed;
	pMedDest->pUnkForRelease = nullptr;
	if (pMedSrc->pUnkForRelease)
	{
		pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
		pMedSrc->pUnkForRelease->AddRef();
	}
}


//////////////////////////////////////////////////////////////////////////
// IAsyncOperation
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE GitDataObject::SetAsyncMode(BOOL fDoOpAsync)
{
	m_bIsAsync = fDoOpAsync;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE GitDataObject::GetAsyncMode(BOOL* pfIsOpAsync)
{
	if (!pfIsOpAsync)
		return E_FAIL;

	*pfIsOpAsync = m_bIsAsync;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE GitDataObject::StartOperation(IBindCtx* /*pbcReserved*/)
{
	m_bInOperation = TRUE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE GitDataObject::InOperation(BOOL* pfInAsyncOp)
{
	if (!pfInAsyncOp)
		return E_FAIL;

	*pfInAsyncOp = m_bInOperation;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE GitDataObject::EndOperation(HRESULT /*hResult*/, IBindCtx* /*pbcReserved*/, DWORD /*dwEffects*/)
{
	m_bInOperation = FALSE;
	return S_OK;
}

HRESULT GitDataObject::SetDropDescription(DROPIMAGETYPE image, LPCTSTR format, LPCTSTR insert)
{
	if (!format || !insert)
		return E_INVALIDARG;

	FORMATETC fetc = { 0 };
	fetc.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_DROPDESCRIPTION));
	fetc.dwAspect = DVASPECT_CONTENT;
	fetc.lindex = -1;
	fetc.tymed = TYMED_HGLOBAL;

	STGMEDIUM medium = { 0 };
	medium.hGlobal = GlobalAlloc(GHND, sizeof(DROPDESCRIPTION));
	if (medium.hGlobal == 0)
		return E_OUTOFMEMORY;

	auto pDropDescription = static_cast<DROPDESCRIPTION*>(GlobalLock(medium.hGlobal));
	if (!pDropDescription)
		return E_FAIL;
	StringCchCopy(pDropDescription->szInsert, _countof(pDropDescription->szInsert), insert);
	StringCchCopy(pDropDescription->szMessage, _countof(pDropDescription->szMessage), format);
	pDropDescription->type = image;
	GlobalUnlock(medium.hGlobal);
	return SetData(&fetc, &medium, TRUE);
}

void CGitEnumFormatEtc::Init(bool localonly, bool containsExistingFiles)
{
	int index = 0;
	m_formats[index].cfFormat = CF_UNICODETEXT;
	m_formats[index].dwAspect = DVASPECT_CONTENT;
	m_formats[index].lindex = -1;
	m_formats[index].ptd = nullptr;
	m_formats[index].tymed = TYMED_HGLOBAL;
	index++;

	m_formats[index].cfFormat = CF_TEXT;
	m_formats[index].dwAspect = DVASPECT_CONTENT;
	m_formats[index].lindex = -1;
	m_formats[index].ptd = nullptr;
	m_formats[index].tymed = TYMED_HGLOBAL;
	index++;

	m_formats[index].cfFormat = CF_PREFERREDDROPEFFECT;
	m_formats[index].dwAspect = DVASPECT_CONTENT;
	m_formats[index].lindex = -1;
	m_formats[index].ptd = nullptr;
	m_formats[index].tymed = TYMED_HGLOBAL;
	index++;

	if (containsExistingFiles && localonly)
	{
		m_formats[index].cfFormat = CF_INETURL;
		m_formats[index].dwAspect = DVASPECT_CONTENT;
		m_formats[index].lindex = -1;
		m_formats[index].ptd = nullptr;
		m_formats[index].tymed = TYMED_HGLOBAL;
		index++;

		m_formats[index].cfFormat = CF_SHELLURL;
		m_formats[index].dwAspect = DVASPECT_CONTENT;
		m_formats[index].lindex = -1;
		m_formats[index].ptd = nullptr;
		m_formats[index].tymed = TYMED_HGLOBAL;
		index++;
	}

	m_formats[index].cfFormat = CF_FILE_ATTRIBUTES_ARRAY;
	m_formats[index].dwAspect = DVASPECT_CONTENT;
	m_formats[index].lindex = -1;
	m_formats[index].ptd = nullptr;
	m_formats[index].tymed = TYMED_HGLOBAL;
	index++;

	if (containsExistingFiles && localonly)
	{
		m_formats[index].cfFormat = CF_HDROP;
		m_formats[index].dwAspect = DVASPECT_CONTENT;
		m_formats[index].lindex = -1;
		m_formats[index].ptd = nullptr;
		m_formats[index].tymed = TYMED_HGLOBAL;
		index++;
	}
	else if (containsExistingFiles)
	{
		m_formats[index].cfFormat = CF_FILECONTENTS;
		m_formats[index].dwAspect = DVASPECT_CONTENT;
		m_formats[index].lindex = -1;
		m_formats[index].ptd = nullptr;
		m_formats[index].tymed = TYMED_ISTREAM;
		index++;

		m_formats[index].cfFormat = CF_FILEDESCRIPTOR;
		m_formats[index].dwAspect = DVASPECT_CONTENT;
		m_formats[index].lindex = -1;
		m_formats[index].ptd = nullptr;
		m_formats[index].tymed = TYMED_HGLOBAL;
		index++;
	}
	// clear possible leftovers
	while (index < GITDATAOBJECT_NUMFORMATS)
	{
		m_formats[index].cfFormat = 0;
		m_formats[index].dwAspect = 0;
		m_formats[index].lindex = -1;
		m_formats[index].ptd = nullptr;
		m_formats[index].tymed = 0;
		index++;
	}
}

CGitEnumFormatEtc::CGitEnumFormatEtc(const std::vector<FORMATETC>& vec, bool localonly, bool containsExistingFiles)
	: m_cRefCount(0)
	, m_iCur(0)
	, m_localonly(localonly)
	, m_containsExistingFiles(containsExistingFiles)
{
	for (size_t i = 0; i < vec.size(); ++i)
		m_vecFormatEtc.push_back(vec[i]);
	Init(localonly, containsExistingFiles);
}

CGitEnumFormatEtc::CGitEnumFormatEtc(const std::vector<FORMATETC*>& vec, bool localonly, bool containsExistingFiles)
	: m_cRefCount(0)
	, m_iCur(0)
	, m_localonly(localonly)
	, m_containsExistingFiles(containsExistingFiles)
{
	for (size_t i = 0; i < vec.size(); ++i)
		m_vecFormatEtc.push_back(*vec[i]);
	Init(localonly, containsExistingFiles);
}

STDMETHODIMP  CGitEnumFormatEtc::QueryInterface(REFIID refiid, void** ppv)
{
	if (!ppv)
		return E_POINTER;
	*ppv = nullptr;
	if (IID_IUnknown == refiid || IID_IEnumFORMATETC == refiid)
		*ppv = static_cast<IEnumFORMATETC*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CGitEnumFormatEtc::AddRef(void)
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CGitEnumFormatEtc::Release(void)
{
	--m_cRefCount;
	if (m_cRefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_cRefCount;
}

STDMETHODIMP CGitEnumFormatEtc::Next(ULONG celt, LPFORMATETC lpFormatEtc, ULONG* pceltFetched)
{
	if (celt <= 0)
		return E_INVALIDARG;
	if (!pceltFetched && celt != 1) // pceltFetched can be NULL only for 1 item request
		return E_POINTER;
	if (!lpFormatEtc)
		return E_POINTER;

	if (pceltFetched)
		*pceltFetched = 0;

	if (m_iCur >= GITDATAOBJECT_NUMFORMATS)
		return S_FALSE;

	ULONG cReturn = celt;

	while (m_iCur < (GITDATAOBJECT_NUMFORMATS + m_vecFormatEtc.size()) && cReturn > 0)
	{
		if (m_iCur < GITDATAOBJECT_NUMFORMATS)
			*lpFormatEtc++ = m_formats[m_iCur++];
		else
			*lpFormatEtc++ = m_vecFormatEtc[m_iCur++ - GITDATAOBJECT_NUMFORMATS];
		--cReturn;
	}

	if (pceltFetched)
		*pceltFetched = celt - cReturn;

	return (cReturn == 0) ? S_OK : S_FALSE;
}

STDMETHODIMP CGitEnumFormatEtc::Skip(ULONG celt)
{
	if ((m_iCur + int(celt)) >= (GITDATAOBJECT_NUMFORMATS + m_vecFormatEtc.size()))
		return S_FALSE;
	m_iCur += celt;
	return S_OK;
}

STDMETHODIMP CGitEnumFormatEtc::Reset(void)
{
	m_iCur = 0;
	return S_OK;
}

STDMETHODIMP CGitEnumFormatEtc::Clone(IEnumFORMATETC** ppCloneEnumFormatEtc)
{
	if (!ppCloneEnumFormatEtc)
		return E_POINTER;

	CGitEnumFormatEtc *newEnum = new (std::nothrow) CGitEnumFormatEtc(m_vecFormatEtc, m_localonly, m_containsExistingFiles);
	if (!newEnum)
		return E_OUTOFMEMORY;

	newEnum->AddRef();
	newEnum->m_iCur = m_iCur;
	*ppCloneEnumFormatEtc = newEnum;
	return S_OK;
}
