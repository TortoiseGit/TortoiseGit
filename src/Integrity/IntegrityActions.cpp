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
#include "EventLog.h"

#define SIZE 256

namespace IntegrityActions {
	void displayException(const IntegrityResponse& response);
	std::wstring getExceptionId(mksAPIException exception);
	std::wstring getExceptionMessage(mksAPIException exception);
	void logException(mksAPIException exception);
	int getIntegerFieldValue(mksField field);
	int getIntegerFieldValue(mksWorkItem item, const std::wstring&  fieldName, int defaultValue);
	void executeUserCommand(const IntegritySession& session, const IntegrityCommand& command);

	void launchSandboxView(const IntegritySession& session, std::wstring path)
	{
		IntegrityCommand command(L"si", L"viewsandbox");
		command.addOption(L"cwd", path);
		command.addOption(L"g");

		executeUserCommand(session, command);
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
			logException(response->getException());
			return NO_STATUS;
		} 

		if (response->getExitCode() != 0) { 
			EventLog::writeWarning(L"get status has non zero exit code");
		}

		for (mksWorkItem item : *response) {
			int status = getIntegerFieldValue(item, L"status", NO_STATUS);

			// TODO exception field?

			EventLog::writeDebug(std::wstring(L"wf fileInfo ") + file + L" has status " +  std::to_wstring(status));
			return status;
		}
		return NO_STATUS;
	}

	bool getBooleanFieldValue(mksField field) 
	{
		unsigned short boolValue = 0;
		mksFieldGetBooleanValue(field, &boolValue);

		if (boolValue) {
			return true;
		} else {
			return false;
		}
	}

	int getIntegerFieldValue(mksField field) 
	{
		int value = 0;
		mksFieldGetIntegerValue(field, &value);
		return value;
	}

	int getIntegerFieldValue(mksWorkItem item, const std::wstring& fieldName, int defaultValue) 
	{
		mksField field = mksWorkItemGetField(item, (wchar_t*)fieldName.c_str());

		if (field != NULL) {
			return getIntegerFieldValue(field);
		} else {
			return defaultValue;
		}
	}

	std::vector<std::wstring> getControlledPaths(const IntegritySession& session) 
	{
		IntegrityCommand command(L"wf", L"folders");
		std::vector<std::wstring> rootPaths;

		std::unique_ptr<IntegrityResponse> response = session.execute(command);

		if (response->getException() != NULL) {
			logException(response->getException());
			return rootPaths;
		} 

		for (mksWorkItem item : *response) {
			wchar_t path[1024];

			mksWorkItemGetId(item, path, 1024);

			// TODO exception field?

			rootPaths.push_back(path);
		}
		
		return rootPaths;
	}

	void logException(mksAPIException exception) {
		std::wstring message = std::wstring(L"Error encountered running command 'wf getstatus': \n\terror id = ")
			+ getExceptionId(exception) + L"\n\t message = '" 
			+ getExceptionMessage(exception) + L"'";

		EventLog::writeError(message);
	}

	void displayException(const IntegrityCommand& command, std::wstring errorId, std::wstring msg) {
		std::wstring message = std::wstring(L"Error encountered running command '")  
			+ command.getApp() + L" " + command.getName()
			+ L"': \n\terror id = " + errorId + L"\n\t message = '" + msg + L"'";

		MessageBoxW(NULL, message.c_str(), L"Error", MB_OK | MB_ICONEXCLAMATION);
	}

	void executeUserCommand(const IntegritySession& session, const IntegrityCommand& command) {		
		std::async(std::launch::async, [&](IntegrityCommand command){ 
				std::unique_ptr<IntegrityResponse> response = session.execute(command);

				displayException(*response);
			}, command);
	}

	void displayException(const IntegrityResponse& response) {
		mksAPIException exception = response.getException();
		if (exception != NULL) {
			for (mksWorkItem item : response) {
				mksAPIException workItemException = mksWorkItemGetAPIException(item);

				if (workItemException != NULL) {
					displayException(response.getCommand(), getExceptionId(workItemException), 
						getExceptionMessage(workItemException));
					return;
				}
			}
			
			displayException(response.getCommand(), getExceptionId(exception), 
				getExceptionMessage(exception));
		}
	}

	std::wstring getExceptionMessage(mksAPIException exception) { 
		wchar_t buffer[1024];
		mksAPIExceptionGetMessage(exception, buffer, 1024);
		return std::wstring(buffer);
	}

	std::wstring getExceptionId(mksAPIException exception) {
		wchar_t buffer[1024];
		mksAPIExceptionGetId(exception, buffer, 1024);
		return std::wstring(buffer);
	}
}