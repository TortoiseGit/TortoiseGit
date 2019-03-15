// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2017-2019 - TortoiseGit
// Copyright (C) 2003-2016 - TortoiseSVN

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
#include "SmartHandle.h"
#include "../Utils/CrashReport.h"

#include <iostream>
#include <io.h>
#include <fcntl.h>

#include "GitWCRev.h"
#include "status.h"
#include "UnicodeUtils.h"
#include "../version.h"

// Define the help text as a multi-line macro
// Every line except the last must be terminated with a backslash
#define HelpText1 "\
Usage: GitWCRev Path [SrcVersionFile DstVersionFile] [-mMuUdqsFe]\n\
\n\
Params:\n\
Path               :   path to a Git working tree OR\n\
                       a directory/file inside a working tree.\n\
SrcVersionFile     :   path to a template file containing keywords.\n\
DstVersionFile     :   path to save the resulting parsed file.\n\
-m                 :   if given, then GitWCRev will error if the passed\n\
                       path contains local modifications.\n\
-M                 :   same as above, but recursively\n\
-u                 :   if given, then GitWCRev will error if the passed\n\
                       path contains unversioned items.\n\
-U                 :   same as above, but recursively\n\
-d                 :   if given, then GitWCRev will only do its job if\n\
                       DstVersionFile does not exist.\n\
-q                 :   if given, then GitWCRev will perform keyword\n\
                       substitution but will not show status on stdout.\n"
#define HelpText2 "\
-s                 :   if given, submodules are not checked. This increases\n\
                       the checking speed.\n"
#define HelpText3 "\
-e                 :   changes the console output encoding to Unicode\n"

#define HelpText4 "\
Switches must be given in a single argument, e.g. '-nm' not '-n -m'.\n\
\n\
GitWCRev reads the Git status of all files in the passed path\n\
including submodules. If SrcVersionFile is specified, it is scanned\n\
for special placeholders of the form \"$WCxxx$\".\n\
SrcVersionFile is then copied to DstVersionFile but the placeholders\n\
are replaced with information about the working tree as follows:\n\
\n\
$WCREV$         HEAD commit revision\n\
$WCREV=$        HEAD commit revision, truncated after the numner of chars\n\
                provided after the =\n\
$WCDATE$        Date of the HEAD revision\n\
$WCDATE=$       Like $WCDATE$ with an added strftime format after the =\n\
$WCNOW$         Current system date & time\n\
$WCNOW=$        Like $WCNOW$ with an added strftime format after the =\n\
\n"

#define HelpText5 "\
The strftime format strings for $WCxxx=$ must not be longer than 1024\n\
characters, and must not produce output greater than 1024 characters.\n\
\n\
Placeholders of the form \"$WCxxx?TrueText:FalseText$\" are replaced with\n\
TrueText if the tested condition is true, and FalseText if false.\n\
\n\
$WCMODS$        True if uncommitted modifications were found\n\
$WCUNVER        True if unversioned and unignored files were found\n\
$WCISTAGGED$    True if the HEAD commit is tagged\n\
$WCINGIT$       True if the item is versioned\n\
$WCLOGCOUNT$    Number of first-parent commits for the current branch\n\
$WCLOGCOUNT&$   Number of commits ANDed with the number after the &\n\
$WCLOGCOUNT+$   Number of commits added with the number after the &\n\
$WCLOGCOUNT-$   Number of commits subtracted with the number after the &\n\
$WCBRANCH$      Current branch name, SHA - 1 if head is detached\n"

// End of multi-line help text.

#define	VERDEF			"$WCREV$"
#define	VERDEFSHORT		"$WCREV="
#define	DATEDEF			"$WCDATE$"
#define	DATEDEFUTC		"$WCDATEUTC$"
#define	DATEWFMTDEF		"$WCDATE="
#define	DATEWFMTDEFUTC	"$WCDATEUTC="
#define	MODDEF			"$WCMODS?"
#define	ISTAGGED		"$WCISTAGGED?"
#define	NOWDEF			"$WCNOW$"
#define	NOWDEFUTC		"$WCNOWUTC$"
#define	NOWWFMTDEF		"$WCNOW="
#define	NOWWFMTDEFUTC	"$WCNOWUTC="
#define	ISINGIT			"$WCINGIT?"
#define	UNVERDEF		"$WCUNVER?"
#define	MODSFILEDEF		"$WCFILEMODS?"
#define	SUBDEF			"$WCSUBMODULE?"
#define	SUBUP2DATEDEF	"$WCSUBMODULEUP2DATE?"
#define	MODINSUBDEF		"$WCMODSINSUBMODULE?"
#define	UNVERINSUBDEF	"$WCUNVERINSUBMODULE?"
#define	UNVERFULLDEF	"$WCUNVERFULL?"
#define	MODFULLDEF		"$WCMODSFULL?"
#define	VALDEF			"$WCLOGCOUNT$"
#define	VALDEFAND		"$WCLOGCOUNT&"
#define	VALDEFOFFSET1	"$WCLOGCOUNT-"
#define	VALDEFOFFSET2	"$WCLOGCOUNT+"
#define	BRANCHDEF		"$WCBRANCH$"

// Value for apr_time_t to signify "now"
#define USE_TIME_NOW    -2 // 0 and -1 might already be significant.

