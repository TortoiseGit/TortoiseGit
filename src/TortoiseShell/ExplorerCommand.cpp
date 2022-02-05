// TortoiseGit- a Windows shell extension for easy version control

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
#include "stdafx.h"
#include "ExplorerCommand.h"

CExplorerCommandEnum::CExplorerCommandEnum(const std::vector<CExplorerCommand> &vec)
: m_vecCommands(vec)
{
}

HRESULT __stdcall CExplorerCommandEnum::QueryInterface(REFIID refiid, void **ppv)
{
	*ppv = nullptr;
	if (IID_IUnknown == refiid || IID_IEnumExplorerCommand == refiid)
		*ppv = this;

	if (*ppv)
	{
		static_cast<LPUNKNOWN>(*ppv)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG __stdcall CExplorerCommandEnum::AddRef()
{
	return ++m_cRefCount;
}

ULONG __stdcall CExplorerCommandEnum::Release()
{
	--m_cRefCount;
	if (m_cRefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_cRefCount;
}

HRESULT __stdcall CExplorerCommandEnum::Next(ULONG celt, IExplorerCommand **rgelt, ULONG *pceltFetched)
{
	HRESULT hr = S_FALSE;

	if (!celt)
		celt = 1;
	if (pceltFetched)
		*pceltFetched = 0;

	ULONG i = 0;
	for (; i < celt; ++i)
	{
		if (m_iCur == static_cast<ULONG>(m_vecCommands.size()))
			break;

		rgelt[i] = new CExplorerCommand(m_vecCommands[m_iCur].m_title,
										m_vecCommands[m_iCur].m_iconId,
										m_vecCommands[m_iCur].m_cmd,
										m_vecCommands[m_iCur].m_appDir,
										m_vecCommands[m_iCur].m_uuidSource,
										m_vecCommands[m_iCur].m_itemStates,
										m_vecCommands[m_iCur].m_itemStatesFolder,
										m_vecCommands[m_iCur].m_paths,
										m_vecCommands[m_iCur].m_subItems,
										m_vecCommands[m_iCur].m_site);
		rgelt[i]->AddRef();

		if (pceltFetched)
			(*pceltFetched)++;

		++m_iCur;
	}

	if (i == celt)
		hr = S_OK;

	return hr;
}

HRESULT __stdcall CExplorerCommandEnum::Skip(ULONG celt)
{
	if ((m_iCur + static_cast<int>(celt)) >= m_vecCommands.size())
		return S_FALSE;
	m_iCur += celt;
	return S_OK;
}

HRESULT __stdcall CExplorerCommandEnum::Reset()
{
	m_iCur = 0;
	return S_OK;
}

HRESULT __stdcall CExplorerCommandEnum::Clone(IEnumExplorerCommand **ppenum)
{
	if (!ppenum)
		return E_POINTER;

	try
	{
		CExplorerCommandEnum *newEnum = new CExplorerCommandEnum(m_vecCommands);

		newEnum->AddRef();
		newEnum->m_iCur = m_iCur;
		*ppenum = newEnum;
	}
	catch (const std::bad_alloc &)
	{
		return E_OUTOFMEMORY;
	}
	return S_OK;
}

CExplorerCommand::CExplorerCommand(const std::wstring &title, UINT iconId,
								   int                           cmd,
								   const std::wstring &          appDir,
								   const std::wstring &          uuidSource,
								   DWORD                         itemStates,
								   DWORD                         itemStatesFolder,
								   std::vector<std::wstring>     paths,
								   std::vector<CExplorerCommand> subItems,
								   Microsoft::WRL::ComPtr<IUnknown> site)
	: m_cRefCount(0)
	, m_title(title)
	, m_iconId(iconId)
	, m_cmd(cmd)
	, m_appDir(appDir)
	, m_uuidSource(uuidSource)
	, m_itemStates(itemStates)
	, m_itemStatesFolder(itemStatesFolder)
	, m_paths(paths)
	, m_subItems(subItems)
	, m_regDiffLater(L"Software\\TortoiseGitMerge\\DiffLater", L"")
	, m_site(site)
{
}

HRESULT __stdcall CExplorerCommand::QueryInterface(REFIID refiid, void **ppv)
{
	*ppv = nullptr;
	if (IID_IUnknown == refiid || IID_IExplorerCommand == refiid)
		*ppv = this;

	if (*ppv)
	{
		static_cast<LPUNKNOWN>(*ppv)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG __stdcall CExplorerCommand::AddRef()
{
	return ++m_cRefCount;
}

ULONG __stdcall CExplorerCommand::Release()
{
	--m_cRefCount;
	if (m_cRefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_cRefCount;
}

HRESULT __stdcall CExplorerCommand::GetTitle(IShellItemArray * /*psiItemArray*/, LPWSTR *ppszName)
{
	if (m_title.empty())
	{
		*ppszName = nullptr;
		return S_FALSE;
	}
	SHStrDupW(m_title.c_str(), ppszName);
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::GetIcon(IShellItemArray * /*psiItemArray*/, LPWSTR *ppszIcon)
{
	if (m_iconId == 0)
	{
		SHStrDupW(L"", ppszIcon);
		return S_FALSE;
	}
	std::wstring iconPath = m_appDir + L"TortoiseGitProc.exe,-";
	iconPath += std::to_wstring(m_iconId);
	SHStrDupW(iconPath.c_str(), ppszIcon);
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::GetToolTip(IShellItemArray * /*psiItemArray*/, LPWSTR * /*ppszInfotip*/)
{
	return E_NOTIMPL;
}

HRESULT __stdcall CExplorerCommand::GetCanonicalName(GUID *pguidCommandName)
{
	*pguidCommandName = CLSID_NULL;
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::GetState(IShellItemArray * /*psiItemArray*/, BOOL /*fOkToBeSlow*/, EXPCMDSTATE *pCmdState)
{
	*pCmdState = ECS_ENABLED;
	if (m_title.empty())
		return E_FAIL;
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::Invoke(IShellItemArray * /*psiItemArray*/, IBindCtx * /*pbc*/)
{
	std::wstring cwdFolder;
	if (m_paths.empty())
	{
		// use the users desktop path as the CWD
		wchar_t desktopDir[MAX_PATH] = {0};
		SHGetSpecialFolderPath(nullptr, desktopDir, CSIDL_DESKTOPDIRECTORY, TRUE);
		cwdFolder = desktopDir;
	}
	else
	{
		cwdFolder = m_paths[0];
		if (!PathIsDirectory(cwdFolder.c_str()))
		{
			cwdFolder = cwdFolder.substr(0, cwdFolder.rfind('\\'));
		}
	}

	CShellExt::InvokeCommand(m_cmd, m_appDir, m_uuidSource,
							 GetForegroundWindow(), m_itemStates, m_itemStatesFolder, m_paths,
							 m_paths.empty() ? L"" : m_paths[0],
							 m_regDiffLater, m_site);
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::GetFlags(EXPCMDFLAGS *pFlags)
{
	*pFlags = ECF_DEFAULT;
	if (!m_subItems.empty())
		*pFlags = ECF_HASSUBCOMMANDS;
	if (m_title.empty())
		*pFlags = ECF_ISSEPARATOR;
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::EnumSubCommands(IEnumExplorerCommand **ppEnum)
{
	if (m_subItems.empty())
		return E_INVALIDARG;
	*ppEnum = new CExplorerCommandEnum(m_subItems);
	(*ppEnum)->AddRef();
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::SetSite(IUnknown * pUnkSite)
{
	m_site = pUnkSite;
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::GetSite(REFIID riid, void ** ppvSite)
{
	return m_site.CopyTo(riid, ppvSite);
}
