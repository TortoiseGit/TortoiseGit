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

#include "ShellExt.h"

CExplorerCommandEnum::CExplorerCommandEnum(const std::vector<Microsoft::WRL::ComPtr<CExplorerCommand>>& vec)
	: m_iCur(0)
{
	m_vecCommands = vec;
}

HRESULT __stdcall CExplorerCommandEnum::Next(ULONG celt, IExplorerCommand** rgelt, ULONG* pceltFetched)
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

		rgelt[i] = Microsoft::WRL::Make<CExplorerCommand>(m_vecCommands[m_iCur]->m_title,
															m_vecCommands[m_iCur]->m_iconId,
															m_vecCommands[m_iCur]->m_cmd,
															m_vecCommands[m_iCur]->m_appDir,
															m_vecCommands[m_iCur]->m_uuidSource,
															m_vecCommands[m_iCur]->m_itemStates,
															m_vecCommands[m_iCur]->m_itemStatesFolder,
															m_vecCommands[m_iCur]->m_paths,
															m_vecCommands[m_iCur]->m_subItems,
															m_vecCommands[m_iCur]->m_site)
						.Detach();

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
		auto newEnum = Microsoft::WRL::Make<CExplorerCommandEnum>(m_vecCommands);

		newEnum->m_iCur = m_iCur;
		*ppenum = newEnum.Detach();
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
								   std::vector<Microsoft::WRL::ComPtr<CExplorerCommand>> subItems,
								   Microsoft::WRL::ComPtr<IUnknown> site)
	: m_title(title)
	, m_iconId(iconId)
	, m_cmd(cmd)
	, m_appDir(appDir)
	, m_uuidSource(uuidSource)
	, m_itemStates(itemStates)
	, m_itemStatesFolder(itemStatesFolder)
	, m_paths(paths)
	, m_subItems(subItems)
	, m_site(site)
{
}

HRESULT __stdcall CExplorerCommand::GetTitle(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszName)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
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
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
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

HRESULT __stdcall CExplorerCommand::GetToolTip(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszInfotip)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
	*ppszInfotip = nullptr;
	return E_NOTIMPL;
}

HRESULT __stdcall CExplorerCommand::GetCanonicalName(GUID *pguidCommandName)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__);
	*pguidCommandName = GUID_NULL;
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::GetState(IShellItemArray * /*psiItemArray*/, BOOL /*fOkToBeSlow*/, EXPCMDSTATE *pCmdState)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
	*pCmdState = ECS_ENABLED;
	if (m_title.empty())
		*pCmdState = ECS_DISABLED;
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::Invoke(IShellItemArray * /*psiItemArray*/, IBindCtx * /*pbc*/)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
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

	CRegStdString regDiffLater{ L"Software\\TortoiseGitMerge\\DiffLater", L"" };
	CShellExt::InvokeCommand(m_cmd, m_appDir, m_uuidSource,
							 GetForegroundWindow(), m_itemStates, m_itemStatesFolder, m_paths,
							 m_paths.empty() ? L"" : m_paths[0],
							 regDiffLater, m_site);
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::GetFlags(EXPCMDFLAGS *pFlags)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
	*pFlags = ECF_DEFAULT;
	if (!m_subItems.empty())
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ L": has subItems\n");
		*pFlags = ECF_HASSUBCOMMANDS;
	}
	if (m_title.empty())
		*pFlags = ECF_ISSEPARATOR;
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::EnumSubCommands(IEnumExplorerCommand **ppEnum)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
	if (m_subItems.empty())
		return E_NOTIMPL;
	*ppEnum = Microsoft::WRL::Make<CExplorerCommandEnum>(m_subItems).Detach();
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::SetSite(IUnknown * pUnkSite)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
	m_site = pUnkSite;
	return S_OK;
}

HRESULT __stdcall CExplorerCommand::GetSite(REFIID riid, void ** ppvSite)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ L": title: %s\n", m_title.c_str());
	return m_site.CopyTo(riid, ppvSite);
}
