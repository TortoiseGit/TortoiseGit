// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "CmdLineParser.h"
#include "TGitPath.h"
#include "git.h"



/**
 * \ingroup TortoiseProc
 * Interface for all command line commands TortoiseProc can execute.
 */
class Command
{
public:
	/**
	 * Executes the command.
	 */
	virtual bool			Execute() = 0;

	void					SetParser(const CCmdLineParser& p) {parser = p;}
	void					SetPaths(const CTGitPathList& plist, const CTGitPath path)
							{
								orgCmdLinePath = path;
								CString WinPath=path.GetWinPath();
								if(WinPath.Left(g_Git.m_CurrentDir.GetLength())==g_Git.m_CurrentDir)
								{
									if(g_Git.m_CurrentDir[g_Git.m_CurrentDir.GetLength()-1] == _T('\\'))
									{
										cmdLinePath.SetFromWin( WinPath.Right(WinPath.GetLength()-g_Git.m_CurrentDir.GetLength()));
									}
									else
									{
										cmdLinePath.SetFromWin( WinPath.Right(WinPath.GetLength()-g_Git.m_CurrentDir.GetLength()-1));
									}
								}
								orgPathList = plist;
								for(int i=0;i<plist.GetCount();i++)
								{
									WinPath=plist[i].GetWinPath();
									CTGitPath p;
									if(WinPath.Left(g_Git.m_CurrentDir.GetLength())==g_Git.m_CurrentDir)
									{
										if(g_Git.m_CurrentDir[g_Git.m_CurrentDir.GetLength()-1] == _T('\\'))
										{
											p.SetFromWin( WinPath.Right(WinPath.GetLength()-g_Git.m_CurrentDir.GetLength()));
										}
										else
										{
											p.SetFromWin( WinPath.Right(WinPath.GetLength()-g_Git.m_CurrentDir.GetLength()-1));
										}
									}
									else
										p=plist[i];
									pathList.AddPath(p);

								}
							}
	void					SetExplorerHwnd(HWND hWnd) {hwndExplorer = hWnd;}
protected:
	CCmdLineParser			parser;
	CTGitPathList			pathList;
	CTGitPathList			orgPathList;
	CTGitPath				cmdLinePath;
	CTGitPath				orgCmdLinePath;
	HWND					hwndExplorer;
};

/**
 * \ingroup TortoiseProc
 * Factory for the different commands which TortoiseProc executes from the
 * command line.
 */
class CommandServer
{
public:

	Command *				GetCommand(const CString& sCmd);
};