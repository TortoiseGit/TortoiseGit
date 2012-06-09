// Copyright 2012 Idol Software, Inc.
//
// This file is part of CrashHandler library.
//
// CrashHandler library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <atlstr.h>
#include <atlfile.h>
#include <stdexcept>
#include "ZipUnzip.h"
#include "contrib\minizip\zip.h"
#include "contrib\minizip\unzip.h"
#include "contrib\minizip\iowin32.h"

using namespace std;

Zip::Zip(LPCWSTR pszFilename, bool append)
{
    zlib_filefunc_def ffunc;
    fill_win32_filefunc(&ffunc);
    m_zf = zipOpen2(CW2CT(pszFilename), append ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE, NULL, &ffunc);

    if (!m_zf)
        throw runtime_error(append ? "failed to append zip file" : "failed to create zip file");
}

Zip::~Zip(void)
{
    zipClose(m_zf, NULL);
}

void Zip::AddFile(LPCWSTR pszFilename, LPCWSTR pszFilenameInZip)
{
    zip_fileinfo zi = {};

    WIN32_FIND_DATAW ff;
    HANDLE hFind = FindFirstFileW(pszFilename, &ff);
    if (hFind == INVALID_HANDLE_VALUE)
        throw runtime_error("file to add to zip not found");

    FILETIME ftLocal;
    FileTimeToLocalFileTime(&(ff.ftLastWriteTime), &ftLocal);
    FileTimeToDosDateTime(&ftLocal, ((LPWORD)&zi.dosDate)+1, ((LPWORD)&zi.dosDate)+0);
    FindClose(hFind);

    int err = zipOpenNewFileInZip3(m_zf, CW2CT(pszFilenameInZip), &zi,
        NULL, 0, NULL, 0, NULL /* comment*/,
        Z_DEFLATED, Z_BEST_COMPRESSION, 0,
        -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
        NULL, 0);

    if (err != ZIP_OK)
        throw runtime_error("failed to create file in zip");


    CAtlFile hFile(CreateFileW(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    if (hFile == INVALID_HANDLE_VALUE)
        throw runtime_error("file to add to zip not found");

    BYTE buf[1024*8];
    DWORD dwReaded;
    while (SUCCEEDED(hFile.Read(buf, _countof(buf), dwReaded)) && dwReaded != 0)
    {
        err = zipWriteInFileInZip(m_zf, buf, dwReaded);
        if (err != ZIP_OK)
            throw runtime_error("failed to write to zip");
    }

    if (ZIP_OK != zipCloseFileInZip(m_zf))
        throw runtime_error("failed to finalize file in zip");
}

Unzip::Unzip()
    : m_uf(NULL)
{
}

Unzip::~Unzip()
{
    unzClose(m_uf);
}

void Unzip::Open(LPCWSTR pszFilename)
{
    zlib_filefunc_def ffunc;
    fill_win32_filefunc(&ffunc);
    m_uf = unzOpen2(CW2CT(pszFilename), &ffunc);

    if (!m_uf)
        throw runtime_error("failed to open zip file");
}

void Unzip::Extract(LPCWSTR pszFolder)
{
    unz_global_info gi;
    if (UNZ_OK != unzGetGlobalInfo(m_uf, &gi))
        throw runtime_error("failed to unzGetGlobalInfo");

    vector<BYTE> buf(8192);

    for (uLong i = 0; i < gi.number_entry; ++i)
    {
        unz_file_info file_info;
        CStringA pathA;
        const int pathLen = 256;
        if (UNZ_OK != unzGetCurrentFileInfo(m_uf, &file_info, pathA.GetBuffer(pathLen), pathLen, NULL, 0, NULL, 0))
            throw runtime_error("failed to unzGetCurrentFileInfo");
        pathA.ReleaseBuffer(-1);
        CStringW path = pszFolder;
        path += L'\\';
        path += pathA;

        CStringW filename = path.Mid(path.ReverseFind(L'\\') + 1);

        if (filename.IsEmpty())
        {
            if (!CreateDirectoryW(path, NULL))
                throw runtime_error("failed to CreateDirectory");
            continue;
        }

        if (UNZ_OK != unzOpenCurrentFilePassword(m_uf, NULL))
            throw runtime_error("failed to unzOpenCurrentFilePassword");

        CAtlFile file(CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
        if (file == INVALID_HANDLE_VALUE)
            throw runtime_error("failed to CreateFile");

        m_files.push_back(path);

        while (1)
        {
            int size = unzReadCurrentFile(m_uf, &buf[0], (unsigned int)buf.size());
            if (size < 0)
                throw runtime_error("failed to unzReadCurrentFile");
            if (size == 0)
                break;

            if (FAILED(file.Write(&buf[0], size)))
                throw runtime_error("failed to WriteFile");
        }

        FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;
        GetFileTime(file, &ftCreate, &ftLastAcc, &ftLastWrite);
        DosDateTimeToFileTime((WORD)(file_info.dosDate>>16), (WORD)file_info.dosDate,&ftLocal);
        LocalFileTimeToFileTime(&ftLocal, &ftm);
        SetFileTime(file, &ftm, &ftLastAcc, &ftm);

        if (UNZ_OK != unzCloseCurrentFile(m_uf))
            throw runtime_error("failed to unzCloseCurrentFile");

        if (i + 1 < gi.number_entry && UNZ_OK != unzGoToNextFile(m_uf))
            throw runtime_error("failed to unzGoToNextFile");
    }
}
