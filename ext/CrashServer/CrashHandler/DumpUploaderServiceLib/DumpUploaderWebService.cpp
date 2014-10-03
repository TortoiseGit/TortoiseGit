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

#include "DumpUploaderWebService.h"
#include "generated/CustomBinding_DumpUploadService3_Uploader.nsmap"
#include "gsoapWinInet.h"
#include <sstream>
#include <atlbase.h>
#include <atlstr.h>

DumpUploaderWebService::DumpUploaderWebService(int responseTimeoutSec)
    : Base(SOAP_ENC_MTOM|SOAP_IO_KEEPALIVE|SOAP_C_UTFSTRING/*SOAP_C_MBSTRING*/)
    , m_testMode(false)
    , m_serviceUrl("https://www.drdump.com")
{
    CRegKey reg;
    if (ERROR_SUCCESS == reg.Open(HKEY_LOCAL_MACHINE, _T("Software\\Idol Software\\DumpUploader"), KEY_READ))
    {
        CString str;
        ULONG size = 1000;
        if (ERROR_SUCCESS == reg.QueryStringValue(_T("ServiceURL"), str.GetBuffer(size), &size))
        {
            str.ReleaseBuffer(size-1);
            m_serviceUrl = str;
        }

        DWORD cfgResponseTimeoutSec = 0;
        if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("ResponseTimeoutSec"), cfgResponseTimeoutSec) && cfgResponseTimeoutSec != 0)
            responseTimeoutSec = static_cast<int>(cfgResponseTimeoutSec);

        DWORD testMode = 0;
        if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("TestMode"), testMode))
            m_testMode = testMode != 0;

#ifdef _DEBUG // soap_set_*_logfile defined only in DEBUG build of GSOAP
        DWORD traceEnable = FALSE;
        if (ERROR_SUCCESS == reg.QueryDWORDValue(_T("TraceEnable"), traceEnable) && traceEnable != FALSE)
        {
            size = 1000;
            if (ERROR_SUCCESS == reg.QueryStringValue(_T("TraceFolder"), str.GetBuffer(size), &size))
            {
                str.ReleaseBuffer(size-1);
                soap_set_test_logfile(this, str + _T("\\_gsoap_test.log"));
                soap_set_sent_logfile(this, str + _T("\\_gsoap_sent.log"));
                soap_set_recv_logfile(this, str + _T("\\_gsoap_recv.log"));
            }
        }
#endif
    }

    m_serviceUrl += "/Service/DumpUploader3.svc";

    soap_endpoint = m_serviceUrl.c_str();
    recv_timeout = responseTimeoutSec;
    soap_register_plugin(this, wininet_plugin);
}

std::wstring DumpUploaderWebService::GetErrorText()
{
    std::ostringstream o;
    soap_stream_fault(o);
    std::vector<wchar_t> buf(o.str().size() + 1);
    buf.resize(MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) o.str().c_str(), (int)o.str().size() + 1, &buf.front(), (int)buf.size()));

    return std::wstring(buf.begin(), buf.end());
}

void DumpUploaderWebService::SetProgressCallback(pfnProgressCallback progressCallback, LPVOID context)
{
    wininet_set_progress_callback(this, progressCallback, context);
}

int DumpUploaderWebService::Hello(_ns1__Hello *ns1__Hello, _ns1__HelloResponse *ns1__HelloResponse)
{
    if (!m_testMode)
        return Base::Hello(ns1__Hello, ns1__HelloResponse);

    return Base::Hello(ns1__Hello, ns1__HelloResponse);
}

int DumpUploaderWebService::UploadMiniDump(_ns1__UploadMiniDump *ns1__UploadMiniDump, _ns1__UploadMiniDumpResponse *ns1__UploadMiniDumpResponse)
{
    return Base::UploadMiniDump(ns1__UploadMiniDump, ns1__UploadMiniDumpResponse);
}

int DumpUploaderWebService::UploadAdditionalInfo(_ns1__UploadAdditionalInfo *ns1__UploadAdditionalInfo, _ns1__UploadAdditionalInfoResponse *ns1__UploadAdditionalInfoResponse)
{
    return Base::UploadAdditionalInfo(ns1__UploadAdditionalInfo, ns1__UploadAdditionalInfoResponse);
}

int DumpUploaderWebService::UploadSymbol(_ns1__UploadSymbol *ns1__UploadSymbol, _ns1__UploadSymbolResponse *ns1__UploadSymbolResponse)
{
    return Base::UploadSymbol(ns1__UploadSymbol, ns1__UploadSymbolResponse);
}