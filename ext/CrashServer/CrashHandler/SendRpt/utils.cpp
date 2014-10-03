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

#include "StdAfx.h"
#include "utils.h"

std::pair<LPCVOID, size_t> ExtractDataFromResource(HMODULE hImage, DWORD resId)
{
    HRSRC hRes = FindResource(hImage, MAKEINTRESOURCE(resId), RT_RCDATA);
    if (!hRes)
        throw std::runtime_error("failed to find data in resources");

    HGLOBAL hResGlobal = LoadResource(hImage, hRes);
    if (!hResGlobal)
        throw std::runtime_error("failed to load data from resources");

    return std::make_pair(LockResource(hResGlobal), SizeofResource(hImage, hRes));
}