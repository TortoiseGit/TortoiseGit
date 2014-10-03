// Copyright 2014 Idol Software, Inc.
//
// This file is part of Doctor Dump SDK.
//
// Doctor Dump SDK is free software: you can redistribute it and/or modify
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

extern "C" voidpf ZCALLBACK win32_open64_file_funcW(voidpf opaque,const void* filename,int mode);

Zip::Zip(LPCWSTR pszFilename, bool append)
{
    zlib_filefunc_def ffunc;
    fill_win32_filefunc(&ffunc);
#if ZLIB128
    // Hack used to force Unicode version of CreateFile in ANSI zipOpen2 function
    ffunc.zopen_file = (open_file_func) win32_open64_file_funcW;
    m_zf = zipOpen2((const char*)pszFilename, append ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE, NULL, &ffunc);
#else
    m_zf = zipOpen2((const char*)CW2A(pszFilename), append ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE, NULL, &ffunc);
#endif

    if (!m_zf)
        throw runtime_error(append ? "failed to append zip file" : "failed to create zip file");
}

Zip::~Zip(void)
{
    zipClose(m_zf, NULL);
}

void Zip::AddFile(LPCWSTR pszFilename, LPCWSTR pszFilenameInZip, bool* cancel)
{
    zip_fileinfo zi = {};

    WIN32_FIND_DATAW ff;
    HANDLE hFind = FindFirstFileW(pszFilename, &ff);
    if (hFind == INVALID_HANDLE_VALUE)
        throw runtime_error("file to add to zip not found");

    if (!pszFilenameInZip)
    {
        LPCWSTR slashPos = wcsrchr(pszFilename, L'\\');
        if (!slashPos)
            slashPos = wcsrchr(pszFilename, L'/');
        pszFilenameInZip = slashPos ? slashPos + 1 : pszFilename;
    }

    FILETIME ftLocal;
    FileTimeToLocalFileTime(&(ff.ftLastWriteTime), &ftLocal);
    FileTimeToDosDateTime(&ftLocal, ((LPWORD)&zi.dosDate)+1, ((LPWORD)&zi.dosDate)+0);
    FindClose(hFind);

    int err = zipOpenNewFileInZip3(m_zf, CW2A(pszFilenameInZip), &zi,
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
        if (cancel && *cancel)
            throw std::runtime_error("canceled");
    }

    if (ZIP_OK != zipCloseFileInZip(m_zf))
        throw runtime_error("failed to finalize file in zip");
}

static unzFile Unzip_Open(LPCWSTR pszFilename)
{
    zlib_filefunc_def ffunc;
    fill_win32_filefunc(&ffunc);
    unzFile uf = unzOpen2(CW2CT(pszFilename), &ffunc);
    if (!uf)
        throw runtime_error("failed to open zip file");
    return uf;
}

static CStringW Unzip_GetCurentFilePath(unzFile uf, FILETIME& ftLocal)
{
    unz_file_info file_info;
    CStringA pathA;
    const int pathLen = 256;
    if (UNZ_OK != unzGetCurrentFileInfo(uf, &file_info, pathA.GetBuffer(pathLen), pathLen, NULL, 0, NULL, 0))
        throw runtime_error("failed to unzGetCurrentFileInfo");
    pathA.ReleaseBuffer(-1);
    DosDateTimeToFileTime((WORD)(file_info.dosDate>>16), (WORD)file_info.dosDate, &ftLocal);
    return CStringW(pathA);
}

static void Unzip_ExtractFile(unzFile uf, const CStringW& path, DWORD flagsAndAttributes, FILETIME ftLocal)
{
    try
    {
        if (UNZ_OK != unzOpenCurrentFilePassword(uf, NULL))
            throw runtime_error("failed to unzOpenCurrentFilePassword");

        CAtlFile file(CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, flagsAndAttributes, NULL));
        if (file == INVALID_HANDLE_VALUE)
            throw runtime_error("failed to CreateFile");

        vector<BYTE> buf(8192);
        while (1)
        {
            int size = unzReadCurrentFile(uf, &buf[0], (unsigned int)buf.size());
            if (size < 0)
                throw runtime_error("failed to unzReadCurrentFile");
            if (size == 0)
                break;

            if (FAILED(file.Write(&buf[0], size)))
                throw runtime_error("failed to WriteFile");
        }

        FILETIME ftm, ftCreate, ftLastAcc, ftLastWrite;
        GetFileTime(file, &ftCreate, &ftLastAcc, &ftLastWrite);
        LocalFileTimeToFileTime(&ftLocal, &ftm);
        SetFileTime(file, &ftm, &ftLastAcc, &ftm);

        if (UNZ_OK != unzCloseCurrentFile(uf))
            throw runtime_error("failed to unzCloseCurrentFile");
    }
    catch (...)
    {
        DeleteFileW(path);
    }
}

