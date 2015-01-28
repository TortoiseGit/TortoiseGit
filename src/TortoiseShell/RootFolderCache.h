// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI

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
#pragma once

#include "IntegritySession.h"

class RootFolderCache {
public:
	RootFolderCache(const IntegritySession& integritySession) : integritySession(integritySession){};

	bool isPathControlled(std::wstring path);
	void forceRefresh();
	bool refreshIfStale();

private:
	const IntegritySession& integritySession;
	std::mutex lockObject;
	std::chrono::time_point<std::chrono::system_clock> lastRefresh;
	std::vector<std::wstring> rootFolders;

	void updateFoldersList();
};