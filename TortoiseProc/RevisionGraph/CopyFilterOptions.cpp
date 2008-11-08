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
#include "CopyFilterOptions.h"

// construction

CCopyFilterOptions::CCopyFilterOptions 
    ( const std::vector<ICopyFilterOption*>& options)
    : options (options)
{
}

// apply all filters 

ICopyFilterOption::EResult 
CCopyFilterOptions::ShallRemove (const CFullGraphNode* node) const
{
    typedef std::vector<ICopyFilterOption*>::const_iterator IT;
    ICopyFilterOption::EResult result = ICopyFilterOption::KEEP_NODE;

    for ( IT iter = options.begin(), end = options.end()
        ; (iter != end) && (result == ICopyFilterOption::KEEP_NODE)
        ; ++iter)
    {
        result = (*iter)->ShallRemove (node);
    }

    return result;
};

