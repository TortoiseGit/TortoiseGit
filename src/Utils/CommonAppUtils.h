// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2020 - TortoiseGit
// Copyright (C) 2003-2008,2010 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * An utility class with static functions.
 */
class CCommonAppUtils
{
public:
	struct LaunchApplicationFlags
	{
private:
		bool bWaitForStartup = false;
		bool bWaitForExit = false;
		HANDLE hWaitHandle = nullptr;
		bool bUAC = false;
		CString* psCWD = nullptr;
		UINT uiIDErrMessageFormat = 0;
		DWORD* pdwExitCode = nullptr;

		friend class CCommonAppUtils;

public:
		LaunchApplicationFlags() {}
		LaunchApplicationFlags& UseSpecificErrorMessage(UINT idErrMessageFormat)
		{
			uiIDErrMessageFormat = idErrMessageFormat;
			return *this;
		}
		LaunchApplicationFlags& WaitForStartup(bool b = true)
		{
			bWaitForStartup = b;
			return *this;
		}
		LaunchApplicationFlags& WaitForExit(bool b = true, HANDLE h = nullptr, DWORD* pExitCode = nullptr)
		{
			ASSERT(!h || b);
			bWaitForExit = b;
			hWaitHandle = h;
			pdwExitCode = pExitCode;
			return *this;
		}
		LaunchApplicationFlags& UAC(bool b = true)
		{
			bUAC = b;
			return *this;
		}
		LaunchApplicationFlags& UseCWD(CString* pCwd)
		{
			psCWD = pCwd;
			return *this;
		}
	};

	/**
	* Launch an external application (usually the diff viewer)
	*/
	static bool LaunchApplication(const CString& sCommandLine, const LaunchApplicationFlags& flags);

	static bool RunTortoiseGitProc(const CString& sCommandLine, bool uac = false, bool includeGroupingUUID = true);

	static bool IsAdminLogin();

	static bool SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID);
	static bool SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width, int height);

	static bool FileOpenSave(CString& path, int* filterindex, UINT title, UINT filterId, bool bOpen, HWND hwndOwner = nullptr, LPCTSTR defaultExt = nullptr, bool handleAsFile = false);

	// Wrapper for LoadImage(IMAGE_ICON)
	static HICON LoadIconEx(UINT resourceId, UINT cx, UINT cy);

	/**
	 * Apply the @a effects or color (depending on @a mask)
	 * for all char ranges given in @a positions to the
	 * @a window text.
	 */
	static void SetCharFormat(CWnd* window, DWORD mask, DWORD effects, const std::vector<CHARRANGE>& positions);
	static void SetCharFormat(CWnd* window, DWORD mask, DWORD effects);

	/**
	 * Returns font name which is used for log messages, etc.
	 */
	static CString GetLogFontName();

	/**
	 * Returns font size which is used for log messages, etc.
	 */
	static DWORD GetLogFontSize();

	/**
	 * Create a font which can is used for log messages, etc
	 */
	static void CreateFontForLogs(CFont& fontToCreate);

	CCommonAppUtils() = delete;
};
