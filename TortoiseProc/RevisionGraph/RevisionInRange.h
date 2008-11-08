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
* no surviving copy. Upper and lower bounds are inclusive.
*/

using namespace LogCache;

class CRevisionInRange 
    : public CRevisionGraphOptionImpl< ICopyFilterOption
                                     , 50
                                     , 0>
{
private:

    /// NO_REVISION meas no limit

    revision_t lowerLimit;
    revision_t upperLimit;

public:

    /// construction

    CRevisionInRange (CRevisionGraphOptionList& list);

    /// get / set limits

    revision_t GetLowerLimit() const;
    void SetLowerLimit (revision_t limit);

    revision_t GetUpperLimit() const;
    void SetUpperLimit (revision_t limit);

    /// implement ICopyFilterOption

    virtual EResult ShallRemove (const CFullGraphNode* node) const;
};
