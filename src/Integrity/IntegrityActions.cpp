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
#include "IntegrityActions.h"
#include "IntegrityResponse.h"
#include "EventLog.h"
#include "CrashReport.h"
#include "DebugEventLog.h"

namespace IntegrityActions {
	void displayException(const IntegrityResponse& response);
	void logAnyExceptions(const IntegrityResponse& response);
	void executeUserCommand(const IntegritySession& session, const IntegrityCommand& command, std::function<void()>);

	void launchSandboxView(const IntegritySession& session, std::wstring path)
	{
		IntegrityCommand command(L"si", L"viewsandbox");
		command.addOption(L"cwd", path);
		command.addOption(L"g");

		executeUserCommand(session, command, nullptr);
	}

	void createSandbox(const IntegritySession& session, std::wstring path, std::function<void()> onDone)
	{
		IntegrityCommand command(L"si", L"createsandbox");
		command.addOption(L"g");
		command.addSelection(path);

		executeUserCommand(session, command, onDone);
	}

	const FileStatusFlags NO_STATUS = 0;

	// get status flags for a set of files...
	FileStatusFlags getFileStatus(const IntegritySession& session, const std::wstring& file) 
	{
		IntegrityCommand command(L"wf", L"fileinfo");
		command.addSelection(file);

		std::future<std::unique_ptr<IntegrityResponse>> responseFuture = session.executeAsync(command);

		std::future_status status = responseFuture.wait_for(std::chrono::seconds(10));
		if (status != std::future_status::ready) {
			EventLog::writeWarning(L"get status timeout for " + file);

			// need to call get so that the unique_ptr will exisit somewhere and go out of 
			// scope and delete the command runner / response
			std::async(std::launch::async, [&]{ 
					std::unique_ptr<IntegrityResponse> response = responseFuture.get(); 
				});

			return (FileStatusFlags)FileStatus::TimeoutError;
		}

		// TODO check future's exception... ?
		std::unique_ptr<IntegrityResponse> response = responseFuture.get();
		if (response == NULL) {
			EventLog::writeWarning(L"get status failed to return response");
			return NO_STATUS;
		}

		if (response->getException() != NULL) {
			logAnyExceptions(*response);
			return NO_STATUS;
		} 

		if (response->getExitCode() != 0) { 
			EventLog::writeWarning(L"get status has non zero exit code");
		}

		for (mksWorkItem item : *response) {
			int status = getIntegerFieldValue(item, L"status", NO_STATUS);

			EventLog::writeDebug(std::wstring(L"wf fileInfo ") + file + L" has status " +  std::to_wstring(status));
			return status;
		}
		return NO_STATUS;
	}

	std::vector<std::wstring> getControlledPaths(const IntegritySession& session) 
	{
		IntegrityCommand command(L"wf", L"folders");
		std::vector<std::wstring> rootPaths;

		std::unique_ptr<IntegrityResponse> response = session.execute(command);

		if (response->getException() != NULL) {
			logAnyExceptions(*response);
			return rootPaths;
		} 

		for (mksWorkItem item : *response) {
			rootPaths.push_back(getId(item));
		}
		
		return rootPaths;
	}

	void logAnyExceptions(const IntegrityResponse& response) {
		if (response.getException() != NULL) {
			std::wstring message = std::wstring(L"Error encountered running command '")
				+ response.getCommand().getApp() + L" " + response.getCommand().getName()
				+ L"': \n\terror id = "
				+ getId(response.getException()) + L"\n\t message = '"
				+ getExceptionMessage(response.getException()) + L"'";

			for (mksWorkItem item : response) {
				mksAPIException workItemException = mksWorkItemGetAPIException(item);

				if (workItemException != NULL) {
					message = std::wstring(L"\nError processing item '")
						+ getId(item) + L"' with type '" + getModelType(item)
						+ L"': \n\terror id = "
						+ getId(response.getException()) + L"\n\t message = '"
						+ getExceptionMessage(response.getException()) + L"'";
				}
			}

			EventLog::writeError(message);
		}
	}

	void displayException(const IntegrityCommand& command, std::wstring errorId, std::wstring msg) {
		std::wstring message = std::wstring(L"Error encountered running command '")  
			+ command.getApp() + L" " + command.getName()
			+ L"': \n\terror id = " + errorId + L"\n\t message = '" + msg + L"'";

		MessageBoxW(NULL, message.c_str(), L"Error", MB_OK | MB_ICONEXCLAMATION);
	}

	void executeUserCommand(const IntegritySession& session, const IntegrityCommand& command, std::function<void()> onDone) {
		std::async(std::launch::async, [&](IntegrityCommand command, std::function<void()> onDone){
			try {
				std::unique_ptr<IntegrityResponse> response = session.execute(command);

				logAnyExceptions(*response);

				if (onDone != nullptr) {
					onDone();
				}
			}
			catch (std::exception)
			{
			}
		}, command, onDone);
	}

	void displayException(const IntegrityResponse& response) {
		mksAPIException exception = response.getException();
		if (exception != NULL) {
			for (mksWorkItem item : response) {
				mksAPIException workItemException = mksWorkItemGetAPIException(item);

				if (workItemException != NULL) {
					displayException(response.getCommand(), getId(workItemException),
						getExceptionMessage(workItemException));
					return;
				}
			}
			
			displayException(response.getCommand(), getId(exception),
				getExceptionMessage(exception));
		}
	}
}