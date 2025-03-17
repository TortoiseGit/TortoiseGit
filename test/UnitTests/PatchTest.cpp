// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018-2019, 2021-2022, 2025 - TortoiseGit

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
#include "RepositoryFixtures.h"
#include "Patch.h"

TEST(CPatch, Empty)
{
	CPatch patch;
	EXPECT_EQ(0, patch.GetNumberOfFiles());
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_STREQ(L"", patch.GetRevision(0));
	EXPECT_STREQ(L"", patch.GetRevision2(0));
	EXPECT_STREQ(L"", patch.GetRevision(1));
	EXPECT_STREQ(L"", patch.GetFilename(0));
	EXPECT_STREQ(L"", patch.GetFilename2(0));
}

TEST(CPatch, Parse_Manual)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\manual.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(1, patch.GetNumberOfFiles());

	EXPECT_STREQ(L"src\\Git\\GitPatch.cpp", patch.GetFilename(0));
	EXPECT_STREQ(L"src\\Git\\GitPatch.cpp", patch.GetFilename2(0));
	EXPECT_STREQ(L"some path\\prefix\\src\\Git\\GitPatch.cpp", patch.GetFullPath(L"some path\\prefix", 0, 0));
	EXPECT_STREQ(L"some path\\prefix\\src\\Git\\GitPatch.cpp", patch.GetFullPath(L"some path\\prefix\\", 0, 0));
	EXPECT_STREQ(L"some path\\prefix\\src\\Git\\GitPatch.cpp", patch.GetFullPath(L"some path\\prefix", 0, 1));
	EXPECT_STREQ(L"some path\\prefix\\src\\Git\\GitPatch.cpp", patch.GetFullPath(L"some path\\prefix\\", 0, 1));
	EXPECT_STREQ(L"", patch.GetRevision(0));
	EXPECT_STREQ(L"", patch.GetRevision2(0));

	// internals
	auto& chunks = patch.GetChunks(0);
	ASSERT_EQ(size_t(2), chunks.size());
	{
		auto chunk = chunks[0].get();
		EXPECT_EQ(5, chunk->arLines.GetCount());
		EXPECT_EQ(4, chunk->lAddLength);
		EXPECT_EQ(4, chunk->lRemoveLength);
		EXPECT_EQ(1, chunk->lAddStart);
		EXPECT_EQ(1, chunk->lRemoveStart);
		ASSERT_EQ(5, chunk->arLinesStates.GetCount());
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_REMOVED), chunk->arLinesStates.GetAt(0));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(1));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(2));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(3));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(4));
	}
	{
		auto chunk = chunks[1].get();
		EXPECT_EQ(8, chunk->arLines.GetCount());
		EXPECT_EQ(7, chunk->lAddLength);
		EXPECT_EQ(7, chunk->lRemoveLength);
		EXPECT_EQ(155, chunk->lAddStart);
		EXPECT_EQ(155, chunk->lRemoveStart);
		ASSERT_EQ(8, chunk->arLinesStates.GetCount());
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(0));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(1));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(2));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_REMOVED), chunk->arLinesStates.GetAt(3));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(4));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(5));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(6));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(7));
	}
}

TEST(CPatch, Parse_OldGitDiffPatch)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\old-git-diff.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(4, patch.GetNumberOfFiles());

	EXPECT_STREQ(L"doc\\doc.build", patch.GetFilename(0));
	EXPECT_STREQ(L"doc\\doc.build", patch.GetFilename2(0));
	EXPECT_STREQ(L"36d19f2", patch.GetRevision(0));
	EXPECT_STREQ(L"365406a", patch.GetRevision2(0));

	EXPECT_STREQ(L"doc\\doc.build.include", patch.GetFilename(1));
	EXPECT_STREQ(L"doc\\doc.build.include", patch.GetFilename2(1));
	EXPECT_STREQ(L"69bd153", patch.GetRevision(1));
	EXPECT_STREQ(L"ca2f366", patch.GetRevision2(1));

	EXPECT_STREQ(L"doc\\source\\styles_chm.css", patch.GetFilename(2));
	EXPECT_STREQ(L"doc\\source\\styles_chm.css", patch.GetFilename2(2));
	EXPECT_STREQ(L"607887c", patch.GetRevision(2));
	EXPECT_STREQ(L"f87bca1", patch.GetRevision2(2));

	EXPECT_STREQ(L"doc\\source\\styles_html.css", patch.GetFilename(3));
	EXPECT_STREQ(L"doc\\source\\styles_html.css", patch.GetFilename2(3));
	EXPECT_STREQ(L"b2127f7", patch.GetRevision(3));
	EXPECT_STREQ(L"d41e7c3", patch.GetRevision2(3));
}

