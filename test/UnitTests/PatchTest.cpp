// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018-2019 - TortoiseGit

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

