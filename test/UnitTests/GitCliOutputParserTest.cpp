// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2026 - TortoiseGit

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
#include "GitCliOutputParser.h"
#include "PathUtils.h"
#include <random>

static std::random_device rd;

static CStringA loadGitOutput(const CString& filename)
{
	CStdioFile file;
	CString resourcesDir = CPathUtils::GetAppDirectory() + L"\\resources";
	if (!PathIsDirectory(resourcesDir))
		resourcesDir = CPathUtils::GetAppDirectory() + L"\\..\\..\\..\\test\\UnitTests\\resources";
	EXPECT_TRUE(file.Open(resourcesDir + L"\\gitexe-output\\" + filename, CFile::typeBinary | CFile::modeRead | CFile::shareDenyWrite));
	CStringA fileContent;
	const UINT filelength = static_cast<UINT>(file.GetLength());
	EXPECT_EQ(filelength, file.Read(CStrBufA(fileContent, static_cast<int>(filelength)), filelength));
	EXPECT_EQ(filelength, static_cast<UINT>(fileContent.GetLength()));
	return fileContent;
}

static std::string ConstructOutputUsingSplittedInputs(const CStringA& fileContent, size_t(*splitAt)(void* payload), void* payload = nullptr)
{
	CGitCliOutputParser cliparser;
	std::string finalText;
	size_t offset = 0;
	while (offset < static_cast<size_t>(fileContent.GetLength()))
	{
		const size_t len = std::min(splitAt(payload), static_cast<size_t>(fileContent.GetLength()) - offset);
		EmittedLines out = cliparser.Process({ fileContent.GetString() + offset, len });
		EXPECT_FALSE(out.limited);
		if (out.erasePreviousLineWithLength)
		{
			EXPECT_FALSE(finalText.empty());
			finalText.erase(finalText.size() - out.erasePreviousLineWithLength);
		}

		finalText += out.text;

		offset += len;
	}
	return finalText;
}

TEST(CGitCliOutputParser, RemoteLines)
{
	std::string_view buffer = "remote: JETZT        \nremote: 123        \rremote: abcd        \rremote: \nremote: def        \nremote: fertig        \n";

	CGitCliOutputParser cliparser;
	EmittedLines out = cliparser.Process(buffer);
	EXPECT_EQ(0u, out.erasePreviousLineWithLength);
	EXPECT_FALSE(out.limited);
	EXPECT_STREQ("remote: JETZT        \nremote: abcd        \nremote: def        \nremote: fertig        \n", out.text.c_str());
}

TEST(CGitCliOutputParser, RemoteLinesOneAtATime)
{
	CStringA input = "remote: JETZT        \nremote: 123        \rremote: abcd        \rremote: \nremote: def        \nremote: fertig        \n";
	CStringA result = "remote: JETZT        \nremote: abcd        \nremote: def        \nremote: fertig        \n";

	EXPECT_STREQ(result, ConstructOutputUsingSplittedInputs(input, [](void*) -> size_t { return 1; }).c_str());
}

TEST(CGitCliOutputParser, RemoteLinesRandomChunks)
{
	CStringA input = "remote: JETZT        \nremote: 123        \rremote: abcd        \rremote: \nremote: def        \nremote: fertig        \n";
	CStringA result = "remote: JETZT        \nremote: abcd        \nremote: def        \nremote: fertig        \n";

	uint32_t seed = (rd() << 16) ^ rd();
	std::mt19937 mt(seed);
	std::uniform_int_distribution<size_t> chunkSize(1, 1024);
	auto payload = std::make_pair(mt, chunkSize);
	EXPECT_STREQ(result, ConstructOutputUsingSplittedInputs(input, [](void* payload) { auto [mt, chunkSize] = *static_cast<std::pair<std::mt19937, std::uniform_int_distribution<size_t>>*>(payload); return chunkSize(mt); }, &payload).c_str()) << "Seed: " << seed;
}

TEST(CGitCliOutputParser, LocalLines)
{
	std::string_view buffer = "Pre-commit\n\nHallo\nCR\rLonger\rFinal\nAnotherCR\rMore Override\r\nAfter empty\nNowCRCR\r\rNewline\nEven more\r\nVery last\n";

	CGitCliOutputParser cliparser;
	EmittedLines out = cliparser.Process(buffer);
	EXPECT_EQ(0u, out.erasePreviousLineWithLength);
	EXPECT_STREQ("Pre-commit\n\nHallo\nFinalr\nMore Override\nAfter empty\nNewline\nEven more\nVery last\n", out.text.c_str());
}

TEST(CGitCliOutputParser, LocalLinesOneAtATime)
{
	CStringA input = "Pre-commit\n\nHallo\nCR\rLonger\rFinal\nAnotherCR\rMore Override\r\nAfter empty\nNowCRCR\r\rNewline\nEven more\r\nVery last\n";
	CStringA result = "Pre-commit\n\nHallo\nFinalr\nMore Override\nAfter empty\nNewline\nEven more\nVery last\n";

	EXPECT_STREQ(result, ConstructOutputUsingSplittedInputs(input, [](void*) -> size_t { return 1; }).c_str());
}