TEST(CPatch, Parse_GitDiffPatch)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\git-diff.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(6, patch.GetNumberOfFiles());

	EXPECT_STREQ(L"appveyor.yml", patch.GetFilename(0));
	EXPECT_STREQ(L"NUL", patch.GetFilename2(0));
	EXPECT_STREQ(L"e6b688767", patch.GetRevision(0));
	EXPECT_STREQ(L"", patch.GetRevision2(0));

	EXPECT_STREQ(L"src\\Git\\GitPatch.cpp", patch.GetFilename(1));
	EXPECT_STREQ(L"src\\Git\\GitPatch.cpp", patch.GetFilename2(1));
	EXPECT_STREQ(L"c85b034cd", patch.GetRevision(1));
	EXPECT_STREQ(L"9f99b1cc4", patch.GetRevision2(1));

	EXPECT_STREQ(L"NUL", patch.GetFilename(5));
	EXPECT_STREQ(L"wusel dusel.txt", patch.GetFilename2(5));
	EXPECT_STREQ(L"NUL", patch.GetFullPath(L"some path\\prefix", 5, 0));
	EXPECT_STREQ(L"some path\\prefix\\wusel dusel.txt", patch.GetFullPath(L"some path\\prefix", 5, 1));
	EXPECT_STREQ(L"some path\\prefix\\wusel dusel.txt", patch.GetFullPath(L"some path\\prefix\\", 5, 1));
	EXPECT_STREQ(L"000000000", patch.GetRevision(5));
	EXPECT_STREQ(L"", patch.GetRevision2(5));

	// internals
	{
		// deleted file
		auto& chunks = patch.GetChunks(0);
		ASSERT_EQ(size_t(1), chunks.size());
		auto chunk = chunks[0].get();
		EXPECT_EQ(30, chunk->arLines.GetCount());
		EXPECT_EQ(0, chunk->lAddLength);
		EXPECT_EQ(30, chunk->lRemoveLength);
		EXPECT_EQ(0, chunk->lAddStart);
		EXPECT_EQ(1, chunk->lRemoveStart);
		ASSERT_EQ(30, chunk->arLinesStates.GetCount());
		for (int i = 0; i < 30; ++i)
			ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_REMOVED), chunk->arLinesStates.GetAt(i));
	}
	{
		auto& chunks = patch.GetChunks(2);
		ASSERT_EQ(size_t(3), chunks.size());
		auto chunk = chunks[1].get();
		EXPECT_EQ(9, chunk->arLines.GetCount());
		EXPECT_EQ(9, chunk->lAddLength);
		EXPECT_EQ(7, chunk->lRemoveLength);
		EXPECT_EQ(402, chunk->lAddStart);
		EXPECT_EQ(400, chunk->lRemoveStart);
		ASSERT_EQ(9, chunk->arLinesStates.GetCount());
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(0));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(1));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(2));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(3));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(4));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(5));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(6));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(7));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_CONTEXT), chunk->arLinesStates.GetAt(8));
	}
	{
		// new file
		auto& chunks = patch.GetChunks(5);
		ASSERT_EQ(size_t(1), chunks.size());
		auto chunk = chunks[0].get();
		EXPECT_EQ(5, chunk->arLines.GetCount());
		EXPECT_EQ(5, chunk->lAddLength);
		EXPECT_EQ(0, chunk->lRemoveLength);
		EXPECT_EQ(1, chunk->lAddStart);
		EXPECT_EQ(0, chunk->lRemoveStart);
		ASSERT_EQ(5, chunk->arLinesStates.GetCount());
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(0));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(1));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(2));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(3));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(4));
	}
}

