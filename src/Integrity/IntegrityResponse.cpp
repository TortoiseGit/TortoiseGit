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
#include "IntegrityResponse.h"

#define DEFAULT_BUFFER_SIZE 1024

std::wstring getExceptionMessage(mksAPIException exception) {
	wchar_t buffer[DEFAULT_BUFFER_SIZE];
	mksAPIExceptionGetMessage(exception, buffer, DEFAULT_BUFFER_SIZE);
	return std::wstring(buffer);
}

std::wstring getId(mksAPIException exception) {
	wchar_t buffer[DEFAULT_BUFFER_SIZE];
	mksAPIExceptionGetId(exception, buffer, DEFAULT_BUFFER_SIZE);
	return std::wstring(buffer);
}

std::wstring getId(mksWorkItem item) {
	wchar_t buffer[DEFAULT_BUFFER_SIZE];
	mksWorkItemGetId(item, buffer, DEFAULT_BUFFER_SIZE);
	return std::wstring(buffer);
}

std::wstring getModelType(mksWorkItem item) {
	wchar_t buffer[DEFAULT_BUFFER_SIZE];
	mksWorkItemGetModelType(item, buffer, DEFAULT_BUFFER_SIZE);
	return std::wstring(buffer);
}


bool getBooleanFieldValue(mksField field)
{
	unsigned short boolValue = 0;
	mksFieldGetBooleanValue(field, &boolValue);

	if (boolValue) {
		return true;
	}
	else {
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
	}
	else {
		return defaultValue;
	}
}