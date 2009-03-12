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

#include "./Containers/DictionaryBasedTempPath.h"

class CNodeClassification
{
public:

    /// classification for a revision graph tree node.
    /// Only the first section will be used by the CPathClassificator.
    /// All others are added by the revision graph.

    enum EFlags
    {
        IS_TRUNK           = 0x00000001,
        IS_BRANCH          = 0x00000002,
        IS_TAG             = 0x00000004,
        IS_OTHER           = 0x00000008,

        IS_ADDED           = 0x00000010,
        IS_MODIFIED        = 0x00000020,
        IS_DELETED         = 0x00000040,
        IS_RENAMED         = 0x00000080,     

        COPIES_TO_TRUNK    = 0x00000100,
        COPIES_TO_BRANCH   = 0x00000200,
        COPIES_TO_TAG      = 0x00000400,
        COPIES_TO_OTHER    = 0x00000800,

        COPIES_TO_ADDED    = 0x00001000,
        COPIES_TO_MODIFIED = 0x00002000,
        COPIES_TO_DELETED  = 0x00004000,
        COPIES_TO_RENAMED  = 0x00008000,

        ALL_COPIES_ADDED   = 0x00010000,
        ALL_COPIES_MODIFIED= 0x00020000,
        ALL_COPIES_DELETED = 0x00040000,
        ALL_COPIES_RENAMED = 0x00080000,

        PATH_ONLY_ADDED    = 0x00100000,
        PATH_ONLY_MODIFIED = 0x00200000,
        PATH_ONLY_DELETED  = 0x00400000,
        PATH_ONLY_RENAMED  = 0x00800000,

        IS_COPY_SOURCE     = 0x01000000,
        IS_COPY_TARGET     = 0x02000000,
        IS_FIRST           = 0x04000000,
        IS_LAST            = 0x08000000,

        IS_WORKINGCOPY     = 0x10000000,
        IS_MODIFIED_WC     = 0x20000000,

        // if set, the node should not be removed
        // (only valid in visible graph)

        MUST_BE_PRESERVED  = 0x80000000
    };

    enum 
    {
        IS_MASK            = 0x000000ff,
        IS_OPERATION_MASK  = 0x000000f0,
        COPIES_TO_MASK     = 0x0000ff00,
        ALL_COPIES_MASK    = 0x000f0000,
        PATH_ONLY_MASK     = 0x00f00000,
        SPECIAL_PROPS_MASK = 0xff000000,

        COPIES_TO_SHIFT    = 0x00000100,
        ALL_COPIES_SHIFT   = 0x00001000,
        PATH_ONLY_SHIFT    = 0x00010000,

        SUBTREE_DELETED    = IS_DELETED | ALL_COPIES_DELETED
    };

private:

    /// actually, this whole class is just a fancy DWORD

    DWORD flags;

public:

    /// empty construction

    CNodeClassification();
    CNodeClassification (DWORD flags);

    /// operations

    void Add (DWORD value);
    void Remove (DWORD value);

    /// specific data access

    bool Matches (DWORD required, DWORD forbidden) const;
    bool Is (DWORD value) const;
    bool IsAnyOf (DWORD value) const;

    DWORD GetFlags() const;
};


/// empty construction

inline CNodeClassification::CNodeClassification()
    : flags (0) 
{
}

inline CNodeClassification::CNodeClassification (DWORD flags)
    : flags (flags) 
{
}

/// operations

inline void CNodeClassification::Add (DWORD value)
{
    flags |= value;
}

inline void CNodeClassification::Remove (DWORD value)
{
    flags &= ~value;
}

/// specific data access

inline bool CNodeClassification::Matches (DWORD required, DWORD forbidden) const
{
    return (flags & (required | forbidden)) == required;
}

inline bool CNodeClassification::Is (DWORD value) const
{
    return Matches (value, 0);
}

inline bool CNodeClassification::IsAnyOf (DWORD value) const
{
    return (flags & value) != 0;
}

inline DWORD CNodeClassification::GetFlags() const
{
    return flags;
}
