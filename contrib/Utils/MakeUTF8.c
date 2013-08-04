/******************************************************************************
    MakeUTF8.c

Copyright (C) 2002 - 2006, 2013 Simon Large

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

Description:
This program checks text files for the presence of a byte-order-mark (BOM)
and for a UTF-8 encoding indicator in the XML version tag. You can also
opt to add either or both of these features.

Use:
MakeUTF8 [ -b ] [ -x ] file [ file ... ]
Wildcard filenames are supported. Subdirectory recursion is not at present.
-b option adds/corrects BOM in file if not already present.
-x option adds/corrects XML tag if not already present.
With no options, the current stateis reported but nothing is changed.

Example:
MakeUTF8 -b *.xml tsvn_dug\*.xml
Fixes BOMs (but not XML tags) in all .xml files in the current directory,
and in the tsvn_dug subdirectory.

This program has only been built using the Microsoft Visual C++ compiler.
Library calls for finding files (_findfirst64) will probably need to be
changed in other environments.

No special compiler options were used. CL MakeUTF8.c works OK.
******************************************************************************/

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>

// Status flags returned from the file processor.
#define ADD_BOM     1       // BOM is missing
#define DOUBLE_BOM  2       // Double BOM found
#define XML_TAG     4       // XML tag missing, or UTF-8 not included
#define FIXED_BOM   64      // BOM has been added or fixed
#define FIXED_TAG   128     // XML tag has been added or fixed

char *help =
    "MakeUTF8     Version 1.1\n"
    "Add UTF-8 byte-order-mark and XML-tag to start of text file.\n\n"
    "Use: MakeUTF8 [ -b ] [ -x ] file [ file ... ]\n"
    "     -b option adds/corrects BOM in file if not already present\n"
    "     -x option adds/corrects XML tag if not already present\n"
    "     With no options, just report current state\n\n";

int ProcessFile(const char *FName, const char *TName, int Action);

int main(int argc, char *argv[])
{
    int n, Action = 0, Result = 0;
    char Path[_MAX_PATH], Temp[_MAX_PATH];
    char *FName;
    struct __finddata64_t FileInfo;
    intptr_t hFile;

    if (argc < 2)
    {
        fprintf(stderr, "%s", help);
        exit(0);
    }

    for (n = 1; n < argc; n++)
    {
        if (_stricmp(argv[n], "-b") == 0)
        {
            Action |= ADD_BOM | DOUBLE_BOM;
            continue;
        }
        if (_stricmp(argv[n], "-x") == 0)
        {
            Action |= XML_TAG;
            continue;
        }
        // Unscramble wildcard filenames
        if ((hFile = _findfirst64(argv[n], &FileInfo)) != -1)
        {
            printf("BOM\tXML-tag\tFile\n");
            printf("--------------------\n");
            // Extract path from original argument.
            strcpy(Path, argv[n]);
            // Set FName to point to filename portion of path
            FName = strrchr(Path, '\\');
            if (FName == NULL)
            {
                FName = strrchr(Path, '/');
            }
            if (FName == NULL)
            {
                FName = strrchr(Path, ':');
            }
            if (FName == NULL)
            {
                FName = Path;
            }
            else
            {
                ++FName;
            }

            // Process all matching files.
            do
            {
                if (!(FileInfo.attrib & _A_SUBDIR))
                {
                    // Append filename to path
                    char *p;
                    strcpy(FName, FileInfo.name);
                    // Create temp filename by replacing extension with $$$
                    strcpy(Temp, Path);
                    p = strrchr(Temp, '.');
                    if (p != NULL)
                    {
                        *p = '\0';      // Trim off extension
                    }
                    strcat(Temp, ".$$$");
                    Result = ProcessFile(Path, Temp, Action);
                    if (Result < 0)
                    {
                        break;          // Failed.
                    }
                    // Show results of analysis / repair
                    if (Result & ADD_BOM)
                    {
                        if (Result & FIXED_BOM)
                        {
                            p = "Added";
                        }
                        else
                        {
                            p = "None";
                        }
                    }
                    else if (Result & DOUBLE_BOM)
                    {
                        if (Result & FIXED_BOM)
                        {
                            p = "Fixed";
                        }
                        else
                        {
                            p = "Multi";
                        }
                    }
                    else
                    {
                        p = "OK";
                    }
                    printf("%s\t", p);
                    if (Result & XML_TAG)
                    {
                        if (Result & FIXED_TAG)
                        {
                            p = "Fixed";
                        }
                        else
                        {
                            p = "None";
                        }
                    }
                    else
                    {
                        p = "OK";
                    }
                    printf("%s\t%s\n", p, FileInfo.name);
                }
            }
            while (_findnext64(hFile, &FileInfo) == 0);
            _findclose(hFile);
        }
    }
    exit((Result < 0) ? 1 : 0);
}

