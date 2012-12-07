// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009-2011 - TortoiseSVN

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
#include "RevisionGraphCommand.h"

#include "RevisionGraph\RevisionGraphDlg.h"

bool RevisionGraphCommand::Execute()
{
    CString val = parser.GetVal(_T("pegrev"));
    GitRev pegrev;// = val.IsEmpty() ? GitRev() : GitRev(val);

   // std::unique_ptr<CRevisionGraphDlg> dlg(new CRevisionGraphDlg());
	CRevisionGraphDlg dlg;
	dlg.SetPath(cmdLinePath.GetWinPathString());
//    dlg.SetPegRevision(pegrev);
    if (parser.HasVal(L"output"))
    {
        dlg.SetOutputFile(parser.GetVal(L"output"));
//        if (parser.HasVal(L"options"))
//            dlg.SetOptions(parser.GetLongVal(L"options"));
        dlg.StartHidden();
    }
    dlg.DoModal();

    return true;
}