// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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

// include base classes

#include "LogCacheGlobals.h"
#include "CopyFilterOptions.h"
#include "revisiongraphoptionsimpl.h"
#include "Resource.h"

/** Remove all branches / tags that have been deleted and have
* no surviving copy.
*/

using namespace LogCache;

class CRemovePathsBySubString 
    : public CRevisionGraphOptionImpl< ICopyFilterOption
                                     , 55
                                     , 0>
{
private:

    /// all paths with these sub-strings shall be removed

    std::set<std::string> filterPaths;

    /// cache results for fully cached paths

    enum PathClassification
    {
        UNKNOWN = 0, 
        KEEP = 1, 
        REMOVE = 3
    };

    mutable std::vector<PathClassification> pathClassification;

public:

    /// construction

    CRemovePathsBySubString (CRevisionGraphOptionList& list);

    /// access to the sub-string container

    const std::set<std::string>& GetFilterPaths() const;
    std::set<std::string>& GetFilterPaths();

    /// implement ICopyFilterOption

    virtual EResult ShallRemove (const CFullGraphNode* node) const;
    virtual void Prepare();
};
