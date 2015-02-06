// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI

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

#include "FileStatus.h"
#include "IntegritySession.h"

// methods to execute integrity commands

namespace IntegrityActions {
	// get status flags for a set of files...
	FileStatusFlags getFileStatus(const IntegritySession& session, const std::wstring& files);

	std::vector<std::wstring> getControlledPaths(const IntegritySession& session);

	void launchSandboxView(const IntegritySession& session, std::wstring path);
	void launchMemberHistoryView(const IntegritySession& session, std::wstring path);
	void launchLocalChangesDiffView(const IntegritySession& session, std::wstring path);
	void launchIncomingChangesDiffView(const IntegritySession& session, std::wstring path);
	void launchAnnotatedRevisionView(const IntegritySession& session, std::wstring path);
	void launchSubmitChangesView(const IntegritySession& session, std::wstring path);
	void launchChangePackageView(const IntegritySession& session);
	void launchMyChangePackageReviewsView(const IntegritySession& session);
	void launchPreferencesView(const IntegritySession& session);
	void launchIntegrityGUI(const IntegritySession& session);

	void lockFile(const IntegritySession& session, std::wstring path);
	void unlockFile(const IntegritySession& session, std::wstring path);
	void addFile(const IntegritySession& session, std::wstring path);
	void dropPath(const IntegritySession& session, std::wstring path);
	void moveFile(const IntegritySession& session, std::wstring path);
	void renameFile(const IntegritySession& session, std::wstring path);
	void revertFile(const IntegritySession& session, std::wstring path);

	void resynchronize(const IntegritySession& session, std::wstring path);
	void resynchronizeByChangePackage(const IntegritySession& session, std::wstring path);

	void createSandbox(const IntegritySession& session, std::wstring path, std::function<void()> onDone);
	void dropSandbox(const IntegritySession& session, std::wstring path);
	void retargetSandbox(const IntegritySession& session, std::wstring path);

}