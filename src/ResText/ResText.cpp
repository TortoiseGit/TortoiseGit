// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "stdafx.h"
#include "ResModule.h"

#include <string>
#include <vector>
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")

typedef std::basic_string<wchar_t> wstring;
#ifdef UNICODE
#	define stdstring wstring
#else
#	define stdstring std::string
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	bool bShowHelp = true;
	bool bQuiet = false;
	bool bNoUpdate = false;
	bool bRTL = false;
	//parse the command line
	std::vector<stdstring> arguments;
	std::vector<stdstring> switches;
	for (int i=1; i<argc; ++i)
	{
		if ((argv[i][0] == '-')||(argv[i][0] == '/'))
		{
			stdstring str = stdstring(&argv[i][1]);
			switches.push_back(str);
		}
		else
		{
			stdstring str = stdstring(&argv[i][0]);
			arguments.push_back(str);
		}
	}

	for (std::vector<stdstring>::iterator I = switches.begin(); I != switches.end(); ++I)
	{
		if (_tcscmp(I->c_str(), _T("?"))==0)
			bShowHelp = true;
		if (_tcscmp(I->c_str(), _T("help"))==0)
			bShowHelp = true;
		if (_tcscmp(I->c_str(), _T("quiet"))==0)
			bQuiet = true;
		if (_tcscmp(I->c_str(), _T("noupdate"))==0)
			bNoUpdate = true;
		if (_tcscmp(I->c_str(), _T("rtl"))==0)
			bRTL = true;
	}
	std::vector<stdstring>::iterator arg = arguments.begin();

	if (arg != arguments.end())
	{
		if (_tcscmp(arg->c_str(), _T("extract"))==0)
		{
			stdstring sDllFile;
			stdstring sPoFile;
			++arg;
			
			std::vector<std::wstring> filelist = arguments;
			filelist.erase(filelist.begin());
			sPoFile = stdstring((--filelist.end())->c_str());
			filelist.erase(--filelist.end());
			
			CResModule module;
			module.SetQuiet(bQuiet);
			if (!module.ExtractResources(filelist, sPoFile.c_str(), bNoUpdate))
				return -1;
			bShowHelp = false;
		}
		else if (_tcscmp(arg->c_str(), _T("apply"))==0)
		{
			stdstring sSrcDllFile;
			stdstring sDstDllFile;
			stdstring sPoFile;
			WORD wLang = 0;
			++arg;
			if (!PathFileExists(arg->c_str()))
			{
				_ftprintf(stderr, _T("the resource dll <%s> does not exist!\n"), arg->c_str());
				return -1;
			}
			sSrcDllFile = stdstring(arg->c_str());
			++arg;
			sDstDllFile = stdstring(arg->c_str());
			++arg;
			if (!PathFileExists(arg->c_str()))
			{
				_ftprintf(stderr, _T("the po-file <%s> does not exist!\n"), arg->c_str());
				return -1;
			}
			sPoFile = stdstring(arg->c_str());
			++arg;
			if (arg != arguments.end())
			{
				wLang = (WORD)_ttoi(arg->c_str());
			}
			CResModule module;
			module.SetQuiet(bQuiet);
			module.SetLanguage(wLang);
			module.SetRTL(bRTL);
			if (!module.CreateTranslatedResources(sSrcDllFile.c_str(), sDstDllFile.c_str(), sPoFile.c_str()))
				return -1;
			bShowHelp = false;
		}
	}

	if (bShowHelp)
	{
		_ftprintf(stdout, _T("usage:\n"));
		_ftprintf(stdout, _T("\n"));
		_ftprintf(stdout, _T("ResText extract <resource.dll> [<resource.dll> ...] <po-file> [-quiet] [-noupdate]\n"));
		_ftprintf(stdout, _T("Extracts all strings from the resource dll and writes them to the po-file\n"));
		_ftprintf(stdout, _T("-quiet: don't print progress messages\n"));
		_ftprintf(stdout, _T("-noupdate: overwrite the po-file\n"));
		_ftprintf(stdout, _T("\n"));
		_ftprintf(stdout, _T("ResText apply <src resource.dll> <dst resource.dll> <po-file> [langID] [-quiet][-rtl]\n"));
		_ftprintf(stdout, _T("Replaces all strings in the dst resource.dll with the po-file translations\n"));
		_ftprintf(stdout, _T("-quiet: don't print progress messages\n"));
		_ftprintf(stdout, _T("-rtl  : change the controls to RTL reading\n"));
		_ftprintf(stdout, _T("\n"));
	}

	return 0;
}

