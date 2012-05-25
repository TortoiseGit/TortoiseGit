// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "Command.h"

#include "AboutCommand.h"
#include "CommitCommand.h"
#include "LogCommand.h"

#include "CreateRepositoryCommand.h"
#include "CloneCommand.h"
#include "PrevDiffCommand.h"
#include "DiffCommand.h"

#include "RenameCommand.h"
#include "RepoStatusCommand.h"

#include "RevertCommand.h"
#include "RemoveCommand.h"
#include "PullCommand.h"
#include "FetchCommand.h"
#include "PushCommand.h"
#include "BranchCommand.h"
#include "TagCommand.h"
#include "MergeCommand.h"
#include "SwitchCommand.h"
#include "ExportCommand.h"
#include "AddCommand.h"
#include "IgnoreCommand.h"
#include "FormatPatchCommand.h"
#include "ImportPatchCommand.h"
#include "BlameCommand.h"
#include "SettingsCommand.h"
#include "ConflictEditorCommand.h"
#include "CleanupCommand.h"
#include "RebaseCommand.h"
#include "ResolveCommand.h"
#include "DropMoveCommand.h"
#include "DropCopyAddCommand.h"
#include "DropCopyCommand.h"
#include "HelpCommand.h"
#include "StashCommand.h"
#include "SubmoduleCommand.h"
#include "ReflogCommand.h"
#include "SendMailCommand.h"
#include "CatCommand.h"
#include "RefBrowseCommand.h"
#include "SVNDCommitCommand.h"
#include "SVNRebaseCommand.h"
#include "SVNFetchCommand.h"
#include "SyncCommand.h"
#include "RequestPullCommand.h"
#include "UpdateCheckCommand.h"
#include "PasteCopyCommand.h"
#include "PasteMoveCommand.h"
#include "SVNIgnoreCommand.h"
#include "BisectCommand.h"
#include "RepositoryBrowserCommand.h"

#if 0


#include "CopyCommand.h"
#include "CrashCommand.h"





#include "PropertiesCommand.h"
#include "RebuildIconCacheCommand.h"
#include "RemoveCommand.h"



#include "RevertCommand.h"
#include "RTFMCommand.h"

#include "ShowCompareCommand.h"

#include "UnIgnoreCommand.h"

#endif
typedef enum
{
	cmdAbout,
	cmdAdd,
	cmdBlame,
	cmdBranch,
	cmdCat,
	cmdCleanup,
	cmdClone,
	cmdCommit,
	cmdConflictEditor,
	cmdCopy,
	cmdCrash,
	cmdDiff,
	cmdDropCopy,
	cmdDropCopyAdd,
	cmdDropMove,
	cmdFetch,
	cmdFormatPatch,
	cmdExport,
	cmdHelp,
	cmdIgnore,
	cmdImportPatch,
	cmdLog,
	cmdMerge,
	cmdPasteCopy,
	cmdPasteMove,
	cmdPrevDiff,
	cmdProperties,
	cmdPull,
	cmdPush,
	cmdRTFM,
	cmdRebuildIconCache,
	cmdRemove,
	cmdRebase,
	cmdRename,
	cmdRepoCreate,
	cmdRepoStatus,
	cmdResolve,
	cmdRevert,
	cmdSendMail,
	cmdSettings,
	cmdShowCompare,
	cmdSwitch,
	cmdTag,
	cmdUnIgnore,
	cmdUpdateCheck,
	cmdStashSave,
	cmdStashApply,
	cmdStashPop,
	cmdStashList,
	cmdSubAdd,
	cmdSubUpdate,
	cmdSubSync,
	cmdRefLog,
	cmdRefBrowse,
	cmdSVNDCommit,
	cmdSVNRebase,
	cmdSVNFetch,
	cmdSVNIgnore,
	cmdSync,
	cmdRequestPull,
	cmdBisect,
	cmdRepoBrowser,
} TGitCommand;

