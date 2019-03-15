// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2017, 2019 - TortoiseGit
// Copyright (C) 2017 - TortoiseSVN

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
#include "NativeRibbonApp.h"

CNativeRibbonApp::CNativeRibbonApp(CFrameWnd* pFrame, IUIFramework* pFramework)
	: m_pFrame(pFrame)
	, m_pFramework(pFramework)
	, m_cRefCount(0)
{
}

CNativeRibbonApp::~CNativeRibbonApp()
{
	ASSERT(m_cRefCount == 0);
}

STDMETHODIMP CNativeRibbonApp::QueryInterface(REFIID riid, void **ppvObject)
{
	if (riid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = static_cast<IUICommandHandler*>(this);
		return S_OK;
	}
	else if (riid == __uuidof(IUIApplication))
	{
		AddRef();
		*ppvObject = static_cast<IUIApplication*>(this);
		return S_OK;
	}
	else if (riid == __uuidof(IUICommandHandler))
	{
		AddRef();
		*ppvObject = static_cast<IUICommandHandler*>(this);
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_(ULONG) CNativeRibbonApp::AddRef(void)
{
	return InterlockedIncrement(&m_cRefCount);
}

STDMETHODIMP_(ULONG) CNativeRibbonApp::Release(void)
{
	return InterlockedDecrement(&m_cRefCount);
}

STDMETHODIMP CNativeRibbonApp::OnViewChanged(UINT32 /*viewId*/, UI_VIEWTYPE typeID, IUnknown* view, UI_VIEWVERB verb, INT32 /*uReasonCode*/)
{
	if (typeID == UI_VIEWTYPE_RIBBON)
	{
		if (verb == UI_VIEWVERB_CREATE)
		{
			if (!m_SettingsFileName.IsEmpty())
			{
				CComQIPtr<IUIRibbon> ribbonView(view);
				if (ribbonView)
					LoadRibbonViewSettings(ribbonView, m_SettingsFileName);
			}

			m_pFrame->RecalcLayout();
		}
		else if (verb == UI_VIEWVERB_DESTROY)
		{
			CComQIPtr<IUIRibbon> ribbonView(view);
			if (ribbonView)
				SaveRibbonViewSettings(ribbonView, m_SettingsFileName);
		}
		else if (verb == UI_VIEWVERB_SIZE)
			m_pFrame->RecalcLayout();
	}

	return S_OK;
}

STDMETHODIMP CNativeRibbonApp::OnCreateUICommand(UINT32 commandId, UI_COMMANDTYPE typeID, IUICommandHandler** commandHandler)
{
	m_commandIds.push_back(commandId);
	if (typeID == UI_COMMANDTYPE_COLLECTION)
		m_collectionCommandIds.push_back(commandId);

	return QueryInterface(IID_PPV_ARGS(commandHandler));
}

STDMETHODIMP CNativeRibbonApp::OnDestroyUICommand(UINT32 commandId, UI_COMMANDTYPE /*typeID*/, IUICommandHandler* /*commandHandler*/)
{
	m_commandIds.remove(commandId);

	return S_OK;
}

STDMETHODIMP CNativeRibbonApp::Execute(UINT32 commandId, UI_EXECUTIONVERB verb, const PROPERTYKEY* key, const PROPVARIANT* currentValue, IUISimplePropertySet* /*commandExecutionProperties*/)
{
	if (verb == UI_EXECUTIONVERB_EXECUTE)
	{
		if (key && IsEqualPropertyKey(*key, UI_PKEY_SelectedItem))
		{
			CComPtr<IUICollection> items = GetUICommandItemsSource(commandId);
			UINT32 selectedItemIdx = 0;
			UIPropertyToUInt32(UI_PKEY_SelectedItem, *currentValue, &selectedItemIdx);
			UINT32 count = 0;
			items->GetCount(&count);

			if (selectedItemIdx < count)
			{
				CComPtr<IUnknown> selectedItemUnk;
				items->GetItem(selectedItemIdx, &selectedItemUnk);
				CComQIPtr<IUISimplePropertySet> selectedItemPropSet(selectedItemUnk);
				if (selectedItemPropSet)
					m_pFrame->PostMessage(WM_COMMAND, GetCommandIdProperty(selectedItemPropSet));
			}
		}
		else
			m_pFrame->PostMessage(WM_COMMAND, commandId);

		return S_OK;
	}

	return S_FALSE;
}

class CRibbonCmdUI : public CCmdUI
{
public:
	CString m_Text;
	BOOL m_bOn;
	int m_nCheck;
	int m_bCheckChanged;

	CRibbonCmdUI(int commandId)
		: m_bOn(FALSE)
		, m_nCheck(0)
		, m_bCheckChanged(FALSE)
	{
		m_nID = commandId;
	}

	virtual void Enable(BOOL bOn)
	{
		m_bOn = bOn;
		m_bEnableChanged = TRUE;
	}

	virtual void SetCheck(int nCheck)
	{
		m_nCheck = nCheck;
		m_bCheckChanged = TRUE;
	}

	virtual void SetText(LPCTSTR lpszText)
	{
		m_Text = lpszText;
	}
};

STDMETHODIMP CNativeRibbonApp::UpdateProperty(UINT32 commandId, REFPROPERTYKEY key, const PROPVARIANT* /*currentValue*/, PROPVARIANT* newValue)
{
	if (key == UI_PKEY_TooltipTitle)
	{
		CString str;
		if (!str.LoadString(commandId))
			return S_FALSE;

		int nIndex = str.Find(L'\n');
		if (nIndex <= 0)
			return S_FALSE;

		str = str.Mid(nIndex + 1);

		CString strLabel;

		if (m_pFrame != NULL && (CKeyboardManager::FindDefaultAccelerator(commandId, strLabel, m_pFrame, TRUE) ||
			CKeyboardManager::FindDefaultAccelerator(commandId, strLabel, m_pFrame->GetActiveFrame(), FALSE)))
		{
			str += _T(" (");
			str += strLabel;
			str += _T(')');
		}

		return UIInitPropertyFromString(UI_PKEY_TooltipTitle, str, newValue);
	}
	else if (key == UI_PKEY_TooltipDescription)
	{
		CString str;
		if (!str.LoadString(commandId))
			return S_FALSE;

		int nIndex = str.Find(L'\n');
		if (nIndex <= 0)
			return S_FALSE;

		str = str.Left(nIndex);

		return UIInitPropertyFromString(UI_PKEY_TooltipDescription, str, newValue);
	}
	else if (key == UI_PKEY_Enabled)
	{
		CRibbonCmdUI ui(commandId);
		ui.DoUpdate(m_pFrame, TRUE);

		return UIInitPropertyFromBoolean(UI_PKEY_Enabled, ui.m_bOn, newValue);
	}
	else if (key == UI_PKEY_BooleanValue)
	{
		CRibbonCmdUI ui(commandId);
		ui.DoUpdate(m_pFrame, TRUE);

		return UIInitPropertyFromBoolean(UI_PKEY_BooleanValue, ui.m_nCheck, newValue);
	}
	else if (key == UI_PKEY_SelectedItem)
	{
		CComPtr<IUICollection> items = GetUICommandItemsSource(commandId);
		if (!items)
			return E_FAIL;

		UINT32 count;
		items->GetCount(&count);
		for (UINT32 idx = 0; idx < count; idx++)
		{
			CComPtr<IUnknown> item;
			items->GetItem(idx, &item);

			CComQIPtr<IUISimplePropertySet> simplePropertySet(item);
			if (simplePropertySet)
			{
				PROPVARIANT var = { 0 };
				UINT32 uiVal;
				simplePropertySet->GetValue(UI_PKEY_CommandId, &var);
				UIPropertyToUInt32(UI_PKEY_CommandId, var, &uiVal);

				CRibbonCmdUI ui(uiVal);
				ui.DoUpdate(m_pFrame, TRUE);

				if (ui.m_nCheck)
				{
					UIInitPropertyFromUInt32(UI_PKEY_SelectedItem, idx, newValue);
					return S_OK;
				}
			}
		}

		// No selected item.
		UIInitPropertyFromUInt32(UI_PKEY_SelectedItem, static_cast<UINT>(-1), newValue);

		return S_OK;
	}

	return E_NOTIMPL;
}

HRESULT CNativeRibbonApp::SaveRibbonViewSettings(IUIRibbon* pRibbonView, const CString& fileName)
{
	CComPtr<IStream> stream;
	HRESULT hr = SHCreateStreamOnFileEx(fileName, STGM_WRITE | STGM_CREATE, FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &stream);
	if (FAILED(hr))
		return hr;

	hr = pRibbonView->SaveSettingsToStream(stream);
	if (FAILED(hr))
	{
		stream->Revert();
		return hr;
	}

	hr = stream->Commit(STGC_DEFAULT);

	return hr;
}

HRESULT CNativeRibbonApp::LoadRibbonViewSettings(IUIRibbon* pRibbonView, const CString& fileName)
{
	CComPtr<IStream> stream;
	HRESULT hr = SHCreateStreamOnFileEx(fileName, STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream);
	if (FAILED(hr))
		return hr;

	hr = pRibbonView->LoadSettingsFromStream(stream);

	return hr;
}

CComPtr<IUICollection> CNativeRibbonApp::GetUICommandItemsSource(UINT commandId)
{
	PROPVARIANT prop = { 0 };

	m_pFramework->GetUICommandProperty(commandId, UI_PKEY_ItemsSource, &prop);

	CComPtr<IUICollection> uiCollection;
	UIPropertyToInterface(UI_PKEY_ItemsSource, prop, &uiCollection);
	PropVariantClear(&prop);

	return uiCollection;
}

void CNativeRibbonApp::SetUICommandItemsSource(UINT commandId, IUICollection* pItems)
{
	PROPVARIANT prop = { 0 };

	UIInitPropertyFromInterface(UI_PKEY_ItemsSource, pItems, &prop);

	m_pFramework->SetUICommandProperty(commandId, UI_PKEY_ItemsSource, prop);

	PropVariantClear(&prop);
}

UINT CNativeRibbonApp::GetCommandIdProperty(IUISimplePropertySet* propertySet)
{
	PROPVARIANT var = { 0 };
	UINT32 commandId = 0;

	if (propertySet->GetValue(UI_PKEY_CommandId, &var) == S_OK)
		UIPropertyToUInt32(UI_PKEY_CommandId, var, &commandId);

	return commandId;
}

void CNativeRibbonApp::UpdateCmdUI(BOOL bDisableIfNoHandler)
{
	for (auto it = m_commandIds.begin(); it != m_commandIds.end(); ++it)
	{
		CRibbonCmdUI ui(*it);
		ui.DoUpdate(m_pFrame, bDisableIfNoHandler);
		if (ui.m_bEnableChanged)
		{
			PROPVARIANT val = { 0 };
			UIInitPropertyFromBoolean(UI_PKEY_Enabled, ui.m_bOn, &val);
			m_pFramework->SetUICommandProperty(*it, UI_PKEY_Enabled, val);
		}

		if (ui.m_bCheckChanged)
		{
			PROPVARIANT val = { 0 };
			UIInitPropertyFromBoolean(UI_PKEY_BooleanValue, ui.m_nCheck, &val);
			m_pFramework->SetUICommandProperty(*it, UI_PKEY_BooleanValue, val);
		}
	}

	for (auto it = m_collectionCommandIds.begin(); it != m_collectionCommandIds.end(); ++it)
	{
		PROPVARIANT currentValue = { 0 };
		PROPVARIANT newValue = { 0 };

		UpdateProperty(*it, UI_PKEY_SelectedItem, &currentValue, &newValue);
		m_pFramework->SetUICommandProperty(*it, UI_PKEY_SelectedItem, newValue);
	}
}

int CNativeRibbonApp::GetRibbonHeight()
{
	CComPtr<IUIRibbon> pRibbon;
	if (SUCCEEDED(m_pFramework->GetView(0, IID_PPV_ARGS(&pRibbon))))
	{
		UINT32 cy = 0;
		pRibbon->GetHeight(&cy);
		return static_cast<int>(cy);
	}

	return 0;
}

class UIDynamicCommandItem : public IUISimplePropertySet
{
private:
	ULONG m_RefCount;
	CString m_Label;
	int m_CommandId;
	CComPtr<IUIImage> m_Image;

public:
	class UIDynamicCommandItem(int commandId, LPCWSTR label, IUIImage* image)
		: m_RefCount(0)
		, m_Label(label)
		, m_CommandId(commandId)
		, m_Image(image)
	{
	}

	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
	{
		if (riid == __uuidof(IUnknown))
		{
			AddRef();
			*ppvObject = static_cast<IUnknown*>(this);
			return S_OK;
		}
		else if (riid == __uuidof(IUISimplePropertySet))
		{
			AddRef();
			*ppvObject = static_cast<IUISimplePropertySet*>(this);
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHOD_(ULONG, AddRef)(void)
	{
		return InterlockedIncrement(&m_RefCount);
	}

	STDMETHOD_(ULONG, Release)(void)
	{
		if (InterlockedDecrement(&m_RefCount) == 0)
		{
			delete this;
			return 0;
		}
		return m_RefCount;
	}

	STDMETHOD(GetValue)(REFPROPERTYKEY key, PROPVARIANT* value)
	{
		if (key == UI_PKEY_Label)
			return UIInitPropertyFromString(key, m_Label, value);
		else if (key == UI_PKEY_CommandId)
			return UIInitPropertyFromUInt32(UI_PKEY_CommandId, m_CommandId, value);
		else if (key == UI_PKEY_ItemImage)
			return UIInitPropertyFromInterface(UI_PKEY_ItemImage, m_Image, value);
		return E_NOTIMPL;
	}
};

void CNativeRibbonApp::SetItems(UINT cmdId, const std::list<CNativeRibbonDynamicItemInfo>& items)
{
	CComPtr<IUICollection> uiCollection = GetUICommandItemsSource(cmdId);
	if (!uiCollection)
		return;

	CComPtr<IUIImageFromBitmap> imageFactory;
	imageFactory.CoCreateInstance(__uuidof(UIRibbonImageFromBitmapFactory));

	UINT32 idx = 0;
	UINT32 count = 0;
	uiCollection->GetCount(&count);
	for (const auto & item : items)
	{
		CComPtr<IUIImage> image;
		if (imageFactory && item.GetImageId() > 0)
		{
			HBITMAP hbm = static_cast<HBITMAP>(LoadImage( AfxGetResourceHandle(), MAKEINTRESOURCE(item.GetImageId()), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
			imageFactory->CreateImage(hbm, UI_OWNERSHIP_TRANSFER, &image);
		}

		if (idx < count)
			uiCollection->Replace(idx, new UIDynamicCommandItem(item.GetCommandId(), item.GetLabel(), image));
		else
			uiCollection->Add(new UIDynamicCommandItem(item.GetCommandId(), item.GetLabel(), image));
		idx++;
	}

	for (; idx < count; idx++)
	{
		uiCollection->RemoveAt(idx);
	}
	SetUICommandItemsSource(cmdId, uiCollection);
}
