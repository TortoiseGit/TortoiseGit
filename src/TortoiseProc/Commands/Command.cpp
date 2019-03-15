// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2007-2009 - TortoiseSVN

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
#include "Command.h"
#include "MessageBox.h"
#include "AboutCommand.h"
#include "AutoTextTestCommand.h"
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
#include "RevisiongraphCommand.h"
#include "ShowCompareCommand.h"
#include "DaemonCommand.h"
#include "CommitIsOnRefsCommand.h"
#include "RTFMCommand.h"

#if 0
#include "CrashCommand.h"
#include "RebuildIconCacheCommand.h"
#include "RemoveCommand.h"
#include "UnIgnoreCommand.h"
#endif

typedef enum
{
	cmdAbout,
	cmdAdd,
	cmdAutoTextTest,
	cmdBlame,
	cmdBranch,
	cmdCat,
	cmdCleanup,
	cmdClone,
	cmdCommit,
	cmdConflictEditor,
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
	cmdRevisionGraph,
	cmdDaemon,
	cmdPGPFP,
	cmdCommitIsOnRefs,
} TGitCommand;

static const struct CommandInfo
{
	TGitCommand command;
	LPCTSTR pCommandName;
} commandInfo[] =
{
	{	cmdAbout,			L"about"			},
	{	cmdAdd,				L"add"				},
	{	cmdAutoTextTest,	L"autotexttest"		},
	{	cmdBlame,			L"blame"			},
	{	cmdBranch,			L"branch"			},
	{	cmdCat,				L"cat"				},
	{	cmdCleanup,			L"cleanup"			},
	{	cmdClone,			L"clone"			},
	{	cmdCommit,			L"commit"			},
	{	cmdConflictEditor,	L"conflicteditor"	},
	{	cmdCrash,			L"crash"			},
	{	cmdDiff,			L"diff"				},
	{	cmdDropCopy,		L"dropcopy"			},
	{	cmdDropCopyAdd,		L"dropcopyadd"		},
	{	cmdDropMove,		L"dropmove"			},
	{	cmdFetch,			L"fetch"			},
	{	cmdFormatPatch,		L"formatpatch"		},
	{	cmdExport,			L"export"			},
	{	cmdHelp,			L"help"				},
	{	cmdIgnore,			L"ignore"			},
	{	cmdImportPatch,		L"importpatch"		},
	{	cmdLog,				L"log"				},
	{	cmdMerge,			L"merge"			},
	{	cmdPasteCopy,		L"pastecopy"		},
	{	cmdPasteMove,		L"pastemove"		},
	{	cmdPrevDiff,		L"prevdiff"			},
	{	cmdPull,			L"pull"				},
	{	cmdPush,			L"push"				},
	{	cmdRTFM,			L"rtfm"				},
	{	cmdRebuildIconCache,L"rebuildiconcache"	},
	{	cmdRemove,			L"remove"			},
	{	cmdRebase,			L"rebase"			},
	{	cmdRename,			L"rename"			},
	{	cmdRepoCreate,		L"repocreate"		},
	{	cmdRepoStatus,		L"repostatus"		},
	{	cmdResolve,			L"resolve"			},
	{	cmdRevert,			L"revert"			},
	{	cmdSendMail,		L"sendmail"			},
	{	cmdSettings,		L"settings"			},
	{	cmdShowCompare,		L"showcompare"		},
	{	cmdSwitch,			L"switch"			},
	{	cmdTag,				L"tag"				},
	{	cmdUnIgnore,		L"unignore"			},
	{	cmdUpdateCheck,		L"updatecheck"		},
	{	cmdStashSave,		L"stashsave"		},
	{	cmdStashApply,		L"stashapply"		},
	{	cmdStashPop,		L"stashpop"			},
	{	cmdStashList,		L"stashlist"		},
	{	cmdSubAdd,			L"subadd"			},
	{	cmdSubUpdate,		L"subupdate"		},
	{	cmdSubSync,			L"subsync"			},
	{	cmdRefLog,			L"reflog"			},
	{	cmdRefBrowse,		L"refbrowse"		},
	{	cmdSVNDCommit,		L"svndcommit"		},
	{	cmdSVNRebase,		L"svnrebase"		},
	{	cmdSVNFetch,		L"svnfetch"			},
	{	cmdSVNIgnore,		L"svnignore"		},
	{	cmdSync,			L"sync"				},
	{	cmdRequestPull,		L"requestpull"		},
	{	cmdBisect,			L"bisect"			},
	{	cmdRepoBrowser,		L"repobrowser"		},
	{	cmdRevisionGraph,	L"revisiongraph"	},
	{	cmdDaemon,			L"daemon"			},
	{	cmdPGPFP,			L"pgpfp"			},
	{	cmdCommitIsOnRefs,	L"commitisonrefs"	},
};


Command * CommandServer::GetCommand(const CString& sCmd)
{
	// Look up the command
	TGitCommand command = cmdAbout;		// Something harmless as a default
	for (int nCommand = 0; nCommand < _countof(commandInfo); ++nCommand)
	{
		if (sCmd.Compare(commandInfo[nCommand].pCommandName) == 0)
		{
			// We've found the command
			command = commandInfo[nCommand].command;
			// If this fires, you've let the enum get out of sync with the commandInfo array
			ASSERT(static_cast<int>(command) == nCommand);
			break;
		}
	}

	// CBrowseRefsDlg dialog
	switch (command)
	{
	case cmdAbout:
		return new AboutCommand;
	case cmdAutoTextTest:
		return new AutoTextTestCommand;
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
#if 0
	case cmdDropCopy:
		return new DropCopyCommand;
#endif
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
#if 0
	case cmdPasteCopy:
		return new PasteCopyCommand;
	case cmdPasteMove:
		return new PasteMoveCommand;
#endif
	case cmdSVNIgnore:
		return new SVNIgnoreCommand;
	case cmdBisect:
		return new BisectCommand;
	case cmdRepoBrowser:
		return new RepositoryBrowserCommand;
	case cmdRevisionGraph:
		return new RevisionGraphCommand;
	case cmdShowCompare:
		return new ShowCompareCommand;
	case cmdDaemon:
		return new DaemonCommand;
	case cmdCommitIsOnRefs:
		return new CommitIsOnRefsCommand;
	case cmdRTFM:
		return new RTFMCommand;
#if 0
	case cmdCrash:
		return new CrashCommand;
	case cmdRebuildIconCache:
		return new RebuildIconCacheCommand;
	case cmdUnIgnore:
		return new UnIgnoreCommand;
#endif
	case cmdPGPFP:
		{
			CMessageBox::Show(GetExplorerHWND(), L"This is the fingerprint of the TortoiseGit Release Signing Key.\nIt can be used to establish a trust path from this release to another one.\n\nTortoiseGit Release Signing Key, 4096-bit RSA:\n74A2 1AE3 01B3 CA5B D807  2F5E F7F1 7B3F 9DD9 539E", L"TortoiseGit", MB_OK);
			return nullptr;
		}
	default:
		CMessageBox::Show(GetExplorerHWND(), L"Command not implemented", L"TortoiseGit", MB_ICONERROR);
		return new AboutCommand;
	}
}
