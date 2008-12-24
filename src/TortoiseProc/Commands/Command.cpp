// TortoiseSVN - a Windows shell extension for easy version control

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

#if 0
#include "AddCommand.h"
#include "BlameCommand.h"
#include "CatCommand.h"
#include "CheckoutCommand.h"
#include "CleanupCommand.h"

#include "ConflictEditorCommand.h"
#include "CopyCommand.h"
#include "CrashCommand.h"
#include "CreatePatchCommand.h"

#include "DelUnversionedCommand.h"

#include "DropCopyAddCommand.h"
#include "DropCopyCommand.h"
#include "DropExportCommand.h"
#include "DropMoveCommand.h"

#include "HelpCommand.h"
#include "IgnoreCommand.h"
#include "ImportCommand.h"
#include "LockCommand.h"


#include "MergeAllCommand.h"
#include "PasteCopyCommand.h"
#include "PasteMoveCommand.h"

#include "PropertiesCommand.h"
#include "RebuildIconCacheCommand.h"
#include "RelocateCommand.h"
#include "RemoveCommand.h"

#include "RepositoryBrowserCommand.h"

#include "ResolveCommand.h"
#include "RevertCommand.h"
#include "RevisiongraphCommand.h"
#include "RTFMCommand.h"
#include "SettingsCommand.h"
#include "ShowCompareCommand.h"

#include "UnIgnoreCommand.h"
#include "UnLockCommand.h"
#include "UpdateCheckCommand.h"
#include "UpdateCommand.h"
#include "UrlDiffCommand.h"
#endif
typedef enum
{
	cmdAbout,
	cmdAdd,
	cmdBlame,
	cmdBranch,
	cmdCat,
	cmdCheckout,
	cmdCleanup,
	cmdClone,
	cmdCommit,
	cmdConflictEditor,
	cmdCopy,
	cmdCrash,
	cmdCreatePatch,
	cmdDelUnversioned,
	cmdDiff,
	cmdDropCopy,
	cmdDropCopyAdd,
	cmdDropExport,
	cmdDropMove,
	cmdFetch,
	cmdExport,
	cmdHelp,
	cmdIgnore,
	cmdImport,
	cmdLock,
	cmdLog,
	cmdMerge,
	cmdMergeAll,
	cmdPasteCopy,
	cmdPasteMove,
	cmdPrevDiff,
	cmdProperties,
	cmdPull,
	cmdPush,
	cmdRTFM,
	cmdRebuildIconCache,
	cmdRelocate,
	cmdRemove,
	cmdRename,
	cmdRepoBrowser,
	cmdRepoCreate,
	cmdRepoStatus,
	cmdResolve,
	cmdRevert,
	cmdRevisionGraph,
	cmdSettings,
	cmdShowCompare,
	cmdSwitch,
	cmdTag,
	cmdUnIgnore,
	cmdUnlock,
	cmdUpdate,
	cmdUpdateCheck,
	cmdUrlDiff,
	
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
	{	cmdCheckout,		_T("checkout")			},
	{	cmdCleanup,			_T("cleanup")			},
	{	cmdClone,			_T("clone")				},
	{	cmdCommit,			_T("commit")			},
	{	cmdConflictEditor,	_T("conflicteditor")	},
	{	cmdCopy,			_T("copy")				},
	{	cmdCrash,			_T("crash")				},
	{	cmdCreatePatch,		_T("createpatch")		},
	{	cmdDelUnversioned,	_T("delunversioned")	},
	{	cmdDiff,			_T("diff")				},
	{	cmdDropCopy,		_T("dropcopy")			},
	{	cmdDropCopyAdd,		_T("dropcopyadd")		},
	{	cmdDropExport,		_T("dropexport")		},
	{	cmdDropMove,		_T("dropmove")			},
	{	cmdFetch,			_T("fetch")				},
	{	cmdExport,			_T("export")			},
	{	cmdHelp,			_T("help")				},
	{	cmdIgnore,			_T("ignore")			},
	{	cmdImport,			_T("import")			},
	{	cmdLock,			_T("lock")				},
	{	cmdLog,				_T("log")				},
	{	cmdMerge,			_T("merge")				},
	{	cmdMergeAll,		_T("mergeall")			},
	{	cmdPasteCopy,		_T("pastecopy")			},
	{	cmdPasteMove,		_T("pastemove")			},
	{	cmdPrevDiff,		_T("prevdiff")			},
	{	cmdProperties,		_T("properties")		},
	{	cmdPull,			_T("pull")				},
	{	cmdPush,			_T("push")				},
	{	cmdRTFM,			_T("rtfm")				},
	{	cmdRebuildIconCache,_T("rebuildiconcache")	},
	{	cmdRelocate,		_T("relocate")			},
	{	cmdRemove,			_T("remove")			},
	{	cmdRename,			_T("rename")			},
	{	cmdRepoBrowser,		_T("repobrowser")		},
	{	cmdRepoCreate,		_T("repocreate")		},
	{	cmdRepoStatus,		_T("repostatus")		},
	{	cmdResolve,			_T("resolve")			},
	{	cmdRevert,			_T("revert")			},
	{	cmdRevisionGraph,	_T("revisiongraph")		},
	{	cmdSettings,		_T("settings")			},
	{	cmdShowCompare,		_T("showcompare")		},
	{	cmdSwitch,			_T("switch")			},
	{	cmdTag,				_T("tag")				},
	{	cmdUnIgnore,		_T("unignore")			},
	{	cmdUnlock,			_T("unlock")			},
	{	cmdUpdate,			_T("update")			},
	{	cmdUpdateCheck,		_T("updatecheck")		},
	{	cmdUrlDiff,			_T("urldiff")			},
};