bool FindPlaceholder(char* def, char* pBuf, size_t& index, size_t filelength)
{
	size_t deflen = strlen(def);
	while (index + deflen <= filelength)
	{
		if (memcmp(pBuf + index, def, deflen) == 0)
			return true;
		index++;
	}
	return false;
}
bool FindPlaceholderW(wchar_t *def, wchar_t *pBuf, size_t & index, size_t filelength)
{
	size_t deflen = wcslen(def);
	while ((index + deflen) * sizeof(wchar_t) <= filelength)
	{
		if (memcmp(pBuf + index, def, deflen * sizeof(wchar_t)) == 0)
			return true;
		index++;
	}

	return false;
}

bool InsertRevision(char* def, char* pBuf, size_t& index, size_t& filelength, size_t maxlength, GitWCRev_t* GitStat)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholder(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	size_t hashlen = strlen(GitStat->HeadHashReadable);
	ptrdiff_t exp = 0;
	if ((strcmp(def, VERDEFSHORT) == 0))
	{
		char format[1024] = { 0 };
		char* pStart = pBuf + index + strlen(def);
		char* pEnd = pStart;

		while (*pEnd != '$')
		{
			++pEnd;
			if (pEnd - pBuf >= static_cast<__int64>(filelength))
				return false; // No terminator - malformed so give up.
		}
		if ((pEnd - pStart) > 1024)
			return false; // value specifier too big
		exp = pEnd - pStart + 1;
		memcpy(format, pStart, pEnd - pStart);
		unsigned long number = strtoul(format, nullptr, 0);
		if (strcmp(def, VERDEFSHORT) == 0 && number < hashlen)
			hashlen = number;
	}
	// Replace the $WCxxx$ string with the actual revision number
	char* pBuild = pBuf + index;
	ptrdiff_t Expansion = hashlen - exp - strlen(def);
	if (Expansion < 0)
		memmove(pBuild, pBuild - Expansion, filelength - ((pBuild - Expansion) - pBuf));
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if (maxlength < Expansion + filelength)
			return false;
		memmove(pBuild + Expansion, pBuild, filelength - (pBuild - pBuf));
	}
	memmove(pBuild, GitStat->HeadHashReadable, hashlen);
	filelength += Expansion;
	return true;
}
bool InsertRevisionW(wchar_t* def, wchar_t* pBuf, size_t& index, size_t& filelength, size_t maxlength, GitWCRev_t* GitStat)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholderW(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	size_t hashlen = strlen(GitStat->HeadHashReadable);
	ptrdiff_t exp = 0;
	if ((wcscmp(def, TEXT(VERDEFSHORT)) == 0))
	{
		wchar_t format[1024] = { 0 };
		wchar_t* pStart = pBuf + index + wcslen(def);
		wchar_t* pEnd = pStart;

		while (*pEnd != '$')
		{
			++pEnd;
			if (static_cast<__int64>(pEnd - pBuf) * static_cast<__int64>(sizeof(wchar_t)) >= static_cast<__int64>(filelength))
				return false; // No terminator - malformed so give up.
		}
		if ((pEnd - pStart) > 1024)
			return false; // Format specifier too big
		exp = pEnd - pStart + 1;
		memcpy(format, pStart, (pEnd - pStart) * sizeof(wchar_t));
		unsigned long number = wcstoul(format, nullptr, 0);
		if (wcscmp(def, TEXT(VERDEFSHORT)) == 0 && number < hashlen)
			hashlen = number;
	}
	// Replace the $WCxxx$ string with the actual revision number
	wchar_t* pBuild = pBuf + index;
	ptrdiff_t Expansion = hashlen - exp - wcslen(def);
	if (Expansion < 0)
		memmove(pBuild, pBuild - Expansion, (filelength - ((pBuild - Expansion) - pBuf) * sizeof(wchar_t)));
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if (maxlength < Expansion * sizeof(wchar_t) + filelength)
			return false;
		memmove(pBuild + Expansion, pBuild, (filelength - (pBuild - pBuf) * sizeof(wchar_t)));
	}
	std::wstring hash = CUnicodeUtils::StdGetUnicode(GitStat->HeadHashReadable);
	memmove(pBuild, hash.c_str(), hashlen * sizeof(wchar_t));
	filelength += (Expansion * sizeof(wchar_t));
	return true;
}

