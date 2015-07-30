// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2015 - TortoiseSVN

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

/**
 * \ingroup TortoiseMerge
 * a custom MFC Ribbon status bar to avoid showing the customize context menu
 */

class CCustomMFCRibbonStatusBar : public CMFCRibbonStatusBar
{
public:
    CCustomMFCRibbonStatusBar() : CMFCRibbonStatusBar() {}

protected:
    virtual void OnPaneContextMenu(CWnd* /*pParentFrame*/, CPoint /*point*/)
    {
        return;
    }
};
