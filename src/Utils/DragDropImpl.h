// IDataObjectImpl.h: interface for the CIDataObjectImpl class.
/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.
   Author: Leon Finker  1/2001
**************************************************************************/
#ifndef __DRAGDROPIMPL_H__
#define __DRAGDROPIMPL_H__
//#include <ShlDisp.h>
#include <shobjidl.h>
#include <shlobj.h>

///////////////////////////////////////////////////////////////////////////////////////////////
class CDragSourceNotify : IDropSourceNotify
{
private:
	LONG refCount;

public:

	CDragSourceNotify()
	{
		refCount = 0;
	}

	~CDragSourceNotify()
	{
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
	{
		if(!ppvObject)
			return E_POINTER;

		if (IsEqualIID(riid, IID_IUnknown))
			*ppvObject = static_cast<IUnknown*>(this);
		else if (IsEqualIID(riid, IID_IDropSourceNotify))
			*ppvObject = static_cast<IDropSourceNotify*>(this);
		else
		{
			*ppvObject = nullptr;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() override
	{
		return InterlockedIncrement(&refCount);
	}

	ULONG STDMETHODCALLTYPE Release() override
	{
		ULONG ret = InterlockedDecrement(&refCount);
		if(!ret) {
			delete this;
		}
		return ret;
	}

	HRESULT STDMETHODCALLTYPE DragEnterTarget(HWND /*hWndTarget*/) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DragLeaveTarget() override
	{
		return S_OK;
	}
};


///////////////////////////////////////////////////////////////////////////////////////////////
class CEnumFormatEtc : public IEnumFORMATETC
{
   private:
	 ULONG           m_cRefCount;
	 CSimpleArray<FORMATETC>  m_pFmtEtc;
	 int           m_iCur;

   public:
	 CEnumFormatEtc(const CSimpleArray<FORMATETC>& ArrFE);
	 CEnumFormatEtc(const CSimpleArray<FORMATETC*>& ArrFE);
	//IUnknown members
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;

	//IEnumFORMATETC members
	STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG*) override;
	STDMETHODIMP Skip(ULONG) override;
	STDMETHODIMP Reset() override;
	STDMETHODIMP Clone(IEnumFORMATETC**) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////
class CIDropSource : public IDropSource
{
	long m_cRefCount;
public:
	bool m_bDropped;
	IDataObject * m_pIDataObj;
	CDragSourceNotify* pDragSourceNotify;


	CIDropSource()
	: m_cRefCount(0)
	, m_bDropped(false)
	, m_pIDataObj(nullptr)
	{
		pDragSourceNotify = new CDragSourceNotify();
		pDragSourceNotify->AddRef();
	}
	~CIDropSource()
	{
		if (m_pIDataObj)
		{
			m_pIDataObj->Release();
			m_pIDataObj = nullptr;
		}
		if (pDragSourceNotify)
		{
			pDragSourceNotify->Release();
			pDragSourceNotify = nullptr;
		}
	}
	//IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;
	//IDropSource
	HRESULT STDMETHODCALLTYPE QueryContinueDrag(
		/* [in] */ BOOL fEscapePressed,
		/* [in] */ DWORD grfKeyState) override;

	HRESULT STDMETHODCALLTYPE GiveFeedback(
		/* [in] */ DWORD dwEffect) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////
class CIDataObject : public IDataObject//,public IAsyncOperation
{
	CIDropSource* m_pDropSource;
	long m_cRefCount;
	CSimpleArray<FORMATETC*> m_ArrFormatEtc;
	CSimpleArray<STGMEDIUM*> m_StgMedium;
public:
	CIDataObject(CIDropSource* pDropSource);
	~CIDataObject();
	void CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc);
	//IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;

	//IDataObject
	/* [local] */ HRESULT STDMETHODCALLTYPE GetData(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
		/* [out] */ STGMEDIUM __RPC_FAR *pmedium) override;

	/* [local] */ HRESULT STDMETHODCALLTYPE GetDataHere(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
		/* [out][in] */ STGMEDIUM __RPC_FAR *pmedium) override;

	HRESULT STDMETHODCALLTYPE QueryGetData(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc) override;

	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
		/* [out] */ FORMATETC __RPC_FAR *pformatetcOut) override;

	/* [local] */ HRESULT STDMETHODCALLTYPE SetData(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
		/* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
		/* [in] */ BOOL fRelease) override;

	HRESULT STDMETHODCALLTYPE EnumFormatEtc(
		/* [in] */ DWORD dwDirection,
		/* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc) override;

	HRESULT STDMETHODCALLTYPE DAdvise(
		/* [in] */ FORMATETC __RPC_FAR *pformatetc,
		/* [in] */ DWORD advf,
		/* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
		/* [out] */ DWORD __RPC_FAR *pdwConnection) override;

	HRESULT STDMETHODCALLTYPE DUnadvise(
		/* [in] */ DWORD dwConnection) override;

	HRESULT STDMETHODCALLTYPE EnumDAdvise(
		/* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise) override;

	//IAsyncOperation
	//virtual HRESULT STDMETHODCALLTYPE SetAsyncMode(
	//    /* [in] */ BOOL fDoOpAsync)
	//{
	//	return E_NOTIMPL;
	//}
	//
	//virtual HRESULT STDMETHODCALLTYPE GetAsyncMode(
	//    /* [out] */ BOOL __RPC_FAR *pfIsOpAsync)
	//{
	//	return E_NOTIMPL;
	//}
	//
	//virtual HRESULT STDMETHODCALLTYPE StartOperation(
	//    /* [optional][unique][in] */ IBindCtx __RPC_FAR *pbcReserved)
	//{
	//	return E_NOTIMPL;
	//}
	//
	//virtual HRESULT STDMETHODCALLTYPE InOperation(
	//    /* [out] */ BOOL __RPC_FAR *pfInAsyncOp)
	//{
	//	return E_NOTIMPL;
	//}
	//
	//virtual HRESULT STDMETHODCALLTYPE EndOperation(
	//    /* [in] */ HRESULT hResult,
	//    /* [unique][in] */ IBindCtx __RPC_FAR *pbcReserved,
	//    /* [in] */ DWORD dwEffects)
	//{
	//	return E_NOTIMPL;
	//}


	// helper function
	HRESULT SetDropDescription(DROPIMAGETYPE image, LPCWSTR format, LPCWSTR insert);
};

///////////////////////////////////////////////////////////////////////////////////////////////
class CIDropTarget : public IDropTarget
{
	DWORD m_cRefCount;
	bool m_bAllowDrop;
	IDropTargetHelper *m_pDropTargetHelper;
	CSimpleArray<FORMATETC> m_formatetc;
	FORMATETC* m_pSupportedFrmt;
protected:
	HWND m_hTargetWnd;
	IDataObject *           m_pIDataObject;
public:

	CIDropTarget(HWND m_hTargetWnd);
	virtual ~CIDropTarget();
	void AddSuportedFormat(FORMATETC& ftetc) { m_formatetc.Add(ftetc); }

	//return values: true - release the medium. false - don't release the medium
	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium,DWORD *pdwEffect, POINTL pt) = 0;

	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override { ATLTRACE("CIDropTarget::AddRef\n"); return ++m_cRefCount; }
	ULONG STDMETHODCALLTYPE Release() override;

	bool QueryDrop(DWORD grfKeyState, LPDWORD pdwEffect);
	HRESULT STDMETHODCALLTYPE DragEnter(
		/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect) override;
	HRESULT STDMETHODCALLTYPE DragOver(
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect) override;
	HRESULT STDMETHODCALLTYPE DragLeave() override;
	HRESULT STDMETHODCALLTYPE Drop(
		/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect) override;

	// helper function
	HRESULT SetDropDescription(DROPIMAGETYPE image, LPCWSTR format, LPCWSTR insert);
};

class CDragSourceHelper
{
	IDragSourceHelper2* pDragSourceHelper2;
	IDragSourceHelper* pDragSourceHelper;

public:
	CDragSourceHelper()
	{
		pDragSourceHelper = nullptr;
		pDragSourceHelper2 = nullptr;
		if(FAILED(CoCreateInstance(CLSID_DragDropHelper,
						nullptr,
						CLSCTX_INPROC_SERVER,
						IID_IDragSourceHelper2,
						reinterpret_cast<void**>(&pDragSourceHelper2))))
		{
			pDragSourceHelper2 = nullptr;
			if(FAILED(CoCreateInstance(CLSID_DragDropHelper,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_IDragSourceHelper,
				reinterpret_cast<void**>(&pDragSourceHelper))))
				pDragSourceHelper = nullptr;
		}
	}
	virtual ~CDragSourceHelper()
	{
		if (pDragSourceHelper2)
		{
			pDragSourceHelper2->Release();
			pDragSourceHelper2 = nullptr;
		}
		if (pDragSourceHelper)
		{
			pDragSourceHelper->Release();
			pDragSourceHelper = nullptr;
		}
	}