bool InsertNumber(char* def, char* pBuf, size_t& index, size_t& filelength, size_t maxlength, size_t Value)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholder(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	ptrdiff_t exp = 0;
	if ((strcmp(def, VALDEFAND) == 0) || (strcmp(def, VALDEFOFFSET1) == 0) || (strcmp(def, VALDEFOFFSET2) == 0))
	{
		char format[1024] = { 0 };
		char* pStart = pBuf + index + strlen(def);
		char* pEnd = pStart;

		while (*pEnd != '$')
		{
			++pEnd;
			if (pEnd - pBuf >= static_cast<__int64>(filelength))
				return false; // No terminator - malformed so give up.
		}
		if ((pEnd - pStart) > 1024)
			return false; // value specifier too big

		exp = pEnd - pStart + 1;
		memcpy(format, pStart, pEnd - pStart);
		unsigned long number = strtoul(format, NULL, 0);
		if (strcmp(def, VALDEFAND) == 0)
		{
			if (Value != -1)
				Value &= number;
		}
		if (strcmp(def, VALDEFOFFSET1) == 0)
		{
			if (Value != -1)
				Value -= number;
		}
		if (strcmp(def, VALDEFOFFSET2) == 0)
		{
			if (Value != -1)
				Value += number;
		}
	}
	// Format the text to insert at the placeholder
	char destbuf[1024] = { 0 };
	sprintf_s(destbuf, "%zd", Value);

	// Replace the $WCxxx$ string with the actual revision number
	char* pBuild = pBuf + index;
	ptrdiff_t Expansion = strlen(destbuf) - exp - strlen(def);
	if (Expansion < 0)
		memmove(pBuild, pBuild - Expansion, filelength - ((pBuild - Expansion) - pBuf));
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if (maxlength < Expansion + filelength)
			return false;
		memmove(pBuild + Expansion, pBuild, filelength - (pBuild - pBuf));
	}
	memmove(pBuild, destbuf, strlen(destbuf));
	filelength += Expansion;
	return true;
}
bool InsertNumberW(wchar_t* def, wchar_t* pBuf, size_t& index, size_t& filelength, size_t maxlength, size_t Value)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholderW(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}

	ptrdiff_t exp = 0;
	if ((wcscmp(def, TEXT(VALDEFAND)) == 0) || (wcscmp(def, TEXT(VALDEFOFFSET1)) == 0) || (wcscmp(def, TEXT(VALDEFOFFSET2)) == 0))
	{
		wchar_t format[1024] = { 0 };
		wchar_t* pStart = pBuf + index + wcslen(def);
		wchar_t* pEnd = pStart;

		while (*pEnd != '$')
		{
			++pEnd;
			if (static_cast<__int64>(pEnd - pBuf) * static_cast<__int64>(sizeof(wchar_t)) >= static_cast<__int64>(filelength))
				return false; // No terminator - malformed so give up.
		}
		if ((pEnd - pStart) > 1024)
			return false; // Format specifier too big

		exp = pEnd - pStart + 1;
		memcpy(format, pStart, (pEnd - pStart) * sizeof(wchar_t));
		unsigned long number = wcstoul(format, NULL, 0);
		if (wcscmp(def, TEXT(VALDEFAND)) == 0)
		{
			if (Value != -1)
				Value &= number;
		}
		if (wcscmp(def, TEXT(VALDEFOFFSET1)) == 0)
		{
			if (Value != -1)
				Value -= number;
		}
		if (wcscmp(def, TEXT(VALDEFOFFSET2)) == 0)
		{
			if (Value != -1)
				Value += number;
		}
	}

	// Format the text to insert at the placeholder
	wchar_t destbuf[40];
	swprintf_s(destbuf, L"%zd", Value);
	// Replace the $WCxxx$ string with the actual revision number
	wchar_t* pBuild = pBuf + index;
	ptrdiff_t Expansion = wcslen(destbuf) - exp - wcslen(def);
	if (Expansion < 0)
		memmove(pBuild, pBuild - Expansion, (filelength - ((pBuild - Expansion) - pBuf) * sizeof(wchar_t)));
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if (maxlength < Expansion * sizeof(wchar_t) + filelength)
			return false;
		memmove(pBuild + Expansion, pBuild, (filelength - (pBuild - pBuf) * sizeof(wchar_t)));
	}
	memmove(pBuild, destbuf, wcslen(destbuf) * sizeof(wchar_t));
	filelength += (Expansion * sizeof(wchar_t));
	return true;
}

void _invalid_parameter_donothing(
	const wchar_t* /*expression*/,
	const wchar_t* /*function*/,
	const wchar_t* /*file*/,
	unsigned int /*line*/,
	uintptr_t /*pReserved*/
	)
{
	// do nothing
}

