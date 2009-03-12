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
#include "StdAfx.h"
#include "RevisionInRange.h"
#include "FullGraphNode.h"

// construction

CRevisionInRange::CRevisionInRange (CRevisionGraphOptionList& list)
    : inherited (list)
    , lowerLimit (static_cast<revision_t>(NO_REVISION))
    , upperLimit (static_cast<revision_t>(NO_REVISION))
{
}

// get / set limits

revision_t CRevisionInRange::GetLowerLimit() const
{
    return lowerLimit;
}

void CRevisionInRange::SetLowerLimit (revision_t limit)
{
    lowerLimit = limit;
}

revision_t CRevisionInRange::GetUpperLimit() const
{
    return upperLimit;
}

void CRevisionInRange::SetUpperLimit (revision_t limit)
{
    upperLimit = limit;
}

// implement IRevisionGraphOption

bool CRevisionInRange::IsActive() const
{
    return ((lowerLimit > 1) && (lowerLimit != NO_REVISION))
        || (upperLimit != NO_REVISION);
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CRevisionInRange::ShallRemove (const CFullGraphNode* node) const
{
    // out of revision range?

    if ((lowerLimit != NO_REVISION) && (node->GetRevision() < lowerLimit))
        return ICopyFilterOption::REMOVE_NODE;

    if ((upperLimit != NO_REVISION) && (node->GetRevision() > upperLimit))
        return ICopyFilterOption::REMOVE_NODE;

    return ICopyFilterOption::KEEP_NODE;
}