TEST(CPatch, Parse_GitFormatPatchNoPrefix)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\git-format-patch-no-prefix.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(1, patch.GetNumberOfFiles());

	EXPECT_STREQ(L"src\\TortoiseProc\\AppUtils.cpp", patch.GetFilename(0));
	EXPECT_STREQ(L"fe40643e9", patch.GetRevision(0));
	EXPECT_STREQ(L"f74cfec7e", patch.GetRevision2(0));
}

TEST(CPatch, Parse_SVNDiffPatch)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\SVN-diff.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(2, patch.GetNumberOfFiles());

	EXPECT_STREQ(L"new file.txt", patch.GetFilename(0));
	EXPECT_STREQ(L"", patch.GetRevision(0));
	EXPECT_STREQ(L"", patch.GetRevision2(0));

	EXPECT_STREQ(L"Utils\\StringUtils.cpp", patch.GetFilename(1));
	EXPECT_STREQ(L"", patch.GetRevision(1));
	EXPECT_STREQ(L"", patch.GetRevision2(1));

	// internals
	{
		// new file
		auto& chunks = patch.GetChunks(0);
		ASSERT_EQ(size_t(1), chunks.size());
		auto chunk = chunks[0].get();
		EXPECT_EQ(5, chunk->arLines.GetCount());
		EXPECT_EQ(5, chunk->lAddLength);
		EXPECT_EQ(0, chunk->lRemoveLength);
		EXPECT_EQ(1, chunk->lAddStart);
		EXPECT_EQ(0, chunk->lRemoveStart);
		ASSERT_EQ(5, chunk->arLinesStates.GetCount());
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(0));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(1));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(2));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(3));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(4));
	}
}

TEST(CPatch, Parse_GitDiffPatch_SingleLineChange)
{
	// cf. issue #3420, file-with-crlf-ko
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\git-diff-single-line-change.patch"));
	ASSERT_STREQ(L"", patch.GetErrorMessage());
	ASSERT_EQ(1, patch.GetNumberOfFiles());

	EXPECT_STREQ(L"gradle.properties", patch.GetFilename(0));
	EXPECT_STREQ(L"gradle.properties", patch.GetFilename2(0));
	EXPECT_STREQ(L"3d04df0", patch.GetRevision(0));
	EXPECT_STREQ(L"c335456", patch.GetRevision2(0));

	// internals
	{
		auto& chunks = patch.GetChunks(0);
		ASSERT_EQ(size_t(1), chunks.size());
		auto chunk = chunks[0].get();
		EXPECT_EQ(2, chunk->arLines.GetCount());
		EXPECT_EQ(1, chunk->lAddLength);
		EXPECT_EQ(1, chunk->lRemoveLength);
		EXPECT_EQ(1, chunk->lAddStart);
		EXPECT_EQ(0, chunk->lRemoveStart);
		ASSERT_EQ(2, chunk->arLinesStates.GetCount());
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_REMOVED), chunk->arLinesStates.GetAt(0));
		ASSERT_EQ(static_cast<DWORD>(PATCHSTATE_ADDED), chunk->arLinesStates.GetAt(1));
	}
}

TEST(CPatch, Parse_QuotedFilename)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\quoted-filename.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(1, patch.GetNumberOfFiles());

	EXPECT_STREQ(L"ödp.txt", patch.GetFilename(0));
	EXPECT_STREQ(L"ödp.txt", patch.GetFilename2(0));
	EXPECT_STREQ(L"279294a", patch.GetRevision(0));
	EXPECT_STREQ(L"1c7369e", patch.GetRevision2(0));

	// internals
	{
		// changed file
		auto& chunks = patch.GetChunks(0);
		ASSERT_EQ(size_t(1), chunks.size());
		auto chunk = chunks[0].get();
		EXPECT_EQ(8, chunk->arLines.GetCount());
		EXPECT_EQ(6, chunk->lAddLength);
		EXPECT_EQ(6, chunk->lRemoveLength);
		EXPECT_EQ(1, chunk->lAddStart);
		EXPECT_EQ(1, chunk->lRemoveStart);
	}
}

