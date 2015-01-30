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

#include <string>
#include <vector>
#include <set>
#include <list>
#include <memory>
#include <future>

extern "C" {
#include <mksapi.h>
}

#ifdef _WIN64
#pragma comment(lib, "..\\..\\ext\\Integrity\\lib\\mksapi64.lib")
#else
#pragma comment(lib, "..\\..\\ext\\Integrity\\lib\\mksapi.lib")
#endif

class IntegrityCommand;
class IntegrityResponse;

class IntegritySession {
public:
	IntegritySession();
	IntegritySession(std::string hostname, int port);
	~IntegritySession();

	std::unique_ptr<IntegrityResponse> execute(const IntegrityCommand& command) const;
	std::shared_future<std::unique_ptr<IntegrityResponse>> executeAsync(const IntegrityCommand& command) const;

private:
	mksIntegrationPoint ip;
	mksSession session;

	void initAPI();
};

class IntegrityCommand {
public:
	IntegrityCommand(wchar_t* app, wchar_t* name) {
		this->app = app;
		this->name = name;
	};

	void addOption(std::wstring name, std::wstring value) {
		options.push_back(Option(name, value));
	}

	void addOption(std::wstring name) {
		options.push_back(Option(name));
	}

	void addSelection(std::wstring selectionItem) {
		selection.push_back(selectionItem);
	}
	// for commands that wrap other commands
	void addSelection(const IntegrityCommand& selectionItem);

	wchar_t* getApp() const {
		return app;
	}

	wchar_t* getName() const {
		return name;
	}

	class Option {
	public:
		const std::wstring name, value;

		Option(std::wstring name, std::wstring value) : name(name), value(value){}
		Option(std::wstring name) : name(name) {}
	};

	std::vector<std::wstring> selection;
	std::vector<Option> options;

private:
	wchar_t *name, *app;
};

class IntegrityResponse {
public:
	IntegrityResponse(mksCmdRunner commandRunner, mksResponse response, const IntegrityCommand& command) 
		: command(command)
	{
		this->commandRunner = commandRunner;
		this->response = response;
	}

	~IntegrityResponse() 
	{
		// releaseing the command runner also releases the response
		if (commandRunner != NULL) {
			mksReleaseCmdRunner(commandRunner);
			commandRunner = NULL;
			response = NULL;
		}
	}

	mksAPIException getException() const {
		if (response != NULL) {
			return mksResponseGetAPIException(response);
		} else {
			return NULL;
		}
	}

	int getExitCode() {
		int exitCode = -1;
		if (response != NULL) {
			mksResponseGetExitCode(response, &exitCode);
		}
		return exitCode;
	}

	mksResponse getResponse() const {
		return response;
	}

	const IntegrityCommand& getCommand() const {
		return command;
	}

private:
	mksCmdRunner commandRunner;
	mksResponse response;
	const IntegrityCommand& command;
};

class WorkItemIterator {
public:
	WorkItemIterator(mksResponse response, mksWorkItem item)  : item(item), response(response){
	}

	WorkItemIterator& operator++() {
		item = mksResponseGetNextWorkItem(response);
		return *this;
	}

	bool operator!=(const WorkItemIterator& other) const {
		return item != other.item;
	}

	mksWorkItem operator*() const {
		return item;
	}

private:
	mksWorkItem item;
	mksResponse response;
};

inline WorkItemIterator begin(mksResponse response) {
	return WorkItemIterator(response, mksResponseGetFirstWorkItem(response));
}

inline WorkItemIterator end(mksResponse response) {
	return WorkItemIterator(response, NULL);
}

inline WorkItemIterator begin(const IntegrityResponse& response) {
	return begin(response.getResponse());
}

inline WorkItemIterator end(const IntegrityResponse& response) {
	return end(response.getResponse());
}

class WorkItemFieldIterator {
public:
	WorkItemFieldIterator(mksWorkItem item, mksField field) : field(field), item(item){
	}

	WorkItemFieldIterator& operator++() {
		field = mksWorkItemGetNextField(item);
		return *this;
	}

	bool operator!=(const WorkItemFieldIterator& other) const {
		return field != other.field;
	}

	mksField operator*() const {
		return field;
	}

private:
	mksWorkItem item;
	mksField field;
};

inline WorkItemFieldIterator begin(mksWorkItem item) {
	return WorkItemFieldIterator(item, mksWorkItemGetFirstField(item));
}

inline WorkItemFieldIterator end(mksWorkItem item) {
	return WorkItemFieldIterator(item, NULL);
}
