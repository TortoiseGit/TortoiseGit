// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2020 - TortoiseGit
// Copyright (C) 2014-2016 - TortoiseSVN

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
#include "SyncSettingsCommand.h"
#include "registry.h"
#include "TGitPath.h"
#include "SmartHandle.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"
//#include "PasswordDlg.h"
#include "SelectFileFilter.h"
#include "TempFile.h"
#include "ProjectProperties.h"
#include "..\..\ext\simpleini\SimpleIni.h"

#define TGIT_SYNC_VERSION		1
#define STRINGIFY(x)			#x
#define TOSTRING(x)				STRINGIFY(x)
#define TGIT_SYNC_VERSION_STR _T(TOSTRING(TGIT_SYNC_VERSION))

// registry entries
std::vector<CString> regUseArray = {
	L"TortoiseGitMerge\\*",
	L"TortoiseGitMerge\\History\\Find\\*",
	L"TortoiseOverlays\\*",
	L"TortoiseGit\\*",
	L"TortoiseGit\\LogDlg\\*",
	L"TortoiseGit\\TortoiseProc\\EmailAddress\\*",
	L"TortoiseGit\\TortoiseProc\\FormatPatch\\*",
	L"TortoiseGit\\TortoiseProc\\RequestPull\\*",
	L"TortoiseGit\\TortoiseProc\\SendMail\\*",
	L"TortoiseGit\\Colors\\*",
	L"TortoiseGit\\History\\**",
	L"TortoiseGit\\History\\repoPaths\\**",
	L"TortoiseGit\\History\\repoURLS\\**",
	L"TortoiseGit\\LogCache\\*",
	L"TortoiseGit\\Merge\\*",
	L"TortoiseGit\\RevisionGraph\\*",
	L"TortoiseGit\\StatusColumns\\*",
	L"TortoiseGit\\TortoiseGitBlame\\Workspace\\*",
};

std::vector<CString> regUseLocalArray = {
	L"TortoiseGit\\DiffTools\\*",
	L"TortoiseGit\\MergeTools\\*",
	L"TortoiseGit\\StatusColumns\\*",
};

std::vector<CString> regBlockArray = {
	L"git_cached_version",
	L"git_file_time",
	L"CheckNewerDay",
	L"checknewerweek",
	L"configdir",
	L"currentversion",
	L"*debug*",
	L"defaultcheckoutpath",
	L"diff",
	L"erroroccurred",
	L"historyhintshown",
	L"hooks",
	L"lastcheckoutpath",
	L"merge",
	L"MSysGit",
	L"Msys2Hack",
	L"systemconfig",
	L"nocontextpaths",
	L"scintilladirect2d",
	L"synccounter",
	L"synclast",
	L"syncpath",
	L"syncpw",
	L"udiffpagesetup*",
	L"*windowpos",
};

std::vector<CString> regBlockLocalArray = {
	L"configdir",
	L"hooks",
	L"scintilladirect2d",
	L"git_cached_version",
	L"git_file_time",
	L"CheckNewerDay",
	L"checknewerweek",
};

