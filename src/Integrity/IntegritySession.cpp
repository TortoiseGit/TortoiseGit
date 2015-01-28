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

#include "stdafx.h"
#include "IntegritySession.h"

IntegritySession::IntegritySession()
{
	ip = NULL;
	session = NULL;

	mksAPIInitialize(NULL); // TODO allow API loging
	//mksLogConfigure(MKS_LOG_ERROR, MKS_LOG_MEDIUM);

	mksCreateLocalIntegrationPoint(&ip, TRUE);
	if (ip != NULL) {
		mksGetCommonSession(&session, ip);
	}
}

IntegritySession::~IntegritySession()
{
	if (ip != NULL) {
		mksReleaseIntegrationPoint(ip);
		mksAPITerminate();

		ip = NULL;
		session = NULL;
	}
}

std::wstring getOptionString(IntegrityCommand::Option& option) {
	std::wstring optionString = L"";
	if (option.name.size() == 1) {
		optionString = L"-" + option.name;
		if (option.value.size() > 0) {
			optionString += L" " + option.value;
		}
	} else {
		optionString = L"--" + option.name;
		if (option.value.size() > 0) {
			optionString += L"=" + option.value;
		}
	}
	return optionString;
}

class NativeIntegrityCommand {
public:
	NativeIntegrityCommand(const IntegrityCommand& command) {
		nativeCommand = mksCreateCommand();
		if (nativeCommand != NULL) {
			nativeCommand->appName = command.getApp();
			nativeCommand->cmdName = command.getName();

			for (IntegrityCommand::Option option : command.options) {
				if (option.value.size() > 0) {
					mksOptionListAdd(nativeCommand->optionList, option.name.c_str(), option.value.c_str());
				} else {
					mksOptionListAdd(nativeCommand->optionList, option.name.c_str(), NULL);
				}
			}

			for (std::wstring selectionItem : command.selection) {
				mksSelectionListAdd(nativeCommand->selectionList, selectionItem.c_str());
			}
		}
	};
	~NativeIntegrityCommand() {
		if (nativeCommand != NULL) {
			mksReleaseCommand(nativeCommand);
		}
	};

	mksCommand nativeCommand;
};

void IntegrityCommand::addSelection(const IntegrityCommand& command)
{
	addSelection(command.getApp());
	addSelection(command.getName());

	for (IntegrityCommand::Option option : command.options) {
		addSelection(getOptionString(option));
	}

	// explicitly mark end of options and start of selection 
	addSelection(L"--");

	for (std::wstring selectionItem : command.selection) {
		addSelection(selectionItem);
	}
}

std::unique_ptr<IntegrityResponse> IntegritySession::execute(const IntegrityCommand& command) const
{
	if (session != NULL) {
		mksCmdRunner commandRunner;
		NativeIntegrityCommand nativeIntegrityCommand(command);

		mksCreateCmdRunner(&commandRunner, session);

		if (commandRunner != NULL) {
			mksResponse response = mksCmdRunnerExecCmd(commandRunner, nativeIntegrityCommand.nativeCommand, NO_INTERIM);

			return std::unique_ptr<IntegrityResponse>(new IntegrityResponse(commandRunner, response, command));
		}
	}
	return NULL;
};

std::shared_future<std::unique_ptr<IntegrityResponse>> IntegritySession::executeAsync(const IntegrityCommand& command) const {
	return std::async(std::launch::async, [&]() { return this->execute(command); } );
}
