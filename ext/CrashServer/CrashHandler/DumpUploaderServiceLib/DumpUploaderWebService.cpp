// Copyright 2012 Idol Software, Inc.
//
// This file is part of CrashHandler library.
//
// CrashHandler library is free software: you can redistribute it and/or modify
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
#include "generated\UploaderSoap.nsmap"
#include "gsoapWinInet.h"
#include <sstream>
#include <atlbase.h>
#include <atlstr.h>

DumpUploaderWebService::DumpUploaderWebService(int responseTimeoutSec)
    : UploaderSoapProxy(SOAP_ENC_MTOM|SOAP_IO_KEEPALIVE|SOAP_C_UTFSTRING/*SOAP_C_MBSTRING*/)
{
    m_serviceUrl = "https://www.crash-server.com";

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

    m_serviceUrl += "/DumpUploader.asmx";

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
