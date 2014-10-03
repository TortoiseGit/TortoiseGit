// Copyright 2014 Idol Software, Inc.
//
// This file is part of Doctor Dump SDK.
//
// Doctor Dump SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "generated/soapCustomBinding_DumpUploadService3_UploaderProxy.h"

class DumpUploaderWebService : public CustomBinding_DumpUploadService3_UploaderProxy
{
    typedef CustomBinding_DumpUploadService3_UploaderProxy Base;
    std::string m_serviceUrl;
    bool m_testMode;
public:
    DumpUploaderWebService(int responseTimeoutSec = 0);
    std::wstring GetErrorText();

    typedef void (*pfnProgressCallback)(BOOL send, SIZE_T bytesCount, LPVOID context);
    void SetProgressCallback(pfnProgressCallback progressCallback, LPVOID context);

    int Hello(_ns1__Hello *ns1__Hello, _ns1__HelloResponse *ns1__HelloResponse) override;
    int UploadMiniDump(_ns1__UploadMiniDump *ns1__UploadMiniDump, _ns1__UploadMiniDumpResponse *ns1__UploadMiniDumpResponse) override;
    int UploadAdditionalInfo(_ns1__UploadAdditionalInfo *ns1__UploadAdditionalInfo, _ns1__UploadAdditionalInfoResponse *ns1__UploadAdditionalInfoResponse) override;
    int UploadSymbol(_ns1__UploadSymbol *ns1__UploadSymbol, _ns1__UploadSymbolResponse *ns1__UploadSymbolResponse) override;
};
