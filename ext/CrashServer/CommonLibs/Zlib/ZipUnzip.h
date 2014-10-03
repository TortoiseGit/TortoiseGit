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

#pragma once

#include <wtypes.h>
#include <vector>
#include <functional>

typedef void* zipFile;
typedef void* unzFile;

class Zip
{
public:
    Zip(LPCWSTR pszFilename, bool append = false);
    ~Zip();

    void AddFile(LPCWSTR pszFilename, LPCWSTR pszFilenameInZip = nullptr, bool* cancel = nullptr);

private:
    zipFile m_zf;
};

class Unzip
{
public:
    static std::vector<CStringW> Extract(LPCWSTR pszFilename, LPCWSTR pszFolder, std::function<bool(LPCWSTR filePath, DWORD& flagsAndAttributes)> predicate);
    static std::vector<CStringW> Extract(LPCWSTR pszFilename, LPCWSTR pszFolder);
};

bool DeflateBuffer(const BYTE* buffer, size_t bufferLen, std::vector<BYTE>& outBuffer, const char* dictionary);
bool InflateBuffer(const BYTE* buffer, size_t bufferLen, std::vector<BYTE>& outBuffer, const char* dictionary);