bool InsertDate(char* def, char* pBuf, size_t& index, size_t& filelength, size_t maxlength, __time64_t ttime)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholder(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	// Format the text to insert at the placeholder
	if (ttime == USE_TIME_NOW)
		_time64(&ttime);

	struct tm newtime;
	if (strstr(def, "UTC"))
	{
		if (_gmtime64_s(&newtime, &ttime))
			return false;
	}
	else
	{
		if (_localtime64_s(&newtime, &ttime))
			return false;
	}
	char destbuf[1024] = { 0 };
	char* pBuild = pBuf + index;
	ptrdiff_t Expansion;
	if ((strcmp(def,DATEWFMTDEF) == 0) || (strcmp(def,NOWWFMTDEF) == 0) || (strcmp(def,DATEWFMTDEFUTC) == 0) || (strcmp(def,NOWWFMTDEFUTC) == 0))
	{
		// Format the date/time according to the supplied strftime format string
		char format[1024] = { 0 };
		char* pStart = pBuf + index + strlen(def);
		char* pEnd = pStart;

		while (*pEnd != '$')
		{
			++pEnd;
			if (pEnd - pBuf >= static_cast<__int64>(filelength))
				return false; // No terminator - malformed so give up.
		}
		if ((pEnd - pStart) > 1024)
			return false; // Format specifier too big
		memcpy(format,pStart,pEnd - pStart);

		// to avoid wcsftime aborting if the user specified an invalid time format,
		// we set a custom invalid parameter handler that does nothing at all:
		// that makes wcsftime do nothing and set errno to EINVAL.
		// we restore the invalid parameter handler right after
		_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(_invalid_parameter_donothing);

		if (strftime(destbuf,1024,format,&newtime) == 0)
		{
			if (errno == EINVAL)
				strcpy_s(destbuf, "Invalid Time Format Specified");
		}
		_set_invalid_parameter_handler(oldHandler);
		Expansion = strlen(destbuf) - (strlen(def) + pEnd - pStart + 1);
	}
	else
	{
		// Format the date/time in international format as yyyy/mm/dd hh:mm:ss
		sprintf_s(destbuf, "%04d/%02d/%02d %02d:%02d:%02d", newtime.tm_year + 1900, newtime.tm_mon + 1, newtime.tm_mday, newtime.tm_hour, newtime.tm_min, newtime.tm_sec);
		Expansion = strlen(destbuf) - strlen(def);
	}
	// Replace the def string with the actual commit date
	if (Expansion < 0)
		memmove(pBuild, pBuild - Expansion, filelength - ((pBuild - Expansion) - pBuf));
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if (maxlength < Expansion + filelength)
			return false;
		memmove(pBuild + Expansion, pBuild, filelength - (pBuild - pBuf));
	}
	memmove(pBuild, destbuf, strlen(destbuf));
	filelength += Expansion;
	return true;
}
bool InsertDateW(wchar_t* def, wchar_t* pBuf, size_t& index, size_t& filelength, size_t maxlength, __time64_t ttime)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholderW(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	// Format the text to insert at the placeholder
	if (ttime == USE_TIME_NOW)
		_time64(&ttime);

	struct tm newtime;
	if (wcsstr(def, L"UTC"))
	{
		if (_gmtime64_s(&newtime, &ttime))
			return false;
	}
	else
	{
		if (_localtime64_s(&newtime, &ttime))
			return false;
	}
	wchar_t destbuf[1024];
	wchar_t* pBuild = pBuf + index;
	ptrdiff_t Expansion;
	if ((wcscmp(def,TEXT(DATEWFMTDEF)) == 0) || (wcscmp(def,TEXT(NOWWFMTDEF)) == 0) ||
		(wcscmp(def,TEXT(DATEWFMTDEFUTC)) == 0) || (wcscmp(def,TEXT(NOWWFMTDEFUTC)) == 0))
	{
		// Format the date/time according to the supplied strftime format string
		wchar_t format[1024] = { 0 };
		wchar_t* pStart = pBuf + index + wcslen(def);
		wchar_t* pEnd = pStart;

		while (*pEnd != '$')
		{
			++pEnd;
			if (static_cast<__int64>(pEnd - pBuf) * static_cast<__int64>(sizeof(wchar_t)) >= static_cast<__int64>(filelength))
				return false; // No terminator - malformed so give up.
		}
		if ((pEnd - pStart) > 1024)
			return false; // Format specifier too big
		memcpy(format, pStart, (pEnd - pStart) * sizeof(wchar_t));

		// to avoid wcsftime aborting if the user specified an invalid time format,
		// we set a custom invalid parameter handler that does nothing at all:
		// that makes wcsftime do nothing and set errno to EINVAL.
		// we restore the invalid parameter handler right after
		_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(_invalid_parameter_donothing);

		if (wcsftime(destbuf,1024,format,&newtime) == 0)
		{
			if (errno == EINVAL)
				wcscpy_s(destbuf, L"Invalid Time Format Specified");
		}
		_set_invalid_parameter_handler(oldHandler);

		Expansion = wcslen(destbuf) - (wcslen(def) + pEnd - pStart + 1);
	}
	else
	{
		// Format the date/time in international format as yyyy/mm/dd hh:mm:ss
		swprintf_s(destbuf, L"%04d/%02d/%02d %02d:%02d:%02d", newtime.tm_year + 1900, newtime.tm_mon + 1, newtime.tm_mday, newtime.tm_hour, newtime.tm_min, newtime.tm_sec);
		Expansion = wcslen(destbuf) - wcslen(def);
	}
	// Replace the def string with the actual commit date
	if (Expansion < 0)
		memmove(pBuild, pBuild - Expansion, (filelength - ((pBuild - Expansion) - pBuf) * sizeof(wchar_t)));
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if (maxlength < Expansion * sizeof(wchar_t) + filelength)
			return false;
		memmove(pBuild + Expansion, pBuild, (filelength - (pBuild - pBuf) * sizeof(wchar_t)));
	}
	memmove(pBuild, destbuf, wcslen(destbuf) * sizeof(wchar_t));
	filelength += Expansion * sizeof(wchar_t);
	return true;
}

