// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

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
#include <afxwin.h>

class CControlsBridge {
public:
	static void AlignHorizontally(CWnd* parent, int labelId, int controlId);

private:
	CControlsBridge();
	~CControlsBridge();
};

inline void CControlsBridge::AlignHorizontally(CWnd* parent, int labelId, int controlId)
{
	CString labelText;
	parent->GetDlgItemText(labelId, labelText);

	CDC * pParentDC = parent->GetDC();
	CDC dc;
	dc.CreateCompatibleDC(pParentDC);
	parent->ReleaseDC(pParentDC);
	dc.SelectObject(parent->GetDlgItem(labelId)->GetFont());
	CSize textSize(dc.GetTextExtent(labelText));

	CRect labelRect;
	parent->GetDlgItem(labelId )->GetWindowRect(labelRect);
	parent->ScreenToClient(labelRect);
	labelRect.right = labelRect.left + textSize.cx + 2;
	parent->GetDlgItem(labelId)->MoveWindow(labelRect);

	CRect controlRect;
	parent->GetDlgItem(controlId)->GetWindowRect(controlRect);
	parent->ScreenToClient(controlRect);
	controlRect.left = labelRect.right;
	parent->GetDlgItem(controlId)->MoveWindow(controlRect);
}
