// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2023 - TortoiseGit
// Copyright (C) 2009-2010, 2015 - TortoiseSVN

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
#include "SettingsPropPage.h"


class CSettingsAdvanced : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsAdvanced)

public:
	CSettingsAdvanced();
	virtual ~CSettingsAdvanced();

	UINT GetIconID() override { return IDI_GENERAL; }

// Dialog Data
	enum { IDD = IDD_SETTINGS_CONFIG };

	class AdvancedSetting
	{
	public:
		AdvancedSetting(const wchar_t* sName) noexcept
			: sName(sName)
		{
		}
		virtual ~AdvancedSetting() = default;

		CString GetName() const { return sName; }
		virtual CString GetValue() const = 0;
		virtual bool IsValid(LPCWSTR s) const = 0;
		virtual void StoreValue(const CString& sValue) const = 0;

	protected:
		CString GetRegistryPath() const
		{
			return L"Software\\TortoiseGit\\" + GetName();
		}

	private:
		CString sName;
	};
	class StringSetting : public AdvancedSetting
	{
	public:
		StringSetting(const wchar_t* sName, LPCWSTR defaultValue) noexcept
			: AdvancedSetting(sName)
			, m_defaultValue(defaultValue)
		{
		}

		CString GetValue() const override
		{
			return CRegString(GetRegistryPath(), m_defaultValue);
		}
		bool IsValid(LPCWSTR /*s*/) const override
		{
			return true;
		}
		void StoreValue(const CString& sValue) const override
		{
			CRegString s(GetRegistryPath(), m_defaultValue);
			if (sValue.Compare(static_cast<CString>(s)))
				s = sValue;
		}

	private:
		LPCWSTR m_defaultValue;
	};
	class BooleanSetting : public AdvancedSetting
	{
	public:
		BooleanSetting(const wchar_t* sName, bool defaultValue) noexcept
			: AdvancedSetting(sName)
			, m_defaultValue(defaultValue)
		{
		}

		CString GetValue() const override
		{
			return static_cast<DWORD>(CRegDWORD(GetRegistryPath(), m_defaultValue)) ? L"true" : L"false";
		}
		bool IsValid(LPCWSTR pszText) const override
		{
			assert(pszText);
			return (!*pszText ||
					(wcscmp(pszText, L"true") == 0) ||
					(wcscmp(pszText, L"false") == 0));
		}
		void StoreValue(const CString& sValue) const override
		{
			CRegDWORD s(GetRegistryPath(), m_defaultValue);
			if (sValue.IsEmpty())
			{
				s.removeValue();
				return;
			}

			const DWORD newValue = sValue.Compare(L"true") == 0;
			if (DWORD(s) != newValue)
				s = newValue;
		}

	private:
		bool m_defaultValue = false;
	};
	class DWORDSetting : public AdvancedSetting
	{
	public:
		DWORDSetting(const wchar_t* sName, DWORD defaultValue) noexcept
			: AdvancedSetting(sName)
			, m_defaultValue(defaultValue)
		{
		}

		CString GetValue() const override
		{
			CString temp;
			temp.Format(L"%ld", static_cast<DWORD>(CRegDWORD(GetRegistryPath(), m_defaultValue)));
			return temp;
		}
		bool IsValid(LPCWSTR pszText) const override
		{
			assert(pszText);
			const wchar_t* pChar = pszText;
			while (*pChar)
			{
				if (!_istdigit(*pChar))
					return false;
				++pChar;
			}
			return true;
		}
		void StoreValue(const CString& sValue) const override
		{
			CRegDWORD s(GetRegistryPath(), m_defaultValue);
			if (sValue.IsEmpty())
			{
				s.removeValue();
				return;
			}
			if (DWORD(_wtol(sValue)) != DWORD(s))
				s = _wtol(sValue);
		}

	private:
		DWORD m_defaultValue;
	};

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	BOOL OnApply() override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	BOOL OnInitDialog() override;
	afx_msg void OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkConfig(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

private:
	template<typename T, typename S>
	void AddSetting(const wchar_t* sName, S defaultValue)
	{
		settings.emplace_back(std::make_unique<T>(sName, defaultValue));
	}

	CListCtrl		m_ListCtrl;
	std::vector<std::unique_ptr<AdvancedSetting>> settings;
};