	// IDragSourceHelper
	HRESULT InitializeFromBitmap(HBITMAP hBitmap,
		const POINT& pt,  // cursor position in client coords of the window
		const RECT& rc,   // selected item's bounding rect
		IDataObject* pDataObject,
		BOOL allowDropDescription=TRUE,
		COLORREF crColorKey=GetSysColor(COLOR_WINDOW)// color of the window used for transparent effect.
		)
	{
		if (!pDragSourceHelper && !pDragSourceHelper2)
			return E_FAIL;

		if ((allowDropDescription)&&(pDragSourceHelper2))
			pDragSourceHelper2->SetFlags(DSH_ALLOWDROPDESCRIPTIONTEXT);

		SHDRAGIMAGE di;
		BITMAP      bm;
		GetObject(hBitmap, sizeof(bm), &bm);
		di.sizeDragImage.cx = bm.bmWidth;
		di.sizeDragImage.cy = bm.bmHeight;
		di.hbmpDragImage = hBitmap;
		di.crColorKey = crColorKey;
		di.ptOffset.x = pt.x - rc.left;
		di.ptOffset.y = pt.y - rc.top;
		if (pDragSourceHelper2)
			return pDragSourceHelper2->InitializeFromBitmap(&di, pDataObject);
		if (!pDragSourceHelper)
			return E_FAIL;
		return pDragSourceHelper->InitializeFromBitmap(&di, pDataObject);
	}
	HRESULT InitializeFromWindow(HWND hwnd, POINT& pt, IDataObject* pDataObject, BOOL allowDropDescription=TRUE)
	{
		if (!pDragSourceHelper && !pDragSourceHelper2)
			return E_FAIL;
		if ((allowDropDescription)&&(pDragSourceHelper2))
			pDragSourceHelper2->SetFlags(DSH_ALLOWDROPDESCRIPTIONTEXT);
		if (pDragSourceHelper2)
			return pDragSourceHelper2->InitializeFromWindow(hwnd, &pt, pDataObject);
		if (!pDragSourceHelper)
			return E_FAIL;
		return pDragSourceHelper->InitializeFromWindow(hwnd, &pt, pDataObject);
	}
};
#endif //__DRAGDROPIMPL_H__
