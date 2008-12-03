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

#include "RevisionGraphOptions.h"

/**
* Extends the base interface with a method that decides
* whether a node or sub-tree shall be copied at all.
*/

class ICopyFilterOption : public IRevisionGraphOption
{
public:

    enum EResult
    {
        KEEP_NODE = 0,
        REMOVE_NODE = 1,
        REMOVE_SUBTREE = 2,
        PRESERVE_NODE = 3  // shall not be removed, even by other filters
    };

    virtual EResult ShallRemove (const CFullGraphNode* node) const = 0;
};

/**
* Filtered sub-set of \ref CAllRevisionGraphOptions.
* It allows the aggregated application of all copy options 
* at once during the copy process.
*
* Contains only \ref ICopyFilterOption instances.
*/

class CCopyFilterOptions : public CRevisionGraphOptionList
{
private:

    std::vector<ICopyFilterOption*> options;

public:

    CCopyFilterOptions (const std::vector<ICopyFilterOption*>& options);
    virtual ~CCopyFilterOptions() {}

    /// apply all filters 

    ICopyFilterOption::EResult ShallRemove (const CFullGraphNode* node) const;
};