static const struct CommandInfo
{
	TGitCommand command;
	LPCTSTR pCommandName;
} commandInfo[] =
{
	{	cmdAbout,			_T("about")				},
	{	cmdAdd,				_T("add")				},
	{	cmdBlame,			_T("blame")				},
	{	cmdBranch,			_T("branch")			},
	{	cmdCat,				_T("cat")				},
	{	cmdCleanup,			_T("cleanup")			},
	{	cmdClone,			_T("clone")				},
	{	cmdCommit,			_T("commit")			},
	{	cmdConflictEditor,	_T("conflicteditor")	},
	{	cmdCopy,			_T("copy")				},
	{	cmdCrash,			_T("crash")				},
	{	cmdDiff,			_T("diff")				},
	{	cmdDropCopy,		_T("dropcopy")			},
	{	cmdDropCopyAdd,		_T("dropcopyadd")		},
	{	cmdDropMove,		_T("dropmove")			},
	{	cmdFetch,			_T("fetch")				},
	{	cmdFormatPatch,		_T("formatpatch")		},
	{	cmdExport,			_T("export")			},
	{	cmdHelp,			_T("help")				},
	{	cmdIgnore,			_T("ignore")			},
	{	cmdImportPatch,		_T("importpatch")		},
	{	cmdLog,				_T("log")				},
	{	cmdMerge,			_T("merge")				},
	{	cmdPasteCopy,		_T("pastecopy")			},
	{	cmdPasteMove,		_T("pastemove")			},
	{	cmdPrevDiff,		_T("prevdiff")			},
	{	cmdProperties,		_T("properties")		},
	{	cmdPull,			_T("pull")				},
	{	cmdPush,			_T("push")				},
	{	cmdRTFM,			_T("rtfm")				},
	{	cmdRebuildIconCache,_T("rebuildiconcache")	},
	{	cmdRemove,			_T("remove")			},
	{	cmdRebase,			_T("rebase")			},
	{	cmdRename,			_T("rename")			},
	{	cmdRepoCreate,		_T("repocreate")		},
	{	cmdRepoStatus,		_T("repostatus")		},
	{	cmdResolve,			_T("resolve")			},
	{	cmdRevert,			_T("revert")			},
	{	cmdSendMail,		_T("sendmail")			},
	{	cmdSettings,		_T("settings")			},
	{	cmdShowCompare,		_T("showcompare")		},
	{	cmdSwitch,			_T("switch")			},
	{	cmdTag,				_T("tag")				},
	{	cmdUnIgnore,		_T("unignore")			},
	{	cmdUpdateCheck,		_T("updatecheck")		},
	{	cmdStashSave,		_T("stashsave")			},
	{	cmdStashApply,		_T("stashapply")		},
	{	cmdStashPop,		_T("stashpop")			},
	{	cmdStashList,		_T("stashlist")			},
	{	cmdSubAdd,			_T("subadd")			},
	{	cmdSubUpdate,		_T("subupdate")			},
	{	cmdSubSync,			_T("subsync")			},
	{	cmdRefLog,			_T("reflog")			},
	{	cmdRefBrowse,		_T("refbrowse")			},
	{	cmdSVNDCommit,		_T("svndcommit")		},
	{	cmdSVNRebase,		_T("svnrebase")			},
	{	cmdSVNFetch,		_T("svnfetch")			},
	{	cmdSVNIgnore,		_T("svnignore")			},
	{	cmdSync,			_T("sync")				},
	{	cmdRequestPull,		_T("requestpull")		},
	{	cmdBisect,			_T("bisect")			},
	{	cmdRepoBrowser,		_T("repobrowser")		},
};


