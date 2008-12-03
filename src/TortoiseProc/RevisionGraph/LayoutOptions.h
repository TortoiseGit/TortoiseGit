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

// forward declarations

class IRevisionGraphLayout;

/**
* Extends the base interface with a method that has full
* modifying access to a given graph layout. In most cases,
* the derived class will require \ref layout to be of
* a particular sub-class and use that classes' interface.
*/

class ILayoutOption : public IRevisionGraphOption
{
public:

    /// cast @a layout pointer to the respective modification
    /// interface and write the data.

    virtual void ApplyTo (IRevisionGraphLayout* layout) = 0;
};

/**
* Filtered sub-set of \ref CAllRevisionGraphOptions.
* It applies all options in the order defined by their \ref priority.
* The option instances may transform the layout any way they consider fit.
*
* Contains only \ref ILayoutOption instances.
*/

class CLayoutOptions : public CRevisionGraphOptionList
{
private:

    std::vector<ILayoutOption*> options;

public:

    // construction / destruction

    CLayoutOptions (const std::vector<ILayoutOption*>& options);
    virtual ~CLayoutOptions() {}

    /// apply all filters 

    void Apply (IRevisionGraphLayout* layout) const;
};
