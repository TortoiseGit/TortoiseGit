/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.
   Author: Leon Finker  1/2001
**************************************************************************/
// IDataObjectImpl.cpp: implementation of the CIDataObjectImpl class.
//////////////////////////////////////////////////////////////////////
#include <stdafx.h>
#include <shlobj.h>
#include <atlbase.h>
#include "DragDropImpl.h"

//////////////////////////////////////////////////////////////////////
// CIDataObject Class
//////////////////////////////////////////////////////////////////////

CIDataObject::CIDataObject(CIDropSource* pDropSource):
m_cRefCount(0), m_pDropSource(pDropSource)
{
}

CIDataObject::~CIDataObject()
{
	for(int i = 0; i < m_StgMedium.GetSize(); ++i)
	{
		ReleaseStgMedium(m_StgMedium[i]);
		delete m_StgMedium[i];
	}
	for(int j = 0; j < m_ArrFormatEtc.GetSize(); ++j)
		delete m_ArrFormatEtc[j];
}

STDMETHODIMP CIDataObject::QueryInterface(/* [in] */ REFIID riid,
/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	*ppvObject = NULL;
	if (IID_IUnknown==riid || IID_IDataObject==riid)
		*ppvObject=this;
	/*if(riid == IID_IAsyncOperation)
		*ppvObject=(IAsyncOperation*)this;*/
	if (NULL!=*ppvObject)
	{
		((LPUNKNOWN)*ppvObject)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CIDataObject::AddRef( void)
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CIDataObject::Release( void)
{
	long nTemp;
	nTemp = --m_cRefCount;
	if(nTemp==0)
		delete this;
	return nTemp;
}

STDMETHODIMP CIDataObject::GetData(
	/* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
	/* [out] */ STGMEDIUM __RPC_FAR *pmedium)
{
	if(pformatetcIn == NULL || pmedium == NULL)
		return E_INVALIDARG;
	pmedium->hGlobal = NULL;

	ATLASSERT(m_StgMedium.GetSize() == m_ArrFormatEtc.GetSize());
	for(int i=0; i < m_ArrFormatEtc.GetSize(); ++i)
	{
		if(pformatetcIn->tymed & m_ArrFormatEtc[i]->tymed &&
			pformatetcIn->dwAspect == m_ArrFormatEtc[i]->dwAspect &&
			pformatetcIn->cfFormat == m_ArrFormatEtc[i]->cfFormat)
		{
			CopyMedium(pmedium, m_StgMedium[i], m_ArrFormatEtc[i]);
			return S_OK;
		}
	}
	return DV_E_FORMATETC;
}

STDMETHODIMP CIDataObject::GetDataHere(
	/* [unique][in] */ FORMATETC __RPC_FAR * /*pformatetc*/,
	/* [out][in] */ STGMEDIUM __RPC_FAR * /*pmedium*/)
{
	return E_NOTIMPL;
}

STDMETHODIMP CIDataObject::QueryGetData(
	/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc)
{
	if(pformatetc == NULL)
		return E_INVALIDARG;

	//support others if needed DVASPECT_THUMBNAIL  //DVASPECT_ICON   //DVASPECT_DOCPRINT
	if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
		return (DV_E_DVASPECT);
	HRESULT  hr = DV_E_TYMED;
	for(int i = 0; i < m_ArrFormatEtc.GetSize(); ++i)
	{
		if(pformatetc->tymed & m_ArrFormatEtc[i]->tymed)
		{
			if(pformatetc->cfFormat == m_ArrFormatEtc[i]->cfFormat)
				return S_OK;
			else
				hr = DV_E_CLIPFORMAT;
		}
		else
			hr = DV_E_TYMED;
	}
	return hr;
}

STDMETHODIMP CIDataObject::GetCanonicalFormatEtc(
	/* [unique][in] */ FORMATETC __RPC_FAR * /*pformatectIn*/,
	/* [out] */ FORMATETC __RPC_FAR *pformatetcOut)
{
	if (pformatetcOut == NULL)
		return E_INVALIDARG;
	return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP CIDataObject::SetData(
	/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
	/* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
	/* [in] */ BOOL fRelease)
{
	if(pformatetc == NULL || pmedium == NULL)
		return E_INVALIDARG;

	ATLASSERT(pformatetc->tymed == pmedium->tymed);
	FORMATETC* fetc=new FORMATETC;
	STGMEDIUM* pStgMed = new STGMEDIUM;

	if(fetc == NULL || pStgMed == NULL)
		return E_OUTOFMEMORY;

	SecureZeroMemory(fetc,sizeof(FORMATETC));
	SecureZeroMemory(pStgMed,sizeof(STGMEDIUM));

	*fetc = *pformatetc;
	m_ArrFormatEtc.Add(fetc);

	if(fRelease)
		*pStgMed = *pmedium;
	else
	{
		CopyMedium(pStgMed, pmedium, pformatetc);
	}
	m_StgMedium.Add(pStgMed);

	return S_OK;
}
void CIDataObject::CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
{
		switch(pMedSrc->tymed)
		{
		case TYMED_HGLOBAL:
			pMedDest->hGlobal = (HGLOBAL)OleDuplicateData(pMedSrc->hGlobal,pFmtSrc->cfFormat, NULL);
			break;
		case TYMED_GDI:
			pMedDest->hBitmap = (HBITMAP)OleDuplicateData(pMedSrc->hBitmap,pFmtSrc->cfFormat, NULL);
			break;
		case TYMED_MFPICT:
			pMedDest->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(pMedSrc->hMetaFilePict,pFmtSrc->cfFormat, NULL);
			break;
		case TYMED_ENHMF:
			pMedDest->hEnhMetaFile = (HENHMETAFILE)OleDuplicateData(pMedSrc->hEnhMetaFile,pFmtSrc->cfFormat, NULL);
			break;
		case TYMED_FILE:
			pMedSrc->lpszFileName = (LPOLESTR)OleDuplicateData(pMedSrc->lpszFileName,pFmtSrc->cfFormat, NULL);
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
		pMedDest->pUnkForRelease = NULL;
		if(pMedSrc->pUnkForRelease != NULL)
		{
			pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
			pMedSrc->pUnkForRelease->AddRef();
		}
}
STDMETHODIMP CIDataObject::EnumFormatEtc(
	/* [in] */ DWORD dwDirection,
	/* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc)
{
	if(ppenumFormatEtc == NULL)
		return E_POINTER;

	*ppenumFormatEtc=NULL;
	switch (dwDirection)
	{
		case DATADIR_GET:
			*ppenumFormatEtc= new CEnumFormatEtc(m_ArrFormatEtc);
			if(*ppenumFormatEtc == NULL)
				return E_OUTOFMEMORY;
			(*ppenumFormatEtc)->AddRef();
			break;

		case DATADIR_SET:
		default:
			return E_NOTIMPL;
			break;
	}

	return S_OK;
}

STDMETHODIMP CIDataObject::DAdvise(
	/* [in] */ FORMATETC __RPC_FAR * /*pformatetc*/,
	/* [in] */ DWORD /*advf*/,
	/* [unique][in] */ IAdviseSink __RPC_FAR * /*pAdvSink*/,
	/* [out] */ DWORD __RPC_FAR * /*pdwConnection*/)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CIDataObject::DUnadvise(
	/* [in] */ DWORD /*dwConnection*/)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CIDataObject::EnumDAdvise(
	/* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR * /*ppenumAdvise*/)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////
// CIDropSource Class
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CIDropSource::QueryInterface(/* [in] */ REFIID riid,
											/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	*ppvObject = NULL;
	if (IID_IUnknown==riid || IID_IDropSource==riid)
		*ppvObject=this;

	if (*ppvObject != NULL)
	{
		((LPUNKNOWN)*ppvObject)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CIDropSource::AddRef( void)
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CIDropSource::Release( void)
{
	long nTemp;
	nTemp = --m_cRefCount;
	ATLASSERT(nTemp >= 0);
	if(nTemp==0)
		delete this;
	return nTemp;
}

STDMETHODIMP CIDropSource::QueryContinueDrag(
	/* [in] */ BOOL fEscapePressed,
	/* [in] */ DWORD grfKeyState)
{
	if(fEscapePressed)
		return DRAGDROP_S_CANCEL;
	if(!(grfKeyState & (MK_LBUTTON|MK_RBUTTON)))
	{
		m_bDropped = true;
		return DRAGDROP_S_DROP;
	}

	return S_OK;

}

STDMETHODIMP CIDropSource::GiveFeedback(
	/* [in] */ DWORD /*dwEffect*/)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

//////////////////////////////////////////////////////////////////////
// CEnumFormatEtc Class
//////////////////////////////////////////////////////////////////////

CEnumFormatEtc::CEnumFormatEtc(const CSimpleArray<FORMATETC>& ArrFE):
m_cRefCount(0),m_iCur(0)
{
	ATLTRACE("CEnumFormatEtc::CEnumFormatEtc()\n");
	for(int i = 0; i < ArrFE.GetSize(); ++i)
		m_pFmtEtc.Add(ArrFE[i]);
}

CEnumFormatEtc::CEnumFormatEtc(const CSimpleArray<FORMATETC*>& ArrFE):
m_cRefCount(0),m_iCur(0)
{
	for(int i = 0; i < ArrFE.GetSize(); ++i)
		m_pFmtEtc.Add(*ArrFE[i]);
}

STDMETHODIMP CEnumFormatEtc::QueryInterface(REFIID refiid, void FAR* FAR* ppv)
{
	*ppv = NULL;
	if (IID_IUnknown==refiid || IID_IEnumFORMATETC==refiid)
			*ppv=this;

	if (*ppv != NULL)
	{
		((LPUNKNOWN)*ppv)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef(void)
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release(void)
{
	long nTemp = --m_cRefCount;
	ATLASSERT(nTemp >= 0);
	if(nTemp == 0)
		delete this;

	return nTemp;
}

STDMETHODIMP CEnumFormatEtc::Next( ULONG celt,LPFORMATETC lpFormatEtc, ULONG FAR *pceltFetched)
{
	if(pceltFetched != NULL)
		*pceltFetched=0;

	ULONG cReturn = celt;

	if(celt <= 0 || lpFormatEtc == NULL || m_iCur >= m_pFmtEtc.GetSize())
		return S_FALSE;

	if(pceltFetched == NULL && celt != 1) // pceltFetched can be NULL only for 1 item request
		return S_FALSE;

	while (m_iCur < m_pFmtEtc.GetSize() && cReturn > 0)
	{
		*lpFormatEtc++ = m_pFmtEtc[m_iCur++];
		--cReturn;
	}
	if (pceltFetched != NULL)
		*pceltFetched = celt - cReturn;

	return (cReturn == 0) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumFormatEtc::Skip(ULONG celt)
{
	if((m_iCur + int(celt)) >= m_pFmtEtc.GetSize())
		return S_FALSE;
	m_iCur += celt;
	return S_OK;
}

STDMETHODIMP CEnumFormatEtc::Reset(void)
{
	m_iCur = 0;
	return S_OK;
}

STDMETHODIMP CEnumFormatEtc::Clone(IEnumFORMATETC FAR * FAR*ppCloneEnumFormatEtc)
{
	if(ppCloneEnumFormatEtc == NULL)
		return E_POINTER;

	CEnumFormatEtc *newEnum = new CEnumFormatEtc(m_pFmtEtc);
	if(newEnum ==NULL)
		return E_OUTOFMEMORY;
	newEnum->AddRef();
	newEnum->m_iCur = m_iCur;
	*ppCloneEnumFormatEtc = newEnum;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CIDropTarget Class
//////////////////////////////////////////////////////////////////////
CIDropTarget::CIDropTarget(HWND hTargetWnd):
	m_hTargetWnd(hTargetWnd),
	m_cRefCount(0), m_bAllowDrop(false),
	m_pDropTargetHelper(NULL), m_pSupportedFrmt(NULL)
{
	if(FAILED(CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
					IID_IDropTargetHelper,(LPVOID*)&m_pDropTargetHelper)))
		m_pDropTargetHelper = NULL;
}

CIDropTarget::~CIDropTarget()
{
	if(m_pDropTargetHelper != NULL)
	{
		m_pDropTargetHelper->Release();
		m_pDropTargetHelper = NULL;
	}
}

HRESULT STDMETHODCALLTYPE CIDropTarget::QueryInterface( /* [in] */ REFIID riid,
						/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	*ppvObject = NULL;
	if (IID_IUnknown==riid || IID_IDropTarget==riid)
		*ppvObject=this;

	if (*ppvObject != NULL)
	{
		((LPUNKNOWN)*ppvObject)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CIDropTarget::Release( void)
{
	long nTemp;
	nTemp = --m_cRefCount;
	ATLASSERT(nTemp >= 0);
	if(nTemp==0)
		delete this;
	return nTemp;
}

bool CIDropTarget::QueryDrop(DWORD grfKeyState, LPDWORD pdwEffect)
{
	DWORD dwOKEffects = *pdwEffect;

	if(!m_bAllowDrop)
	{
		*pdwEffect = DROPEFFECT_NONE;
		return false;
	}
	//CTRL+SHIFT  -- DROPEFFECT_LINK
	//CTRL        -- DROPEFFECT_COPY
	//SHIFT       -- DROPEFFECT_MOVE
	//no modifier -- DROPEFFECT_MOVE or whatever is allowed by src
	*pdwEffect = (grfKeyState & MK_CONTROL) ?
				( (grfKeyState & MK_SHIFT) ? DROPEFFECT_LINK : DROPEFFECT_COPY ):
				( (grfKeyState & MK_SHIFT) ? DROPEFFECT_MOVE : 0 );
	if(*pdwEffect == 0)
	{
		// No modifier keys used by user while dragging.
		if (DROPEFFECT_MOVE & dwOKEffects)
			*pdwEffect = DROPEFFECT_MOVE;
		else if (DROPEFFECT_COPY & dwOKEffects)
			*pdwEffect = DROPEFFECT_COPY;
		else if (DROPEFFECT_LINK & dwOKEffects)
			*pdwEffect = DROPEFFECT_LINK;
		else
		{
			*pdwEffect = DROPEFFECT_NONE;
		}
	}
	else
	{
		// Check if the drag source application allows the drop effect desired by user.
		// The drag source specifies this in DoDragDrop
		if(!(*pdwEffect & dwOKEffects))
			*pdwEffect = DROPEFFECT_NONE;
	}

	return (DROPEFFECT_NONE == *pdwEffect)?false:true;
}

HRESULT STDMETHODCALLTYPE CIDropTarget::DragEnter(
	/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
	/* [in] */ DWORD grfKeyState,
	/* [in] */ POINTL pt,
	/* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
	if(pDataObj == NULL)
		return E_INVALIDARG;

	if(m_pDropTargetHelper)
		m_pDropTargetHelper->DragEnter(m_hTargetWnd, pDataObj, (LPPOINT)&pt, *pdwEffect);
	//IEnumFORMATETC* pEnum;
	//pDataObj->EnumFormatEtc(DATADIR_GET,&pEnum);
	//FORMATETC ftm;
	//for()
	//pEnum->Next(1,&ftm,0);
	//pEnum->Release();
	m_pSupportedFrmt = NULL;
	for(int i =0; i<m_formatetc.GetSize(); ++i)
	{
		m_bAllowDrop = (pDataObj->QueryGetData(&m_formatetc[i]) == S_OK)?true:false;
		if(m_bAllowDrop)
		{
			m_pSupportedFrmt = &m_formatetc[i];
			break;
		}
	}

	QueryDrop(grfKeyState, pdwEffect);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CIDropTarget::DragOver(
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
	if(m_pDropTargetHelper)
		m_pDropTargetHelper->DragOver((LPPOINT)&pt, *pdwEffect);
	QueryDrop(grfKeyState, pdwEffect);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CIDropTarget::DragLeave( void)
{
	if(m_pDropTargetHelper)
		m_pDropTargetHelper->DragLeave();

	m_bAllowDrop = false;
	m_pSupportedFrmt = NULL;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CIDropTarget::Drop(
	/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
	/* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt,
	/* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
	if (pDataObj == NULL)
		return E_INVALIDARG;

	if(m_pDropTargetHelper)
		m_pDropTargetHelper->Drop(pDataObj, (LPPOINT)&pt, *pdwEffect);

	if(QueryDrop(grfKeyState, pdwEffect))
	{
		if(m_bAllowDrop && m_pSupportedFrmt != NULL)
		{
			STGMEDIUM medium;
			if(pDataObj->GetData(m_pSupportedFrmt, &medium) == S_OK)
			{
				if(OnDrop(m_pSupportedFrmt, medium, pdwEffect, pt)) //does derive class wants us to free medium?
					ReleaseStgMedium(&medium);
			}
		}
	}
	m_bAllowDrop=false;
	*pdwEffect = DROPEFFECT_NONE;
	m_pSupportedFrmt = NULL;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CIDragSourceHelper Class
//////////////////////////////////////////////////////////////////////
