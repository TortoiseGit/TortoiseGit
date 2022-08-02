// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2021-2022 - TortoiseSVN

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
#include <Shobjidl.h>
#include "ShellExt.h"

class CExplorerCommand;

class CExplorerCommandEnum : public IEnumExplorerCommand
{
public:
	explicit CExplorerCommandEnum(const std::vector<CExplorerCommand>& vec);
	virtual ~CExplorerCommandEnum() = default;

	// IUnknown members
	HRESULT __stdcall QueryInterface(REFIID, void**) override;
	ULONG __stdcall AddRef() override;
	ULONG __stdcall Release() override;

	// IEnumExplorerCommand members
	HRESULT __stdcall Next(ULONG, IExplorerCommand**, ULONG*) override;
	HRESULT __stdcall Skip(ULONG) override;
	HRESULT __stdcall Reset() override;
	HRESULT __stdcall Clone(IEnumExplorerCommand**) override;

private:
	std::vector<CExplorerCommand> m_vecCommands;
	ULONG m_cRefCount = 0;
	size_t m_iCur = 0;
};

class __declspec(uuid("A4FEC38E-C9F5-489A-ADAB-28DA10F523A3")) CExplorerCommand : public IExplorerCommand, IObjectWithSite
{
	friend class CExplorerCommandEnum;

public:
	explicit CExplorerCommand(const std::wstring& title, UINT iconId,
								int cmd,
								const std::wstring& appDir,
								const std::wstring& uuidSource,
								DWORD itemStates,
								DWORD itemStatesFolder,
								std::vector<std::wstring> paths,
								std::vector<CExplorerCommand> subItems,
								Microsoft::WRL::ComPtr<IUnknown> site);

	virtual ~CExplorerCommand() = default;
	void PrependTitleWith(const std::wstring& prep) { m_title = prep + m_title; }

	// IUnknown members
	HRESULT __stdcall QueryInterface(REFIID, void**) override;
	ULONG __stdcall AddRef() override;
	ULONG __stdcall Release() override;

	// Inherited via IExplorerCommand
	HRESULT __stdcall GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName) override;
	HRESULT __stdcall GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon) override;
	HRESULT __stdcall GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip) override;
	HRESULT __stdcall GetCanonicalName(GUID* pguidCommandName) override;
	HRESULT __stdcall GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState) override;
	HRESULT __stdcall Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc) override;
	HRESULT __stdcall GetFlags(EXPCMDFLAGS* pFlags) override;
	HRESULT __stdcall EnumSubCommands(IEnumExplorerCommand** ppEnum) override;

	// Inherited via IObjectWithSite
	HRESULT __stdcall SetSite(IUnknown* pUnkSite) override;
	HRESULT __stdcall GetSite(REFIID riid, void** ppvSite) override;

private:
	ULONG m_cRefCount;

	std::wstring					m_title;
	UINT							m_iconId = 0;
	int								m_cmd = 0;
	std::wstring					m_appDir;
	std::wstring					m_uuidSource;
	DWORD							m_itemStates = 0;
	DWORD							m_itemStatesFolder = 0;
	std::vector<std::wstring>		m_paths;
	std::vector<CExplorerCommand>	m_subItems;
	CRegStdString					m_regDiffLater;
	Microsoft::WRL::ComPtr<IUnknown>	m_site;
};