std::vector<CStringW> Unzip::Extract(LPCWSTR pszFilename, LPCWSTR pszFolder, std::function<bool(LPCWSTR filePath, DWORD& flagsAndAttributes)> predicate)
{
    std::vector<CStringW> result;

    unzFile uf = NULL;
    try
    {
        uf = Unzip_Open(pszFilename);

        unz_global_info gi;
        if (UNZ_OK != unzGetGlobalInfo(uf, &gi))
            throw runtime_error("failed to unzGetGlobalInfo");

        for (uLong i = 0; i < gi.number_entry; ++i)
        {
            FILETIME ftLocal;
            CStringW path = CStringW(pszFolder) + L'\\' + Unzip_GetCurentFilePath(uf, ftLocal);

            CStringW filename = path.Mid(path.ReverseFind(L'\\') + 1);
            if (filename.IsEmpty())
            {
                if (!CreateDirectoryW(path, NULL))
                    throw runtime_error("failed to CreateDirectory");
                continue;
            }

            DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
            if (predicate(path, flagsAndAttributes))
            {
                Unzip_ExtractFile(uf, path, flagsAndAttributes, ftLocal);
                result.push_back(path);
            }

            if (i + 1 < gi.number_entry && UNZ_OK != unzGoToNextFile(uf))
                throw runtime_error("failed to unzGoToNextFile");
        }
        unzClose(uf);

        return result;
    }
    catch (...)
    {
        if (uf != NULL)
            unzClose(uf);
        for each (auto file in result)
            DeleteFileW(file);
        throw;
    }
}

std::vector<CStringW> Unzip::Extract(LPCWSTR pszFilename, LPCWSTR pszFolder)
{
    return Extract(pszFilename, pszFolder, [](LPCWSTR filePath, DWORD& flagsAndAttributes) { return true; });
}

bool DeflateBuffer(const BYTE* buffer, size_t bufferLen, std::vector<BYTE>& outBuffer, const char* dictionary)
{
    z_stream stream = {};
    deflateInit(&stream, Z_BEST_COMPRESSION);
    outBuffer.resize(deflateBound(&stream, static_cast<uLong>(bufferLen)));
    if (dictionary)
        deflateSetDictionary(&stream, (const Bytef*)dictionary, static_cast<uInt>(strlen(dictionary)));
    stream.next_in = (Bytef*)buffer;
    stream.avail_in = static_cast<uInt>(bufferLen);
    stream.next_out = &outBuffer[0];
    stream.avail_out = static_cast<uInt>(outBuffer.size());
    while (1)
    {
        switch (deflate(&stream, Z_FINISH))
        {
        case Z_STREAM_END:
            outBuffer.resize(outBuffer.size() - stream.avail_out);
            deflateEnd(&stream);
            return true;
        case Z_OK:
            stream.avail_out += static_cast<uInt>(outBuffer.size());
            outBuffer.resize(outBuffer.size() * 2);
            stream.next_out = &outBuffer[outBuffer.size() - stream.avail_out];
            break;
        default:
            deflateEnd(&stream);
            return false;
        }
    }
}

bool InflateBuffer(const BYTE* buffer, size_t bufferLen, std::vector<BYTE>& outBuffer, const char* dictionary)
{
    z_stream stream = {};
    stream.next_in = (Bytef*)buffer;
    stream.avail_in = static_cast<uInt>(bufferLen);
    inflateInit(&stream);
    outBuffer.resize(2*bufferLen);
    stream.next_out = &outBuffer[0];
    stream.avail_out = static_cast<uInt>(outBuffer.size());
    while (1)
    {
        switch (inflate(&stream, 0))
        {
        case Z_NEED_DICT:
            if (!dictionary || Z_OK != inflateSetDictionary(&stream, (const Bytef*)dictionary, static_cast<uInt>(strlen(dictionary))))
            {
                inflateEnd(&stream);
                return false;
            }
            break;
        case Z_STREAM_END:
            outBuffer.resize(outBuffer.size() - stream.avail_out);
            inflateEnd(&stream);
            return true;
        case Z_OK:
            stream.avail_out += static_cast<uInt>(outBuffer.size());
            outBuffer.resize(outBuffer.size() * 2);
            stream.next_out = &outBuffer[outBuffer.size() - stream.avail_out];
            break;
        default:
            inflateEnd(&stream);
            return false;
        }
    }
}