int InsertBoolean(char* def, char* pBuf, size_t& index, size_t& filelength, BOOL isTrue)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholder(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	// Look for the terminating '$' character
	char* pBuild = pBuf + index;
	char* pEnd = pBuild + 1;
	while (*pEnd != '$')
	{
		++pEnd;
		if (pEnd - pBuf >= static_cast<__int64>(filelength))
			return false; // No terminator - malformed so give up.
	}

	// Look for the ':' dividing TrueText from FalseText
	char *pSplit = pBuild + 1;
	// this loop is guaranteed to terminate due to test above.
	while (*pSplit != ':' && *pSplit != '$')
		pSplit++;

	if (*pSplit == '$')
		return false; // No split - malformed so give up.

	if (isTrue)
	{
		// Replace $WCxxx?TrueText:FalseText$ with TrueText
		// Remove :FalseText$
		memmove(pSplit, pEnd + 1, filelength - (pEnd + 1 - pBuf));
		filelength -= (pEnd + 1 - pSplit);
		// Remove $WCxxx?
		size_t deflen = strlen(def);
		memmove(pBuild, pBuild + deflen, filelength - (pBuild + deflen - pBuf));
		filelength -= deflen;
	}
	else
	{
		// Replace $WCxxx?TrueText:FalseText$ with FalseText
		// Remove terminating $
		memmove(pEnd, pEnd + 1, filelength - (pEnd + 1 - pBuf));
		filelength--;
		// Remove $WCxxx?TrueText:
		memmove(pBuild, pSplit + 1, filelength - (pSplit + 1 - pBuf));
		filelength -= (pSplit + 1 - pBuild);
	}
	return true;
}
bool InsertBooleanW(wchar_t* def, wchar_t* pBuf, size_t& index, size_t& filelength, BOOL isTrue)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholderW(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	// Look for the terminating '$' character
	wchar_t* pBuild = pBuf + index;
	wchar_t* pEnd = pBuild + 1;
	while (*pEnd != L'$')
	{
		++pEnd;
		if (pEnd - pBuf >= static_cast<__int64>(filelength))
			return false; // No terminator - malformed so give up.
	}

	// Look for the ':' dividing TrueText from FalseText
	wchar_t *pSplit = pBuild + 1;
	// this loop is guaranteed to terminate due to test above.
	while (*pSplit != L':' && *pSplit != L'$')
		pSplit++;

	if (*pSplit == L'$')
		return false; // No split - malformed so give up.

	if (isTrue)
	{
		// Replace $WCxxx?TrueText:FalseText$ with TrueText
		// Remove :FalseText$
		memmove(pSplit, pEnd + 1, (filelength - (pEnd + 1 - pBuf) * sizeof(wchar_t)));
		filelength -= ((pEnd + 1 - pSplit) * sizeof(wchar_t));
		// Remove $WCxxx?
		size_t deflen = wcslen(def);
		memmove(pBuild, pBuild + deflen, (filelength - (pBuild + deflen - pBuf) * sizeof(wchar_t)));
		filelength -= (deflen * sizeof(wchar_t));
	}
	else
	{
		// Replace $WCxxx?TrueText:FalseText$ with FalseText
		// Remove terminating $
		memmove(pEnd, pEnd + 1, (filelength - (pEnd + 1 - pBuf) * sizeof(wchar_t)));
		filelength -= sizeof(wchar_t);
		// Remove $WCxxx?TrueText:
		memmove(pBuild, pSplit + 1, (filelength - (pSplit + 1 - pBuf) * sizeof(wchar_t)));
		filelength -= ((pSplit + 1 - pBuild) * sizeof(wchar_t));
	}
	return true;
}