Command * CommandServer::GetCommand(const CString& sCmd)
{
	// Look up the command
	TGitCommand command = cmdAbout;		// Something harmless as a default
	for (int nCommand = 0; nCommand < _countof(commandInfo); nCommand++)
	{
		if (sCmd.Compare(commandInfo[nCommand].pCommandName) == 0)
		{
			// We've found the command
			command = commandInfo[nCommand].command;
			// If this fires, you've let the enum get out of sync with the commandInfo array
			ASSERT((int)command == nCommand);
			break;
		}
	}

	// CBrowseRefsDlg dialog
	switch (command)
	{
	case cmdAbout:
		return new AboutCommand;
	case cmdCommit:
		return new CommitCommand;
	case cmdLog:
		return new LogCommand;
	case cmdRepoCreate:
		return new CreateRepositoryCommand;
	case cmdClone:
		return new CloneCommand;
	case cmdPrevDiff:
		return new PrevDiffCommand;
	case cmdDiff:
		return new DiffCommand;
	case cmdRename:
		return new RenameCommand;
	case cmdRepoStatus:
		return new RepoStatusCommand;
	case cmdRemove:
		return new RemoveCommand;
	case cmdRevert:
		return new RevertCommand;
	case cmdPull:
		return new PullCommand;
	case cmdFetch:
		return new FetchCommand;
	case cmdPush:
		return new PushCommand;
	case cmdBranch:
		return new BranchCommand;
	case cmdTag:
		return new TagCommand;
	case cmdMerge:
		return new MergeCommand;
	case cmdSwitch:
		return new SwitchCommand;
	case cmdExport:
		return new ExportCommand;
	case cmdAdd:
		return new AddCommand;
	case cmdIgnore:
		return new IgnoreCommand;
	case cmdFormatPatch:
		return new FormatPatchCommand;
	case cmdImportPatch:
		return new ImportPatchCommand;
	case cmdBlame:
		return new BlameCommand;
	case cmdSettings:
		return new SettingsCommand;
	case cmdConflictEditor:
		return new ConflictEditorCommand;
	case cmdCleanup:
		return new CleanupCommand;
	case cmdRebase:
		return new RebaseCommand;
	case cmdResolve:
		return new ResolveCommand;
	case cmdDropMove:
		return new DropMoveCommand;
	case cmdDropCopy:
		return new DropCopyCommand;
	case cmdDropCopyAdd:
		return new DropCopyAddCommand;
	case cmdHelp:
		return new HelpCommand;
	case cmdStashSave:
		return new StashSaveCommand;
	case cmdStashApply:
		return new StashApplyCommand;
	case cmdStashPop:
		return new StashPopCommand;
	case cmdSubAdd:
		return new SubmoduleAddCommand;
	case cmdSubUpdate:
		return new SubmoduleUpdateCommand;
	case cmdRefLog:
		return new RefLogCommand;
	case cmdSubSync:
		return new SubmoduleSyncCommand;
	case cmdSendMail:
		return new SendMailCommand;
	case cmdCat:
		return new CatCommand;
	case cmdRefBrowse:
		return new RefBrowseCommand;
	case cmdSVNDCommit:
		return new SVNDCommitCommand;
	case cmdSVNRebase:
		return new SVNRebaseCommand;
	case cmdSVNFetch:
		return new SVNFetchCommand;
	case cmdSync:
		return new SyncCommand;
	case cmdRequestPull:
		return new RequestPullCommand;
	case cmdUpdateCheck:
		return new UpdateCheckCommand;
	case cmdPasteCopy:
		return new PasteCopyCommand;
	case cmdPasteMove:
		return new PasteMoveCommand;
	case cmdSVNIgnore:
		return new SVNIgnoreCommand;
	case cmdBisect:
		return new BisectCommand;
	case cmdRepoBrowser:
		return new RepositoryBrowserCommand;

#if 0



	case cmdCopy:
		return new CopyCommand;
	case cmdCrash:
		return new CrashCommand;



	case cmdPrevDiff:
		return new PrevDiffCommand;
	case cmdProperties:
		return new PropertiesCommand;
	case cmdRTFM:
		return new RTFMCommand;
	case cmdRebuildIconCache:
		return new RebuildIconCacheCommand;
	case cmdShowCompare:
		return new ShowCompareCommand;
	case cmdUnIgnore:
		return new UnIgnoreCommand;
#endif
	default:
		CMessageBox::Show(hWndExplorer, _T("Command not implemented"), _T("TortoiseGit"), MB_ICONERROR);
		return new AboutCommand;
	}
}