static bool HandleRegistryKey(const CString& regname, CSimpleIni& iniFile, bool bCloudIsNewer)
{
	CAutoRegKey hKey;
	CAutoRegKey hKeyKey;
	DWORD regtype = 0;
	DWORD regsize = 0;
	CString sKeyPath = L"Software";
	CString sValuePath = regname;
	CString sIniKeyName = regname;
	CString sRegname = regname;
	CString sValue;
	bool bHaveChanges = false;
	if (regname.Find(L'\\') >= 0)
	{
		// handle values in sub-keys
		sKeyPath = L"Software\\" + regname.Left(regname.ReverseFind(L'\\'));
		sValuePath = regname.Mid(regname.ReverseFind(L'\\') + 1);
	}
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sKeyPath, 0, KEY_READ, hKey.GetPointer()) == ERROR_SUCCESS)
	{
		bool bEnum = false;
		bool bEnumKeys = false;
		int index = 0;
		int keyindex = 0;
		// an asterisk means: use all values inside the specified key
		if (sValuePath == L"*")
			bEnum = true;
		if (sValuePath == L"**")
		{
			bEnumKeys = true;
			bEnum = true;
			RegOpenKeyEx(HKEY_CURRENT_USER, sKeyPath, 0, KEY_READ, hKeyKey.GetPointer());
		}
		do
		{
			if (bEnumKeys)
			{
				bEnum = true;
				index = 0;
				wchar_t cKeyName[MAX_PATH] = { 0 };
				DWORD cLen = _countof(cKeyName);
				if (RegEnumKeyEx(hKeyKey, keyindex, cKeyName, &cLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
				{
					bEnumKeys = false;
					break;
				}
				++keyindex;
				sKeyPath = L"Software\\" + regname.Left(regname.ReverseFind(L'\\')) + L"\\" + cKeyName + L"\\";
				sRegname = regname.Left(regname.ReverseFind(L'\\')) + L"\\" + cKeyName + L"\\";
				hKey.CloseHandle();
				if (RegOpenKeyEx(HKEY_CURRENT_USER, sKeyPath, 0, KEY_READ, hKey.GetPointer()) != ERROR_SUCCESS)
				{
					bEnumKeys = false;
					break;
				}
			}
			do
			{
				if (bEnum)
				{
					// start enumerating all values
					wchar_t cValueName[MAX_PATH] = { 0 };
					DWORD cLen = _countof(cValueName);
					if (RegEnumValue(hKey, index, cValueName, &cLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
					{
						bEnum = false;
						break;
					}
					++index;
					sValuePath = cValueName;
					CString sValueLower = sValuePath;
					sValueLower.MakeLower();
					bool bIgnore = false;
					for (const auto& ignore : regBlockArray)
					{
						if (wcswildcmp(ignore, sValueLower))
						{
							bIgnore = true;
							break;
						}
					}
					if (bIgnore)
						continue;
					sIniKeyName = sRegname.Left(sRegname.ReverseFind(L'\\'));
					if (sIniKeyName.IsEmpty())
						sIniKeyName = sValuePath;
					else
						sIniKeyName += L"\\" + sValuePath;
				}
				if (RegQueryValueEx(hKey, sValuePath, nullptr, &regtype, nullptr, &regsize) == ERROR_SUCCESS)
				{
					if (regtype != 0)
					{
						auto regbuf = std::make_unique<BYTE[]>(regsize);
						if (RegQueryValueEx(hKey, sValuePath, nullptr, &regtype, regbuf.get(), &regsize) == ERROR_SUCCESS)
						{
							switch (regtype)
							{
							case REG_DWORD: {
								DWORD value = *(DWORD*)regbuf.get();
								sValue = iniFile.GetValue(L"registry_dword", sIniKeyName);
								DWORD nValue = DWORD(_wtol(sValue));
								if (nValue != value)
								{
									if (bCloudIsNewer)
										RegSetValueEx(hKey, sValuePath, 0, regtype, (BYTE*)&nValue, sizeof(nValue));
									else
									{
										bHaveChanges = true;
										sValue.Format(L"%lu", value);
										iniFile.SetValue(L"registry_dword", sIniKeyName, sValue);
									}
								}
								if (bCloudIsNewer)
									iniFile.Delete(L"registry_dword", sIniKeyName);
							}
							break;
							case REG_QWORD: {
								QWORD value = *(QWORD*)regbuf.get();
								sValue = iniFile.GetValue(L"registry_qword", sIniKeyName);
								QWORD nValue = QWORD(_wtoi64(sValue));
								if (nValue != value)
								{
									if (bCloudIsNewer)
										RegSetValueEx(hKey, sValuePath, 0, regtype, (BYTE*)&nValue, sizeof(nValue));
									else
									{
										bHaveChanges = true;
										sValue.Format(L"%I64d", value);
										iniFile.SetValue(L"registry_qword", sIniKeyName, sValue);
									}
								}
								if (bCloudIsNewer)
									iniFile.Delete(L"registry_qword", sIniKeyName);
							}
							break;
							case REG_EXPAND_SZ:
							case REG_MULTI_SZ:
							case REG_SZ: {
								sValue = (LPCWSTR)regbuf.get();
								CString iniValue = iniFile.GetValue(L"registry_string", sIniKeyName);
								if (iniValue != sValue)
								{
									if (bCloudIsNewer)
										RegSetValueEx(hKey, sValuePath, 0, regtype, (BYTE*)(LPCWSTR)iniValue, (iniValue.GetLength() + 1) * sizeof(WCHAR));
									else
									{
										bHaveChanges = true;
										iniFile.SetValue(L"registry_string", sIniKeyName, sValue);
									}
								}
								if (bCloudIsNewer)
									iniFile.Delete(L"registry_string", sIniKeyName);
							}
							break;
							}
						}
					}
				}
			} while (bEnum);
		} while (bEnumKeys);
	}
	return bHaveChanges;
}

bool SyncSettingsCommand::Execute()
{
	bool bRet = false;
	CRegString rSyncPath(L"Software\\TortoiseGit\\SyncPath");
	CTGitPath syncPath = CTGitPath(CString(rSyncPath));
	CTGitPath syncFolder = syncPath;
	CRegDWORD regCount(L"Software\\TortoiseGit\\SyncCounter");
	CRegDWORD regSyncAuth(L"Software\\TortoiseGit\\SyncAuth");
	bool bSyncAuth = DWORD(regSyncAuth) != 0;
	if (!cmdLinePath.IsEmpty())
		syncPath = cmdLinePath;
	if (syncPath.IsEmpty() && !parser.HasKey(L"askforpath"))
		return false;
	syncPath.AppendPathString(L"tsvnsync.tsex");

	BOOL bWithLocals = FALSE;
	if (parser.HasKey(L"askforpath"))
	{
		// ask for the path first, then for the password
		// this is used for a manual import/export
		CString path;
		bool bGotPath = FileOpenSave(path, bWithLocals, !!parser.HasKey(L"load"), GetExplorerHWND());
		if (bGotPath)
		{
			syncPath = CTGitPath(path);
			if (!parser.HasKey(L"load") && syncPath.GetFileExtension().IsEmpty())
				syncPath.AppendRawString(L".tsex");
		}
		else
			return false;
	}


	CSimpleIni iniFile;
	iniFile.SetMultiLine(true);

	CAutoRegKey hMainKey;
	RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\TortoiseGit", 0, KEY_READ, hMainKey.GetPointer());
	FILETIME filetime = { 0 };
	RegQueryInfoKey(hMainKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &filetime);

	bool bCloudIsNewer = false;
	/*if (!parser.HasKey(L"save"))
	{
		// open the file in read mode
		CAutoFile hFile = CreateFile(syncPath.GetWinPathString(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile.IsValid())
		{
			// load the file
			LARGE_INTEGER fsize = { 0 };
			if (GetFileSizeEx(hFile, &fsize))
			{
				auto filebuf = std::make_unique<char[]>(DWORD(fsize.QuadPart));
				DWORD bytesread = 0;
				if (ReadFile(hFile, filebuf.get(), DWORD(fsize.QuadPart), &bytesread, NULL))
				{
					// decrypt the file contents
					std::string encrypted;
					if (bytesread > 0)
						encrypted = std::string(filebuf.get(), bytesread);
					CRegString regPW(L"Software\\TortoiseGit\\SyncPW");
					CString password;
					if (parser.HasKey(L"askforpath") && parser.HasKey(L"load"))
					{
						INT_PTR dlgret = 0;
						bool bPasswordMatches = true;
						do
						{
							bPasswordMatches = true;
							CPasswordDlg passDlg(CWnd::FromHandle(GetExplorerHWND()));
							passDlg.m_bForSave = !!parser.HasKey(L"save");
							dlgret = passDlg.DoModal();
							password = passDlg.m_sPW1;
							if ((dlgret == IDOK) && (parser.HasKey(L"load")))
							{
								std::string passworda = CUnicodeUtils::StdGetUTF8((LPCWSTR)password);
								std::string decrypted = CStringUtils::Decrypt(encrypted, passworda);
								if ((decrypted.size() < 3) || (decrypted.substr(0, 3) != "***"))
									bPasswordMatches = false;
							}
						} while ((dlgret == IDOK) && !bPasswordMatches);
						if (dlgret != IDOK)
							return false;
					}
					else
					{
						auto passwordbuf = CStringUtils::Decrypt(CString(regPW));
						if (passwordbuf.get())
							password = passwordbuf.get();
						else
						{
							// password does not match or it couldn't be read from
							// the registry!
							//
							TaskDialog(hWndExplorer, AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_SYNC_WRONGPASSWORD), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
							CString sCmd = L" /command:settings /page:21";
							CAppUtils::RunTortoiseProc(sCmd);
							return false;
						}
					}
					std::string passworda = CUnicodeUtils::StdGetUTF8((LPCWSTR)password);
					std::string decrypted = CStringUtils::Decrypt(encrypted, passworda);
					if (decrypted.size() >= 3)
					{
						if (decrypted.substr(0, 3) == "***")
						{
							decrypted = decrypted.substr(3);
							// pass the decrypted data to the ini file
							iniFile.LoadFile(decrypted.c_str(), decrypted.size());
							int inicount = _wtoi(iniFile.GetValue(L"sync", L"synccounter", L""));
							if (inicount != 0)
							{
								if (int(DWORD(regCount)) < inicount)
								{
									bCloudIsNewer = true;
									regCount = inicount;
								}
							}
						}
						else
						{
							CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error decrypting, password may be wrong\n");
							return false;
						}
					}
				}
			}
		}
		else
		{
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error opening file %s, Error %u\n", syncPath.GetWinPath(), GetLastError());
			auto lasterr = GetLastError();
			if ((lasterr != ERROR_FILE_NOT_FOUND) && (lasterr != ERROR_PATH_NOT_FOUND))
				return false;
		}
	}*/

	if (parser.HasKey(L"load"))
		bCloudIsNewer = true;
	if (parser.HasKey(L"save"))
		bCloudIsNewer = false;

	bool bHaveChanges = false;

	if (bWithLocals || parser.HasKey(L"local"))
	{
		// remove all blocks that are allowed for local exports
		for (const auto& allow : regBlockLocalArray)
			regBlockArray.erase(std::remove(regBlockArray.begin(), regBlockArray.end(), allow), regBlockArray.end());
	}
	// go through all registry values and update either the registry
	// or the ini file, depending on which is newer
	for (const auto& regname : regUseArray)
	{
		bool bChanges = HandleRegistryKey(regname, iniFile, bCloudIsNewer);
		bHaveChanges = bHaveChanges || bChanges;
	}
	if (bWithLocals || parser.HasKey(L"local"))
	{
		for (const auto& regname : regUseLocalArray)
		{
			bool bChanges = HandleRegistryKey(regname, iniFile, bCloudIsNewer);
			bHaveChanges = bHaveChanges || bChanges;
		}
	}

	if (bCloudIsNewer)
	{
		CString regpath = L"Software\\";

		CSimpleIni::TNamesDepend keys;
		iniFile.GetAllKeys(L"registry_dword", keys);
		for (const auto& k : keys)
		{
			CRegDWORD reg(regpath + k.pItem);
			reg = _wtol(iniFile.GetValue(L"registry_dword", k.pItem, L""));
		}

		/*keys.clear();
		iniFile.GetAllKeys(L"registry_qword", keys);
		for (const auto& k : keys)
		{
			CRegQWORD reg(regpath + k);
			reg = _wtoi64(iniFile.GetValue(L"registry_qword", k, L""));
		}*/

		keys.clear();
		iniFile.GetAllKeys(L"registry_string", keys);
		for (const auto& k : keys)
		{
			CRegString reg(regpath + k.pItem);
			reg = CString(iniFile.GetValue(L"registry_string", k.pItem, L""));
		}
	}

	{
		// sync TortoiseMerge regex filters
		CSimpleIni regexIni;
		regexIni.SetMultiLine(true);
		CString sDataFilePath = CPathUtils::GetAppDataDirectory();
		sDataFilePath += L"\\regexfilters.ini";

		if (bCloudIsNewer)
		{
			CSimpleIni origRegexIni;

			if (PathFileExists(sDataFilePath))
			{
				int retrycount = 5;
				SI_Error err = SI_OK;
				do
				{
					err = origRegexIni.LoadFile(sDataFilePath);
					if (err == SI_FILE)
					{
						CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error loading %s, retrycount %d\n", (LPCWSTR)sDataFilePath, retrycount);
						Sleep(500);
					}
				} while ((err == SI_FILE) && --retrycount);

				if (err == SI_FILE)
					return false;
			}

			CSimpleIni::TNamesDepend keys;
			iniFile.GetAllKeys(L"ini_tmergeregex", keys);
			for (const auto& k : keys)
			{
				CString sKey = k.pItem;
				CString sSection = sKey.Left(sKey.Find(L'.'));
				sKey = sKey.Mid(sKey.Find(L'.') + 1);
				CString sValue = CString(iniFile.GetValue(L"ini_tmergeregex", k.pItem, L""));
				regexIni.SetValue(sSection, sKey, sValue);
			}
			FILE * pFile = NULL;
			errno_t err = 0;
			int retrycount = 5;
			CString sTempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
			do
			{
				err = _tfopen_s(&pFile, sTempfile, L"wb");
				if ((err == 0) && pFile)
				{
					regexIni.SaveFile(pFile);
					err = fclose(pFile);
				}
				if (err)
				{
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error saving %s, retrycount %d\n", (LPCWSTR)sTempfile, retrycount);
					Sleep(500);
				}
			} while (err && retrycount--);
			if (err == 0)
			{
				if (!CopyFile(sTempfile, sDataFilePath, FALSE))
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error copying %s to %s, Error: %u\n", (LPCWSTR)sTempfile, (LPCWSTR)sDataFilePath, GetLastError());
			}
		}
		else
		{
			if (PathFileExists(sDataFilePath))
			{
				int retrycount = 5;
				SI_Error err = SI_OK;
				do
				{
					err = regexIni.LoadFile(sDataFilePath);
					if (err == SI_FILE)
					{
						CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error loading %s, retrycount %d\n", (LPCWSTR)sDataFilePath, retrycount);
						Sleep(500);
					}
				} while ((err == SI_FILE) && retrycount--);

				if (err == SI_FILE)
					return false;
			}

			CSimpleIni::TNamesDepend mitems;
			regexIni.GetAllSections(mitems);
			for (const auto& mitem : mitems)
			{
				CString sSection = mitem.pItem;

				CString newval = regexIni.GetValue(mitem.pItem, L"regex", L"");
				CString oldval = iniFile.GetValue(L"ini_tmergeregex", sSection + L".regex", L"");
				bHaveChanges |= newval != oldval;
				iniFile.SetValue(L"ini_tmergeregex", sSection + L".regex", newval);

				newval = regexIni.GetValue(mitem.pItem, L"replace", L"5");
				oldval = iniFile.GetValue(L"ini_tmergeregex", sSection + L".replace", L"0");
				bHaveChanges |= newval != oldval;
				iniFile.SetValue(L"ini_tmergeregex", sSection + L".replace", newval);
			}
		}
	}


	if (bHaveChanges)
	{
		iniFile.SetValue(L"sync", L"version", TGIT_SYNC_VERSION_STR);
		DWORD count = regCount;
		++count;
		regCount = count;
		CString tmp;
		tmp.Format(L"%lu", count);
		iniFile.SetValue(L"sync", L"synccounter", tmp);

		std::string encrypted;
		iniFile.Save(encrypted);

/*
		// save the ini file
		std::string iniData;
		iniFile.SaveString(iniData);
		iniData = "***" + iniData;
		// encrypt the string

		CString password;
		if (parser.HasKey(L"askforpath"))
		{
			CPasswordDlg passDlg(CWnd::FromHandle(GetExplorerHWND()));
			passDlg.m_bForSave = true;
			if (passDlg.DoModal() != IDOK)
				return false;
			password = passDlg.m_sPW1;
		}
		else
		{
			CRegString regPW(L"Software\\TortoiseGit\\SyncPW");
			auto passwordbuf = CStringUtils::Decrypt(CString(regPW));
			if (passwordbuf.get())
				password = passwordbuf.get();
		}

		std::string passworda = CUnicodeUtils::StdGetUTF8((LPCWSTR)password);

		std::string encrypted = CStringUtils::Encrypt(iniData, passworda);*/
		CPathUtils::MakeSureDirectoryPathExists(syncPath.GetContainingDirectory().GetWinPathString());
		CString sTempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
		CAutoFile hFile = CreateFile(sTempfile, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile.IsValid())
		{
			DWORD written = 0;
			if (WriteFile(hFile, encrypted.c_str(), DWORD(encrypted.size()), &written, NULL))
			{
				if (hFile.CloseHandle())
				{
					if (!CopyFile(sTempfile, syncPath.GetWinPath(), FALSE))
						CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error copying %s to %s, Error: %u\n", (LPCWSTR)sTempfile, syncPath.GetWinPath(), GetLastError());
					else
						bRet = true;
				}
				else
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error closing file %s, Error: %u\n", (LPCWSTR)sTempfile, GetLastError());
			}
			else
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error writing to file %s, Error: %u\n", (LPCWSTR)sTempfile, GetLastError());
		}
		else
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error creating file %s for writing, Error: %u\n", (LPCWSTR)sTempfile, GetLastError());
	}

	return bRet;
}

bool SyncSettingsCommand::FileOpenSave(CString& path, BOOL& bWithLocals, bool bOpen, HWND hwndOwner)
{
	path = L"d:\\test.txt";
	bWithLocals = true;
	return true;
	HRESULT hr;
	bWithLocals = FALSE;
	// Create a new common save file dialog
	/*CComPtr<IFileDialog> pfd;

	if (!SUCCEEDED(pfd.CoCreateInstance(bOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER)))
		return false;
	if (SUCCEEDED(hr))
	{
		// Set the dialog options
		DWORD dwOptions;
		if (SUCCEEDED(hr = pfd->GetOptions(&dwOptions)))
		{
			if (bOpen)
				hr = pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
			else
			{
				hr = pfd->SetOptions(dwOptions | FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
				hr = pfd->SetFileName(CPathUtils::GetFileNameFromPath(path));
			}
		}

		// Set a title
		if (SUCCEEDED(hr))
		{
			CString temp;
			temp.LoadString(IDS_SYNC_SETTINGSFILE);
			CStringUtils::RemoveAccelerators(temp);
			pfd->SetTitle(temp);
		}

		CSelectFileFilter fileFilter(IDS_EXPORTFILTER);
		hr = pfd->SetFileTypes(fileFilter.GetCount(), fileFilter);

		if (!bOpen)
		{
			CComPtr<IFileDialogCustomize> pfdCustomize;
			hr = pfd.QueryInterface(&pfdCustomize);
			if (SUCCEEDED(hr))
				pfdCustomize->AddCheckButton(101, CString(MAKEINTRESOURCE(IDS_SYNC_INCLUDELOCAL)), FALSE);
		}

		// Show the save/open file dialog
		if (SUCCEEDED(hr) && SUCCEEDED(hr = pfd->Show(hwndOwner)))
		{
			// Get the selection from the user
			CComPtr<IShellItem> psiResult = nullptr;
			hr = pfd->GetResult(&psiResult);
			if (SUCCEEDED(hr))
			{
				PWSTR pszPath = nullptr;
				hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
				if (SUCCEEDED(hr))
				{
					path = CString(pszPath);
					if (!bOpen)
					{
						CComPtr<IFileDialogCustomize> pfdCustomize;
						hr = pfd.QueryInterface(&pfdCustomize);
						if (SUCCEEDED(hr))
							pfdCustomize->GetCheckButtonState(101, &bWithLocals);
					}
					return true;
				}
			}
		}
	}
	return false;*/
}
