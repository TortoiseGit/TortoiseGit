// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
#include "StdAfx.h"
#include "COMError.h"

#pragma comment(lib, "Rpcrt4.lib")

COMError::COMError(HRESULT hr)
{
	_com_error e(hr);
	IErrorInfo *pIErrorInfo = NULL;
	GetErrorInfo(0, &pIErrorInfo);

	if (pIErrorInfo == NULL)
	{
		e = _com_error(hr);
		message = e.ErrorMessage();
	}
	else
	{
		e = _com_error(hr, pIErrorInfo);
		message = e.ErrorMessage();
		IErrorInfo *ptrIErrorInfo = e.ErrorInfo();
		if (ptrIErrorInfo != NULL)
		{
			// IErrorInfo Interface located
			description = (WCHAR *)e.Description();
			source = (WCHAR *)e.Source();
			GUID tmpGuid = e.GUID();
			RPC_WSTR guidStr = NULL;
			// must link in Rpcrt4.lib for UuidToString
			UuidToString(&tmpGuid, &guidStr);
			uuid = (WCHAR*)guidStr;
			RpcStringFree(&guidStr);

			ptrIErrorInfo->Release();
		}
	}
}

COMError::~COMError()
{

}