TEST(CGitCliOutputParser, LocalLinesRandomChunks)
{
	CStringA input = "Pre-commit\n\nHallo\nCR\rLonger\rFinal\nAnotherCR\rMore Override\r\nAfter empty\nNowCRCR\r\rNewline\nEven more\r\nVery last\n";
	CStringA result = "Pre-commit\n\nHallo\nFinalr\nMore Override\nAfter empty\nNewline\nEven more\nVery last\n";

	uint32_t seed = (rd() << 16) ^ rd();
	std::mt19937 mt(seed);
	std::uniform_int_distribution<size_t> chunkSize(1, 1024);
	auto payload = std::make_pair(mt, chunkSize);
	EXPECT_STREQ(result, ConstructOutputUsingSplittedInputs(input, [](void* payload) { auto [mt, chunkSize] = *static_cast<std::pair<std::mt19937, std::uniform_int_distribution<size_t>>*>(payload); return chunkSize(mt); }, &payload).c_str()) << "Seed: " << seed;
}

TEST(CGitCliOutputParser, CloneOutput)
{
	std::string_view buffer =
		"Cloning into 'poc'...\n"
		"Looking up 127.0.0.1 ... done.\n"
		"Connecting to 127.0.0.1 (port 9418) ... 127.0.0.1 done.\n"
		"remote: remote: HOOK-HIT        \n"
		"remote: Umlauts: ääö        \n"
		"remote: 123456789        \r"
		"remote: abcd        \r"
		"remote: \r"
		"remote: \n"
		"remote: \n"
		"remote: def        \n"
		"remote: 0:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r"
		"remote: 100:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r"
		"remote: 200:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r"
		"remote: 300:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r"
		"remote: 400:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r";

	CGitCliOutputParser cliparser;
	EmittedLines out = cliparser.Process(buffer);
	EXPECT_EQ(0u, out.erasePreviousLineWithLength);
	EXPECT_FALSE(out.limited);
	EXPECT_STREQ("Cloning into 'poc'...\n"
		"Looking up 127.0.0.1 ... done.\n"
		"Connecting to 127.0.0.1 (port 9418) ... 127.0.0.1 done.\n"
		"remote: remote: HOOK-HIT        \n"
		"remote: Umlauts: ääö        \n"
		"remote: abcd        \n"
		"remote: \n"
		"remote: def        \n"
		"remote: 400:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        ", out.text.c_str());
}

TEST(CGitCliOutputParser, CloneOutputSimpleTwoSteps)
{
	std::string_view buffer1 =
		"Cloning into 'poc'...\n"
		"Looking up 127.0.0.1 ... done.\n"
		"Connecting to 127.0.0.1 (port 9418) ... 127.0.0.1 done.\n"
		"remote: remote: HOOK-HIT        \n"
		"remote: Umlauts: ääö        \n";
	std::string_view buffer2 = "def\n";

	CGitCliOutputParser cliparser;
	EmittedLines out1 = cliparser.Process(buffer1);
	EXPECT_EQ(0u, out1.erasePreviousLineWithLength);
	EXPECT_FALSE(out1.limited);
	EXPECT_STREQ("Cloning into 'poc'...\n"
		"Looking up 127.0.0.1 ... done.\n"
		"Connecting to 127.0.0.1 (port 9418) ... 127.0.0.1 done.\n"
		"remote: remote: HOOK-HIT        \n"
		"remote: Umlauts: ääö        \n", out1.text.c_str());

	EmittedLines out2 = cliparser.Process(buffer2);
	EXPECT_EQ(0u, out2.erasePreviousLineWithLength);
	EXPECT_FALSE(out2.limited);
	EXPECT_STREQ("def\n", out2.text.c_str());
}

TEST(CGitCliOutputParser, CloneOutputTwoSteps)
{
	std::string_view buffer1 =
		"Cloning into 'poc'...\n"
		"Looking up 127.0.0.1 ... done.\n"
		"Connecting to 127.0.0.1 (port 9418) ... 127.0.0.1 done.\n"
		"remote: remote: HOOK-HIT        \n"
		"remote: Umlauts: ääö        \n"
		"remote: 123456789        \r"
		"remote: abcd        \r";
	std::string_view buffer2 =
		"remote: \r"
		"remote: \n"
		"remote: \n"
		"remote: def        \n"
		"remote: 0:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r"
		"remote: 100:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r"
		"remote: 200:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r"
		"remote: 300:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r"
		"remote: 400:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        \r";

	CGitCliOutputParser cliparser;
	EmittedLines out1 = cliparser.Process(buffer1);
	EXPECT_EQ(0u, out1.erasePreviousLineWithLength);
	EXPECT_FALSE(out1.limited);
	EXPECT_STREQ("Cloning into 'poc'...\n"
		"Looking up 127.0.0.1 ... done.\n"
		"Connecting to 127.0.0.1 (port 9418) ... 127.0.0.1 done.\n"
		"remote: remote: HOOK-HIT        \n"
		"remote: Umlauts: ääö        \n"
		"remote: abcd        ", out1.text.c_str());

	EmittedLines out2 = cliparser.Process(buffer2);
	EXPECT_EQ(20u, out2.erasePreviousLineWithLength);
	EXPECT_FALSE(out2.limited);
	EXPECT_STREQ("remote: abcd        \n"
		"remote: \n"
		"remote: def        \n"
		"remote: 400:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA        ", out2.text.c_str());
}