// These 3 bytes are the BOM we want
char BOMbuf[3] = { 0xef, 0xbb, 0xbf };

// This is the XML tag we want
char *UTFtag = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";

// Read this amount at start of file to check for BOM and tag
#define BUFSIZE 2048

int ProcessFile(const char *FName, const char *TName, int Action)
{
    FILE *fp, *fpout;
    char Buffer[BUFSIZE + 1024];
    size_t Len;
    size_t NumRead;
    int Changed = 0, Checked = 0;
    size_t UTFtaglen;
    char *TagStart, *TagStop;
    char *AfterBOM = Buffer;

    if ((fp = fopen(FName, "r")) == NULL)
    {
        return -1;
    }

    // Check if output file exists already
    if ((fpout = fopen(TName, "r")) != NULL) {
        fprintf(stderr, "%s:\tTemp file already exists\n", TName);
        fclose(fpout);
        fclose(fp);
        return -1;
    }

    while ((NumRead = fread(Buffer, 1, BUFSIZE, fp)) > 0)
    {
        if (!Checked)
        {
            Checked = 1;
            // Check for no BOM or multiple BOM.
            if (memcmp(BOMbuf, Buffer, 3) == 0)
            {
                // BOM already exists.
                AfterBOM = Buffer + 3;
                while (memcmp(BOMbuf, AfterBOM, 3) == 0)
                {
                    // Multiple BOM found.
                    Changed |= DOUBLE_BOM;
                    if (Action & DOUBLE_BOM)
                    {
                        // Delete BOM from source
                        NumRead -= 3;
                        memmove(Buffer, AfterBOM, NumRead);
                        Buffer[NumRead] = '\0';
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                // No BOM found.
                Changed |= ADD_BOM;
                if (Action & ADD_BOM)
                {
                    // Add BOM to source
                    AfterBOM = Buffer + 3;
                    memmove(AfterBOM, Buffer, NumRead);
                    memcpy(Buffer, BOMbuf, 3);
                    NumRead += 3;
                }
            }

            // Check for XML tag <?xml version="1.0" encoding="UTF-8"?>
            Buffer[NumRead] = '\0';     // Add null terminator for string search.
            UTFtaglen = strlen(UTFtag);
            if (strstr(Buffer, "encoding=\"UTF-8\"") == NULL)
            {
                // No XML tag found.
                Changed |= XML_TAG;
                if (Action & XML_TAG)
                {
                    TagStart = strstr(Buffer, "<?xml version");
                    if (TagStart != NULL)
                    {
                        TagStop = strstr(TagStart, "?>");
                        if (TagStop != NULL)
                        {
                            // Version tag present without UTF-8
                            Len = UTFtaglen - (TagStop - TagStart + 2);
                            if (Len != 0)
                            {
                                // Expand/contract the space
                                memmove(TagStop + Len, TagStop, NumRead - (TagStop - Buffer));
                                NumRead += Len;
                            }
                            memcpy(TagStart, UTFtag, UTFtaglen);
                        }
                        else
                        {
                            // Version tag is not terminated. Cannot fix.
                            Action &= ~XML_TAG;
                        }
                    }
                    else
                    {
                        // No version tag found. Add one after BOM, with newline.
                        memmove(AfterBOM + UTFtaglen + 1, AfterBOM, NumRead);
                        memcpy(AfterBOM, UTFtag, UTFtaglen);
                        AfterBOM[UTFtaglen] = '\n';
                        NumRead += UTFtaglen + 1;
                    }
                }
            }

            if (!(Action & Changed))
            {
                // If no problems marked for fixing, leave it here.
                break;
            }
            // Changes made - open a temp file for the BOM'ed version
            if ((fpout = fopen(TName, "w")) == NULL)
            {
                fprintf(stderr, "Cannot open temp file\n");
                fclose(fp);
                return -1;
            }
        }
        if (fwrite(Buffer, 1, NumRead, fpout) != NumRead)
        {
            fprintf(stderr, "Error writing to temp file\n");
            fclose(fpout);
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);

    // If changes have been made, replace original file with temp file.
    if (Changed & Action)
    {
        // Replace original with temp file
        if (fpout)
        {
            fclose(fpout);
        }
        if (remove(FName) != 0)
        {
            fprintf(stderr, "Cannot delete original file\n");
            return -1;
        }
        if (rename(TName, FName) != 0)
        {
            fprintf(stderr, "Cannot replace original file with fixed version\n");
            return -1;
        }
        // Add flags to indicate what we have actually fixed
        if (Changed & Action & (DOUBLE_BOM | ADD_BOM))
        {
            Changed |= FIXED_BOM;
        }
        if (Changed & Action & XML_TAG)
        {
            Changed |= FIXED_TAG;
        }
    }

    return Changed;
}
