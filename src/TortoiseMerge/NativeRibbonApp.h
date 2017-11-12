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
#pragma once

class CNativeRibbonDynamicItemInfo
{
public:
	CNativeRibbonDynamicItemInfo(UINT cmdId, const CString& text, UINT imageId)
		: m_CmdId(cmdId)
		, m_Text(text)
		, m_ImageId(imageId)
	{
	}

	const CString & GetLabel() const { return m_Text; }
	UINT GetCommandId() const { return m_CmdId; }
	UINT GetImageId() const { return m_ImageId; }
private:
	UINT m_CmdId;
	CString m_Text;
	UINT m_ImageId;
};

class CNativeRibbonApp : public IUIApplication, public IUICommandHandler
{
public:
	CNativeRibbonApp(CFrameWnd* pFrame, IUIFramework* pFramework);
	~CNativeRibbonApp();

	void SetSettingsFileName(const CString& file)
	{
		m_SettingsFileName = file;
	}

	void UpdateCmdUI(BOOL bDisableIfNoHandler);
	int GetRibbonHeight();
	void SetItems(UINT cmdId, const std::list<CNativeRibbonDynamicItemInfo>& items);

protected:
	// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

		// IUIApplication
	STDMETHOD(OnViewChanged)(UINT32 viewId, UI_VIEWTYPE typeID, IUnknown* view, UI_VIEWVERB verb, INT32 uReasonCode);

	STDMETHOD(OnCreateUICommand)(UINT32 commandId, UI_COMMANDTYPE typeID, IUICommandHandler** commandHandler);

	STDMETHOD(OnDestroyUICommand)(UINT32 commandId, UI_COMMANDTYPE typeID, IUICommandHandler* commandHandler);

	// IUICommandHandler
	STDMETHOD(Execute)(UINT32 commandId, UI_EXECUTIONVERB verb, const PROPERTYKEY* key, const PROPVARIANT* currentValue, IUISimplePropertySet* commandExecutionProperties);

	STDMETHOD(UpdateProperty)(UINT32 commandId, REFPROPERTYKEY key, const PROPVARIANT* currentValue, PROPVARIANT* newValue);

	HRESULT SaveRibbonViewSettings(IUIRibbon* pRibbonView, const CString& fileName);
	HRESULT LoadRibbonViewSettings(IUIRibbon* pRibbonView, const CString& fileName);
	CComPtr<IUICollection> GetUICommandItemsSource(UINT commandId);
	void SetUICommandItemsSource(UINT commandId, IUICollection* pItems);
	static UINT GetCommandIdProperty(IUISimplePropertySet* propertySet);

private:
	CFrameWnd* m_pFrame;
	CComPtr<IUIFramework> m_pFramework;
	std::list<UINT32> m_commandIds;
	std::list<UINT32> m_collectionCommandIds;
	ULONG m_cRefCount;
	CString m_SettingsFileName;
};
