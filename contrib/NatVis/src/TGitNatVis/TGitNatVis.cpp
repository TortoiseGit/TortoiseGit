// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2017 - TortoiseGit

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
#include "TGitNatVis.h"

#define GIT_HASH_SIZE 20

typedef struct tagDEBUGHELPER
{
	DWORD dwVersion;
	HRESULT(WINAPI* ReadDebuggeeMemory)(struct tagDEBUGHELPER* pThis, DWORD dwAddr, DWORD nWant, VOID* pWhere, DWORD* nGot);
	// from here only when dwVersion >= 0x20000
	DWORDLONG(WINAPI* GetRealAddress)(struct tagDEBUGHELPER* pThis);
	HRESULT(WINAPI* ReadDebuggeeMemoryEx)(struct tagDEBUGHELPER* pThis, DWORDLONG qwAddr, DWORD nWant, VOID* pWhere, DWORD* nGot);
	int (WINAPI* GetProcessorType)(struct tagDEBUGHELPER* pThis);
} DEBUGHELPER;

typedef HRESULT(WINAPI *CUSTOMVIEWER)(DWORD dwAddress, DEBUGHELPER* pHelper, int nBase, BOOL bUniStrings, char* pResult, size_t max, DWORD reserved);

static CStringA ToString(const unsigned char* hash)
{
	CStringA str;
	for (int i = 0; i < GIT_HASH_SIZE; ++i)
		str.AppendFormat("%02x", hash[i]);
	return str;
}

extern "C" TGITNATVIS_API HRESULT SHA1Formatter(DWORD dwAddress, DEBUGHELPER* pHelper, int nBase, BOOL bUniStrings, char* pResult, size_t max, DWORD reserved);

TGITNATVIS_API HRESULT SHA1Formatter(DWORD dwAddress, DEBUGHELPER* pHelper, int nBase, BOOL bUniStrings, char* pResult, size_t max, DWORD reserved)
{
	unsigned char hash[GIT_HASH_SIZE];
	DWORD nGot;
	auto realAddress = pHelper->GetRealAddress(pHelper);
	pHelper->ReadDebuggeeMemoryEx(pHelper, realAddress, GIT_HASH_SIZE, &hash, &nGot);
	//sprintf_s(pResult, max, "Dll MyClass: max=%d nGot=%d MyClass=%llx SHA1=%s", max, nGot, realAddress, (LPCSTR)ToString(hash));
	sprintf_s(pResult, max, "SHA1: %s", (LPCSTR)ToString(hash));
	return S_OK;
}