bool InsertText(char* def, char* pBuf, size_t& index, size_t& filelength, size_t maxlength, const std::string& Value)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholder(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	// Replace the $WCxxx$ string with the actual value
	char* pBuild = pBuf + index;
	ptrdiff_t Expansion = Value.length() - strlen(def);
	if (Expansion < 0)
		memmove(pBuild, pBuild - Expansion, filelength - ((pBuild - Expansion) - pBuf));
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if (maxlength < Expansion + filelength)
			return false;
		memmove(pBuild + Expansion, pBuild, filelength - (pBuild - pBuf));
	}
	memmove(pBuild, Value.c_str(), Value.length());
	filelength += Expansion;
	return true;
}
bool InsertTextW(wchar_t* def, wchar_t* pBuf, size_t& index, size_t& filelength, size_t maxlength, const std::string& Value)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholderW(def, pBuf, index, filelength))
	{
		// No more matches found.
		return false;
	}
	// Replace the $WCxxx$ string with the actual revision number
	wchar_t* pBuild = pBuf + index;
	ptrdiff_t Expansion = Value.length() - wcslen(def);
	if (Expansion < 0)
		memmove(pBuild, pBuild - Expansion, (filelength - ((pBuild - Expansion) - pBuf) * sizeof(wchar_t)));
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if (maxlength < Expansion * sizeof(wchar_t) + filelength)
			return false;
		memmove(pBuild + Expansion, pBuild, (filelength - (pBuild - pBuf) * sizeof(wchar_t)));
	}
	std::wstring wValue = CUnicodeUtils::StdGetUnicode(Value);
	memmove(pBuild, wValue.c_str(), wValue.length() * sizeof(wchar_t));
	filelength += (Expansion * sizeof(wchar_t));
	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// we have three parameters
	const TCHAR* src = nullptr;
	const TCHAR* dst = nullptr;
	const TCHAR* wc = nullptr;
	BOOL bErrResursively = FALSE;
	BOOL bErrOnMods = FALSE;
	BOOL bErrOnUnversioned = FALSE;
	BOOL bQuiet = FALSE;
	GitWCRev_t GitStat;

	SetDllDirectory(L"");
	CCrashReportTGit crasher(L"GitWCRev " _T(APP_X64_STRING), TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, TGIT_VERDATE);

	if (argc >= 2 && argc <= 5)
	{
		// WC path is always first argument.
		wc = argv[1];
	}
	if (argc == 4 || argc == 5)
	{
		// GitWCRev Path Tmpl.in Tmpl.out [-params]
		src = argv[2];
		dst = argv[3];
		if (!PathFileExists(src))
		{
			_tprintf(L"File '%s' does not exist\n", src);
			return ERR_FNF; // file does not exist
		}
	}
	if (argc == 3 || argc == 5)
	{
		// GitWCRev Path -params
		// GitWCRev Path Tmpl.in Tmpl.out -params
		const TCHAR* Params = argv[argc - 1];
		if (Params[0] == L'-')
		{
			if (wcschr(Params, L'e') != 0)
				_setmode(_fileno(stdout), _O_U16TEXT);
			if (wcschr(Params, L'q') != 0)
				bQuiet = TRUE;
			if (wcschr(Params, L'm') != 0)
				bErrOnMods = TRUE;
			if (wcschr(Params, L'u') != 0)
				bErrOnUnversioned = TRUE;
			if (wcschr(Params, L'M') != 0)
			{
				bErrOnMods = TRUE;
				bErrResursively = TRUE;
			}
			if (wcschr(Params, L'U') != 0)
			{
				bErrOnUnversioned = TRUE;
				bErrResursively = TRUE;
			}
			if (wcschr(Params, L'd') != 0)
			{
				if (dst && PathFileExists(dst))
				{
					_tprintf(L"File '%s' already exists\n", dst);
					return ERR_OUT_EXISTS;
				}
			}
			if (wcschr(Params, L's') != 0)
				GitStat.bNoSubmodules = TRUE;
		}
		else
		{
			// Bad params - abort and display help.
			wc = nullptr;
		}
	}

	if (!wc)
	{
		_tprintf(L"GitWCRev %d.%d.%d, Build %d - %s\n\n", TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, _T(TGIT_PLATFORM));
		_putts(_T(HelpText1));
		_putts(_T(HelpText2));
		_putts(_T(HelpText3));
		_putts(_T(HelpText4));
		_putts(_T(HelpText5));
		return ERR_SYNTAX;
	}

	DWORD reqLen = GetFullPathName(wc, 0, nullptr, nullptr);
	auto wcfullPath = std::make_unique<TCHAR[]>(reqLen + 1);
	GetFullPathName(wc, reqLen, wcfullPath.get(), nullptr);
	// GetFullPathName() sometimes returns the full path with the wrong
	// case. This is not a problem on Windows since its filesystem is
	// case-insensitive. But for Git that's a problem if the wrong case
	// is inside a working copy: the git index is case sensitive.
	// To fix the casing of the path, we use a trick:
	// convert the path to its short form, then back to its long form.
	// That will fix the wrong casing of the path.
	int shortlen = GetShortPathName(wcfullPath.get(), nullptr, 0);
	if (shortlen)
	{
		auto shortPath = std::make_unique<TCHAR[]>(shortlen + 1);
		if (GetShortPathName(wcfullPath.get(), shortPath.get(), shortlen + 1))
		{
			reqLen = GetLongPathName(shortPath.get(), nullptr, 0);
			wcfullPath = std::make_unique<TCHAR[]>(reqLen + 1);
			GetLongPathName(shortPath.get(), wcfullPath.get(), reqLen);
		}
	}
	wc = wcfullPath.get();
	std::unique_ptr<TCHAR[]> dstfullPath;
	if (dst)
	{
		reqLen = GetFullPathName(dst, 0, nullptr, nullptr);
		dstfullPath = std::make_unique<TCHAR[]>(reqLen + 1);
		GetFullPathName(dst, reqLen, dstfullPath.get(), nullptr);
		shortlen = GetShortPathName(dstfullPath.get(), nullptr, 0);
		if (shortlen)
		{
			auto shortPath = std::make_unique<TCHAR[]>(shortlen + 1);
			if (GetShortPathName(dstfullPath.get(), shortPath.get(), shortlen+1))
			{
				reqLen = GetLongPathName(shortPath.get(), nullptr, 0);
				dstfullPath = std::make_unique<TCHAR[]>(reqLen + 1);
				GetLongPathName(shortPath.get(), dstfullPath.get(), reqLen);
			}
		}
		dst = dstfullPath.get();
	}
	std::unique_ptr<TCHAR[]> srcfullPath;
	if (src)
	{
		reqLen = GetFullPathName(src, 0, nullptr, nullptr);
		srcfullPath = std::make_unique<TCHAR[]>(reqLen + 1);
		GetFullPathName(src, reqLen, srcfullPath.get(), nullptr);
		shortlen = GetShortPathName(srcfullPath.get(), nullptr, 0);
		if (shortlen)
		{
			auto shortPath = std::make_unique<TCHAR[]>(shortlen + 1);
			if (GetShortPathName(srcfullPath.get(), shortPath.get(), shortlen+1))
			{
				reqLen = GetLongPathName(shortPath.get(), nullptr, 0);
				srcfullPath = std::make_unique<TCHAR[]>(reqLen + 1);
				GetLongPathName(shortPath.get(), srcfullPath.get(), reqLen);
			}
		}
		src = srcfullPath.get();
	}

	if (!PathFileExists(wc))
	{
		_tprintf(L"Directory or file '%s' does not exist\n", wc);
		if (wcschr(wc, '\"')) // dir contains a quotation mark
		{
			_tprintf(L"The Path contains a quotation mark.\n");
			_tprintf(L"this indicates a problem when calling GitWCRev from an interpreter which treats\n");
			_tprintf(L"a backslash char specially.\n");
			_tprintf(L"Try using double backslashes or insert a dot after the last backslash when\n");
			_tprintf(L"calling GitWCRev\n");
			_tprintf(L"Examples:\n");
			_tprintf(L"GitWCRev \"path to wc\\\\\"\n");
			_tprintf(L"GitWCRev \"path to wc\\.\"\n");
		}
		return ERR_FNF; // dir does not exist
	}
	std::unique_ptr<char[]> pBuf;
	DWORD readlength = 0;
	size_t filelength = 0;
	size_t maxlength  = 0;
	if (dst)
	{
		// open the file and read the contents
		CAutoFile hFile = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0);
		if (!hFile)
		{
			_tprintf(L"Unable to open input file '%s'\n", src);
			return ERR_OPEN; // error opening file
		}
		filelength = GetFileSize(hFile, nullptr);
		if (filelength == INVALID_FILE_SIZE)
		{
			_tprintf(L"Could not determine file size of '%s'\n", src);
			return ERR_READ;
		}
		maxlength = filelength + 8192; // We might be increasing file size.
		pBuf = std::make_unique<char[]>(maxlength);
		if (!pBuf)
		{
			_tprintf(L"Could not allocate enough memory!\n");
			return ERR_ALLOC;
		}
		if (!ReadFile(hFile, pBuf.get(), static_cast<DWORD>(filelength), &readlength, nullptr))
		{
			_tprintf(L"Could not read the file '%s'\n", src);
			return ERR_READ;
		}
		if (readlength != filelength)
		{
			_tprintf(L"Could not read the file '%s' to the end!\n", src);
			return ERR_READ;
		}
	}

	git_libgit2_init();

	// Now check the status of every file in the working copy
	// and gather revision status information in GitStat.
	int err = GetStatus(wc, GitStat);

	git_libgit2_shutdown();
	if (err)
		return err;

	if (!bQuiet)
	{
		char wcfull_oem[MAX_PATH] = { 0 };
		CharToOem(wc, wcfull_oem);
		_tprintf(L"GitWCRev: '%hs'\n", wcfull_oem);
	}

	if (bErrOnMods && (GitStat.HasMods || GitStat.bHasSubmoduleNewCommits || (bErrResursively && GitStat.bHasSubmoduleMods)))
	{
		if (!bQuiet)
			_tprintf(L"Working tree has uncomitted modifications!\n");
		return ERR_GIT_MODS;
	}
	if (bErrOnUnversioned && (GitStat.HasUnversioned || (bErrResursively && GitStat.bHasSubmoduleUnversioned)))
	{
		if (!bQuiet)
			_tprintf(L"Working tree has unversioned items!\n");
		return ERR_GIT_UNVER;
	}

	if (!bQuiet)
	{
		_tprintf(L"HEAD is %s\n", CUnicodeUtils::StdGetUnicode(GitStat.HeadHashReadable).c_str());

		if (GitStat.HasMods)
			_tprintf(L"Uncommitted modifications found\n");

		if (GitStat.HasUnversioned)
			_tprintf(L"Unversioned items found\n");
	}

	if (!dst)
		return 0;

	// now parse the file contents for version defines.
	size_t index = 0;
	while (InsertRevision(VERDEF, pBuf.get(), index, filelength, maxlength, &GitStat));
	index = 0;
	while (InsertRevisionW(TEXT(VERDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, &GitStat));

	index = 0;
	while (InsertRevision(VERDEFSHORT, pBuf.get(), index, filelength, maxlength, &GitStat));
	index = 0;
	while (InsertRevisionW(TEXT(VERDEFSHORT), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, &GitStat));

	index = 0;
	while (InsertDate(DATEDEF, pBuf.get(), index, filelength, maxlength, GitStat.HeadTime));
	index = 0;
	while (InsertDateW(TEXT(DATEDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.HeadTime));

	index = 0;
	while (InsertDate(DATEDEFUTC, pBuf.get(), index, filelength, maxlength, GitStat.HeadTime));
	index = 0;
	while (InsertDateW(TEXT(DATEDEFUTC), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.HeadTime));

	index = 0;
	while (InsertDate(DATEWFMTDEF, pBuf.get(), index, filelength, maxlength, GitStat.HeadTime));
	index = 0;
	while (InsertDateW(TEXT(DATEWFMTDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.HeadTime));
	index = 0;
	while (InsertDate(DATEWFMTDEFUTC, pBuf.get(), index, filelength, maxlength, GitStat.HeadTime));
	index = 0;
	while (InsertDateW(TEXT(DATEWFMTDEFUTC), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.HeadTime));

	index = 0;
	while (InsertDate(NOWDEF, pBuf.get(), index, filelength, maxlength, USE_TIME_NOW));
	index = 0;
	while (InsertDateW(TEXT(NOWDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, USE_TIME_NOW));

	index = 0;
	while (InsertDate(NOWDEFUTC, pBuf.get(), index, filelength, maxlength, USE_TIME_NOW));
	index = 0;
	while (InsertDateW(TEXT(NOWDEFUTC), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, USE_TIME_NOW));

	index = 0;
	while (InsertDate(NOWWFMTDEF, pBuf.get(), index, filelength, maxlength, USE_TIME_NOW));
	index = 0;
	while (InsertDateW(TEXT(NOWWFMTDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, USE_TIME_NOW));

	index = 0;
	while (InsertDate(NOWWFMTDEFUTC, pBuf.get(), index, filelength, maxlength, USE_TIME_NOW));
	index = 0;
	while (InsertDateW(TEXT(NOWWFMTDEFUTC), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, USE_TIME_NOW));

	index = 0;
	while (InsertBoolean(MODDEF, pBuf.get(), index, filelength, GitStat.HasMods || GitStat.bHasSubmoduleNewCommits));
	index = 0;
	while (InsertBooleanW(TEXT(MODDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.HasMods || GitStat.bHasSubmoduleNewCommits));

	index = 0;
	while (InsertBoolean(UNVERDEF, pBuf.get(), index, filelength, GitStat.HasUnversioned));
	index = 0;
	while (InsertBooleanW(TEXT(UNVERDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.HasUnversioned));

	index = 0;
	while (InsertBoolean(ISTAGGED, pBuf.get(), index, filelength, GitStat.bIsTagged));
	index = 0;
	while (InsertBooleanW(TEXT(ISTAGGED), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.bIsTagged));

	index = 0;
	while (InsertBoolean(ISINGIT, pBuf.get(), index, filelength, GitStat.bIsGitItem));
	index = 0;
	while (InsertBooleanW(TEXT(ISINGIT), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.bIsGitItem));

	index = 0;
	while (InsertBoolean(SUBDEF, pBuf.get(), index, filelength, GitStat.bHasSubmodule));
	index = 0;
	while (InsertBooleanW(TEXT(SUBDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.bHasSubmodule));

	index = 0;
	while (InsertBoolean(SUBUP2DATEDEF, pBuf.get(), index, filelength, !GitStat.bHasSubmoduleNewCommits));
	index = 0;
	while (InsertBooleanW(TEXT(SUBUP2DATEDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, !GitStat.bHasSubmoduleNewCommits));

	index = 0;
	while (InsertBoolean(MODINSUBDEF, pBuf.get(), index, filelength, GitStat.bHasSubmoduleMods));
	index = 0;
	while (InsertBooleanW(TEXT(MODINSUBDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.bHasSubmoduleMods));

	index = 0;
	while (InsertBoolean(UNVERINSUBDEF, pBuf.get(), index, filelength, GitStat.bHasSubmoduleUnversioned));
	index = 0;
	while (InsertBooleanW(TEXT(UNVERINSUBDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.bHasSubmoduleUnversioned));

	index = 0;
	while (InsertBoolean(UNVERFULLDEF, pBuf.get(), index, filelength, GitStat.HasUnversioned || GitStat.bHasSubmoduleUnversioned));
	index = 0;
	while (InsertBooleanW(TEXT(UNVERFULLDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.HasUnversioned || GitStat.bHasSubmoduleUnversioned));

	index = 0;
	while (InsertBoolean(MODFULLDEF, pBuf.get(), index, filelength, GitStat.HasMods || GitStat.bHasSubmoduleMods || GitStat.bHasSubmoduleUnversioned));
	index = 0;
	while (InsertBooleanW(TEXT(MODFULLDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.HasMods || GitStat.bHasSubmoduleMods || GitStat.bHasSubmoduleUnversioned));

	index = 0;
	while (InsertBoolean(MODSFILEDEF, pBuf.get(), index, filelength, GitStat.HasMods));
	index = 0;
	while (InsertBooleanW(TEXT(MODSFILEDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, GitStat.HasMods));

	index = 0;
	while (InsertNumber(VALDEF, pBuf.get(), index, filelength, maxlength, GitStat.NumCommits));
	index = 0;
	while (InsertNumberW(TEXT(VALDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.NumCommits));

	index = 0;
	while (InsertNumber(VALDEFAND, pBuf.get(), index, filelength, maxlength, GitStat.NumCommits));
	index = 0;
	while (InsertNumberW(TEXT(VALDEFAND), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.NumCommits));

	index = 0;
	while (InsertNumber(VALDEFOFFSET1, pBuf.get(), index, filelength, maxlength, GitStat.NumCommits));
	index = 0;
	while (InsertNumberW(TEXT(VALDEFOFFSET1), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.NumCommits));

	index = 0;
	while (InsertNumber(VALDEFOFFSET2, pBuf.get(), index, filelength, maxlength, GitStat.NumCommits));
	index = 0;
	while (InsertNumberW(TEXT(VALDEFOFFSET2), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.NumCommits));

	index = 0;
	while (InsertText(BRANCHDEF, pBuf.get(), index, filelength, maxlength, GitStat.CurrentBranch));
	index = 0;
	while (InsertTextW(TEXT(BRANCHDEF), reinterpret_cast<wchar_t*>(pBuf.get()), index, filelength, maxlength, GitStat.CurrentBranch));

	CAutoFile hFile = CreateFile(dst, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 0, 0);
	if (!hFile)
	{
		_tprintf(L"Unable to open output file '%s' for writing\n", dst);
		return ERR_OPEN;
	}

	size_t filelengthExisting = GetFileSize(hFile, nullptr);
	BOOL sameFileContent = FALSE;
	if (filelength == filelengthExisting)
	{
		DWORD readlengthExisting = 0;
		auto pBufExisting = std::make_unique<char[]>(filelength);
		if (!ReadFile(hFile, pBufExisting.get(), static_cast<DWORD>(filelengthExisting), &readlengthExisting, nullptr))
		{
			_tprintf(L"Could not read the file '%s'\n", dst);
			return ERR_READ;
		}
		if (readlengthExisting != filelengthExisting)
		{
			_tprintf(L"Could not read the file '%s' to the end!\n", dst);
			return ERR_READ;
		}
		sameFileContent = (memcmp(pBuf.get(), pBufExisting.get(), filelength) == 0);
	}

	// The file is only written if its contents would change.
	// this object prevents the timestamp from changing.
	if (!sameFileContent)
	{
		SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

		WriteFile(hFile, pBuf.get(), static_cast<DWORD>(filelength), &readlength, nullptr);
		if (readlength != filelength)
		{
			_tprintf(L"Could not write the file '%s' to the end!\n", dst);
			return ERR_READ;
		}

		if (!SetEndOfFile(hFile))
		{
			_tprintf(L"Could not truncate the file '%s' to the end!\n", dst);
			return ERR_READ;
		}
	}

	return 0;
}