TEST(CGitCliOutputParser, SimpleCloneOutput)
{
	CStringA filecontent = loadGitOutput(L"clone.txt");

	CGitCliOutputParser cliparser;
	EmittedLines out = cliparser.Process({ filecontent.GetString(), static_cast<size_t>(filecontent.GetLength()) });
	EXPECT_EQ(0u, out.erasePreviousLineWithLength);
	EXPECT_FALSE(out.limited);
	EXPECT_STREQ(loadGitOutput(L"clone-final.txt"), out.text.c_str());
}

TEST(CGitCliOutputParser, SimpleCloneOutputOneAtATime)
{
	EXPECT_STREQ(loadGitOutput(L"clone-final.txt"), ConstructOutputUsingSplittedInputs(loadGitOutput(L"clone.txt"), [](void*) -> size_t {return 1; }).c_str());
}

TEST(CGitCliOutputParser, SimpleCloneOutputRandomChunks)
{
	uint32_t seed = (rd() << 16) ^ rd();
	std::mt19937 mt(seed);
	std::uniform_int_distribution<size_t> chunkSize(1, 1024);
	auto payload = std::make_pair(mt, chunkSize);
	EXPECT_STREQ(loadGitOutput(L"clone-final.txt"), ConstructOutputUsingSplittedInputs(loadGitOutput(L"clone.txt"), [](void* payload) { auto [mt, chunkSize] = *static_cast<std::pair<std::mt19937, std::uniform_int_distribution<size_t>>*>(payload); return chunkSize(mt); }, &payload).c_str()) << "Seed: " << seed;
}

TEST(CGitCliOutputParser, ComplexCloneOutput)
{
	CStringA filecontent = loadGitOutput(L"clone2.txt");

	CGitCliOutputParser cliparser;
	EmittedLines out = cliparser.Process({ filecontent.GetString(), static_cast<size_t>(filecontent.GetLength()) });
	EXPECT_EQ(0u, out.erasePreviousLineWithLength);
	EXPECT_FALSE(out.limited);
	EXPECT_STREQ(loadGitOutput(L"clone2-final.txt"), out.text.c_str());
}

TEST(CGitCliOutputParser, ComplexCloneOutputOneAtATime)
{
	EXPECT_STREQ(loadGitOutput(L"clone2-final.txt"), ConstructOutputUsingSplittedInputs(loadGitOutput(L"clone2.txt"), [](void*) -> size_t { return 1; }).c_str());
}

TEST(CGitCliOutputParser, ComplexCloneOutputRandomChunks)
{
	uint32_t seed = (rd() << 16) ^ rd();
	std::mt19937 mt(seed);
	std::uniform_int_distribution<size_t> chunkSize(1, 1024);
	auto payload = std::make_pair(mt, chunkSize);
	EXPECT_STREQ(loadGitOutput(L"clone2-final.txt"), ConstructOutputUsingSplittedInputs(loadGitOutput(L"clone2.txt"), [](void* payload) { auto [mt, chunkSize] = *static_cast<std::pair<std::mt19937, std::uniform_int_distribution<size_t>>*>(payload); return chunkSize(mt); }, &payload).c_str()) << "Seed: " << seed;
}

TEST(CGitCliOutputParser, ManyLines)
{
	std::string buffer =
		"Cloning into 'poc'...\n"
		"Looking up 127.0.0.1 ... done.\n"
		"remote: def        \n";
	for (int i = 0; i < 5000; ++i)
	{
		buffer.append(1024, 'A');
		buffer.push_back('\n');
	}

	CGitCliOutputParser cliparser{ 1024 * 1024 };
	EmittedLines out = cliparser.Process(buffer);
	EXPECT_EQ(0u, out.erasePreviousLineWithLength);
	EXPECT_TRUE(out.limited);
	EXPECT_EQ(1048648u, out.text.size());
	EXPECT_TRUE(out.text.starts_with("Cloning into 'poc'...\n"
		"Looking up 127.0.0.1 ... done.\n"
		"remote: def        \n"
		"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
}