Command * CommandServer::GetCommand(const CString& sCmd)
{
	// Look up the command
	TGitCommand command = cmdAbout;		// Something harmless as a default
	for (int nCommand = 0; nCommand < (sizeof(commandInfo)/sizeof(commandInfo[0])); nCommand++)
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
#if 0
	case cmdAdd:
		return new AddCommand;
	case cmdBlame:
		return new BlameCommand;
	case cmdCat:
		return new CatCommand;
	case cmdCheckout:
		return new CheckoutCommand;
	case cmdCleanup:
		return new CleanupCommand;

	case cmdConflictEditor:
		return new ConflictEditorCommand;
	case cmdCopy:
		return new CopyCommand;
	case cmdCrash:
		return new CrashCommand;
	case cmdCreatePatch:
		return new CreatePatchCommand;
	case cmdDelUnversioned:
		return new DelUnversionedCommand;

	case cmdDropCopy:
		return new DropCopyCommand;
	case cmdDropCopyAdd:
		return new DropCopyAddCommand;
	case cmdDropExport:
		return new DropExportCommand;
	case cmdDropMove:
		return new DropMoveCommand;

	case cmdHelp:
		return new HelpCommand;
	case cmdIgnore:
		return new IgnoreCommand;
	case cmdImport:
		return new ImportCommand;
	case cmdLock:
		return new LockCommand;

	case cmdMerge:
		return new MergeCommand;
	case cmdMergeAll:
		return new MergeAllCommand;
	case cmdPasteCopy:
		return new PasteCopyCommand;
	case cmdPasteMove:
		return new PasteMoveCommand;
	case cmdPrevDiff:
		return new PrevDiffCommand;
	case cmdProperties:
		return new PropertiesCommand;
	case cmdRTFM:
		return new RTFMCommand;
	case cmdRebuildIconCache:
		return new RebuildIconCacheCommand;
	case cmdRelocate:
		return new RelocateCommand;
	case cmdRemove:
		return new RemoveCommand;

	case cmdRepoBrowser:
		return new RepositoryBrowserCommand;


	case cmdResolve:
		return new ResolveCommand;
	case cmdRevert:
		return new RevertCommand;
	case cmdRevisionGraph:
		return new RevisionGraphCommand;
	case cmdSettings:
		return new SettingsCommand;
	case cmdShowCompare:
		return new ShowCompareCommand;

	case cmdUnIgnore:
		return new UnIgnoreCommand;
	case cmdUnlock:
		return new UnLockCommand;
	case cmdUpdate:
		return new UpdateCommand;
	case cmdUpdateCheck:
		return new UpdateCheckCommand;
	case cmdUrlDiff:
		return new UrlDiffCommand;
#endif
	default:
		CMessageBox::Show(hWndExplorer, _T("Have not implemented"), _T("TortoiseGit"), MB_ICONERROR);
		return new AboutCommand;
	}
}