TEST(CPatch, Parse_UnquotedFilenameUTF8)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\patch-utf8.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(1, patch.GetNumberOfFiles());

	EXPECT_STREQ(L"ödp.txt", patch.GetFilename(0));
	EXPECT_STREQ(L"ödp.txt", patch.GetFilename2(0));
	EXPECT_STREQ(L"279294a", patch.GetRevision(0));
	EXPECT_STREQ(L"1c7369e", patch.GetRevision2(0));

	// internals
	{
		// changed file
		auto& chunks = patch.GetChunks(0);
		ASSERT_EQ(size_t(1), chunks.size());
		auto chunk = chunks[0].get();
		EXPECT_EQ(8, chunk->arLines.GetCount());
		EXPECT_EQ(6, chunk->lAddLength);
		EXPECT_EQ(6, chunk->lRemoveLength);
		EXPECT_EQ(1, chunk->lAddStart);
		EXPECT_EQ(1, chunk->lRemoveStart);
	}
}

TEST(CPatch, PatchFile)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CAutoTempDir tempDir;
	ASSERT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\input", nullptr));
	ASSERT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\input\\src", nullptr));
	ASSERT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\input\\src\\TortoiseMerge", nullptr));
	ASSERT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\output", nullptr));
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tempDir.GetTempDir() + L"\\input\\appveyor.yml", L"version: '{branch}.{build}'\nskip_tags: true\nimage: Visual Studio 2017\ninit:\n- git version\nbuild_script:\n- git submodule update --init -- ext/googletest ext/libgit2 ext/simpleini ext/tgit ext/zlib\n- cd ext\\libgit2\n- git config --global user.email \"dummy@example.com\"\n- git config --global user.name \"Dummy Name\"\n- for %%G in (..\\libgit2-*.patch) do ( type %%G | git am )\n- git config --unset --global user.email\n- git config --unset --global user.name\n- cd ..\\..\n- msbuild \"src\\TortoiseGit.sln\" /t:\"test\\UnitTests\" /m /verbosity:minimal /p:Configuration=Debug /p:PlatformToolset=v141 /p:Platform=x64 /maxcpucount /logger:\"C:\\Program Files\\AppVeyor\\BuildAgent\\Appveyor.MSBuildLogger.dll\"\n- msbuild \"src\\TortoiseGit.sln\" /t:\"test\\UnitTests\" /m /verbosity:minimal /p:Configuration=Debug /p:PlatformToolset=v141 /p:Platform=Win32 /maxcpucount /logger:\"C:\\Program Files\\AppVeyor\\BuildAgent\\Appveyor.MSBuildLogger.dll\"\ntest_script:\n- bin\\Debug\\bin\\tests.exe\n- bin\\Debug64\\bin\\tests.exe\n- reg add HKCU\\Software\\TortoiseGit /v CygwinHack /t REG_DWORD /f /d 1\n- reg add HKCU\\Software\\TortoiseGit /v MSysGit /t REG_SZ /f /d \"c:\\cygwin\\bin\"\n- set HOME=%USERPROFILE%\n- c:\\cygwin\\bin\\git version\n- bin\\Debug\\bin\\tests.exe\n- bin\\Debug64\\bin\\tests.exe\n- reg delete HKCU\\Software\\TortoiseGit /v CygwinHack /f\n- reg delete HKCU\\Software\\TortoiseGit /v MSysGit /f\n- msbuild \"src\\TortoiseGit.sln\" /t:\"GitWCRev\" /t:\"GitWCRevCom\" /t:\"TortoiseGitSetup\\CustomActions\" /t:\"TortoiseGitSetup\\RestartExplorer\" /t:\"ext\\Crash-Server\\CrashServerSDK\\CrashHandler\" /t:\"ext\\Crash-Server\\CrashServerSDK\\SendRpt\" /m /verbosity:minimal /p:Configuration=Release /p:Platform=x64 /maxcpucount /p:PlatformToolset=v141 /logger:\"C:\\Program Files\\AppVeyor\\BuildAgent\\Appveyor.MSBuildLogger.dll\"\n- git submodule update --init -- ext/apr ext/apr-util ext/editorconfig ext/pcre\n- msbuild \"src\\TortoiseGit.sln\" /t:\"TGitCache\" /t:\"TortoiseGitBlame\" /t:\"TortoiseGitIDiff\" /t:\"TortoiseGitMerge\" /t:\"TortoiseGitPlink\" /t:\"TortoiseGitProc\" /t:\"TortoiseGitStub\" /t:\"TortoiseGitUDiff\" /t:\"TortoiseShell\" /t:\"SshAskPass\" /t:\"tgittouch\" /t:\"GitWCRev\" /t:\"GitWCRevCom\" /m /verbosity:minimal /p:Configuration=Debug /p:Platform=x64 /maxcpucount /p:PlatformToolset=v141 /logger:\"C:\\Program Files\\AppVeyor\\BuildAgent\\Appveyor.MSBuildLogger.dll\"", true));
	ASSERT_TRUE(CopyFile(resourceDir + L"\\patches\\git-diff-Patch.cpp", tempDir.GetTempDir() + L"\\input\\src\\TortoiseMerge\\Patch.cpp", true));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\git-diff.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(6, patch.GetNumberOfFiles());

	// delete file
	{
		EXPECT_TRUE(patch.PatchFile(0, 0, tempDir.GetTempDir() + L"\\input", tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename(0), L"", false)); // use old filename here on purpose as NUL is not allowed
		struct _stat64 stat = { 0 };
		EXPECT_EQ(0, _wstat64(tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename(0), &stat));
		EXPECT_EQ(0, stat.st_size);
	}

	// add file
	{
		EXPECT_TRUE(patch.PatchFile(0, 5, tempDir.GetTempDir() + L"\\input", tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(5), L"", false));
		CString text;
		EXPECT_EQ(TRUE, CStringUtils::ReadStringFromTextFile(tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(5), text));
		EXPECT_STREQ(L"new file\r\ndfkdsf#dsf\r\n\r\n\r\ndsf\r\n", text);
		git_oid oid2 = { 0 };
		EXPECT_EQ(0, git_odb_hashfile(&oid2, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(5)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
		CGitHash hashAfter = oid2;
		EXPECT_STREQ(patch.GetRevision2(5), hashAfter.ToString(patch.GetRevision2(5).GetLength()));
	}

	// modify file
	{
		git_oid oid1 = { 0 };
		EXPECT_EQ(0, git_odb_hashfile(&oid1, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\input\\" + patch.GetFilename(2)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
		CGitHash hashBefore = oid1;
		EXPECT_STREQ(patch.GetRevision(2), hashBefore.ToString(patch.GetRevision(2).GetLength()));
		EXPECT_TRUE(patch.PatchFile(0, 2, tempDir.GetTempDir() + L"\\input", tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(2), L"", false));
		git_oid oid2 = { 0 };
		EXPECT_EQ(0, git_odb_hashfile(&oid2, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(2)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
		CGitHash hashAfter = oid2;
		EXPECT_STREQ(patch.GetRevision2(2), hashAfter.ToString(patch.GetRevision2(2).GetLength()));
	}
}

TEST(CPatch, PatchFile_UTF8)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CAutoTempDir tempDir;
	ASSERT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\input", nullptr));
	ASSERT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\output", nullptr));
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tempDir.GetTempDir() + L"\\input\\ödp.txt", L"dsfds\r\nä\r\ndsf\r\nädsf\r\ndsf\r\nsdf", true));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\patch-utf8.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(1, patch.GetNumberOfFiles());

	git_oid oid1 = { 0 };
	EXPECT_EQ(0, git_odb_hashfile(&oid1, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\input\\" + patch.GetFilename(0)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
	CGitHash hashBefore = oid1;
	EXPECT_STREQ(patch.GetRevision(0), hashBefore.ToString(patch.GetRevision(0).GetLength()));
	EXPECT_EQ(TRUE, patch.PatchFile(0, 0, tempDir.GetTempDir() + L"\\input", tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(0), L"", false));
	git_oid oid2 = { 0 };
	EXPECT_EQ(0, git_odb_hashfile(&oid2, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(0)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
	CGitHash hashAfter = oid2;
	EXPECT_STREQ(patch.GetRevision2(0), hashAfter.ToString(patch.GetRevision2(0).GetLength()));
}

TEST(CPatch, PatchFile_BOMChanges)
{
	CString resourceDir;
	ASSERT_TRUE(GetResourcesDir(resourceDir));

	CAutoTempDir tempDir;
	ASSERT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\input", nullptr));
	ASSERT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\output", nullptr));

	CPatch patch;
	EXPECT_TRUE(patch.OpenUnifiedDiffFile(resourceDir + L"\\patches\\bom-change.patch"));
	EXPECT_STREQ(L"", patch.GetErrorMessage());
	EXPECT_EQ(4, patch.GetNumberOfFiles());

	// Add file with BOM
	{
		EXPECT_TRUE(patch.PatchFile(0, 0, tempDir.GetTempDir() + L"\\input", tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(0), L"", false));
		git_oid oid2 = { 0 };
		EXPECT_EQ(0, git_odb_hashfile(&oid2, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(0)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
		CGitHash hashAfter = oid2;
		EXPECT_STREQ(patch.GetRevision2(0), hashAfter.ToString(patch.GetRevision2(0).GetLength()));
	}

	// Add BOM to file
	{
		ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tempDir.GetTempDir() + L"\\input\\utf8-add-bom.txt", L"dsfds\r\nä\r\ndsf\r\nädsf\r\ndsf\r\nsdf", true));
		git_oid oid1 = { 0 };
		EXPECT_EQ(0, git_odb_hashfile(&oid1, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\input\\" + patch.GetFilename(2)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
		CGitHash hashBefore = oid1;
		EXPECT_STREQ(patch.GetRevision(2), hashBefore.ToString(patch.GetRevision(2).GetLength()));
		EXPECT_TRUE(patch.PatchFile(0, 2, tempDir.GetTempDir() + L"\\input", tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(2), L"", false));
		git_oid oid2 = { 0 };
		EXPECT_EQ(0, git_odb_hashfile(&oid2, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(2)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
		CGitHash hashAfter = oid2;
		EXPECT_STREQ(patch.GetRevision2(2), hashAfter.ToString(patch.GetRevision2(2).GetLength()));
	}

	// Remove BOM from file
	{
		ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tempDir.GetTempDir() + L"\\input\\drop-bom.txt", L"\uFEFFdsfds\r\nä\r\ndsf\r\nädsf\r\ndsf\r\nsdf", true));
		git_oid oid1 = { 0 };
		EXPECT_EQ(0, git_odb_hashfile(&oid1, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\input\\" + patch.GetFilename(3)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
		CGitHash hashBefore = oid1;
		EXPECT_STREQ(patch.GetRevision(3), hashBefore.ToString(patch.GetRevision(3).GetLength()));
		EXPECT_TRUE(patch.PatchFile(0, 3, tempDir.GetTempDir() + L"\\input", tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(3), L"", false));
		git_oid oid2 = { 0 };
		EXPECT_EQ(0, git_odb_hashfile(&oid2, CUnicodeUtils::GetUTF8(tempDir.GetTempDir() + L"\\output\\" + patch.GetFilename2(3)), GIT_OBJECT_BLOB, GIT_OID_SHA1));
		CGitHash hashAfter = oid2;
		EXPECT_STREQ(patch.GetRevision2(3), hashAfter.ToString(patch.GetRevision2(3).GetLength()));
	}
}
