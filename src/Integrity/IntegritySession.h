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
	std::future<std::unique_ptr<IntegrityResponse>> executeAsync(const IntegrityCommand& command) const;

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