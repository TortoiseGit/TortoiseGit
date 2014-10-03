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

#include "stdafx.h"
#include "DoctorDump.h"
#include "..\..\CommonLibs\Log\log.h"
#include "..\..\CommonLibs\Zlib\ZipUnzip.h"

using namespace std;
using namespace doctor_dump;

class ProgressNotifier
{
public:
    ProgressNotifier(DumpUploaderWebService& webService, size_t total, IUploadProgress& uploadProgress)
        : m_webService(webService), m_sent(0), m_total(total), m_uploadProgress(uploadProgress)
    {
        m_webService.SetProgressCallback(ProgressCallback, this);
    }

    ~ProgressNotifier()
    {
        m_webService.SetProgressCallback(nullptr, nullptr);
    }

private:
    size_t m_sent;
    size_t m_total;
    IUploadProgress& m_uploadProgress;
    DumpUploaderWebService& m_webService;

    static void ProgressCallback(BOOL send, SIZE_T bytesCount, LPVOID context)
    {
        static_cast<ProgressNotifier*>(context)->ProgressCallback(send, bytesCount);
    }

    void ProgressCallback(BOOL send, SIZE_T bytesCount)
    {
        if (!send)
            return;
        m_sent += bytesCount;
        if (m_sent > m_total)
            m_sent = m_total;

        m_uploadProgress.OnProgress(m_total, m_sent);
    }
};

std::vector<unsigned char> DumpUploaderWebServiceEx::ReadFile(const std::wstring& path)
{
    std::vector<unsigned char> data;

    CAtlFile file;
    for (int i = 0; ; ++i)
    {
        file.Attach(CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
        if (file != INVALID_HANDLE_VALUE)
            break;

        DWORD err = GetLastError();
        // File indexing services like to open just closed files (our doctor_dump_mini.zip file), so lets make few tries.
        if (err == ERROR_SHARING_VIOLATION && i < 10)
        {
            Sleep(1000);
            continue;
        }
        CStringA text;
        text.Format("failed (err %d) to open file %ls", err, path.c_str());
        throw std::runtime_error(text);
    }


    ULONGLONG fileSize;
    if (FAILED(file.GetSize(fileSize)))
        throw std::runtime_error(std::string("failed to get file size ") + (const char*)CW2A(path.c_str()));

    if (fileSize != 0)
    {
        data.resize((size_t)fileSize);
        if (FAILED(file.Read(&data[0], static_cast<DWORD>(fileSize))))
            throw std::runtime_error(std::string("failed to read file ") + (const char*)CW2A(path.c_str()));
    }

    return data;
}

std::unique_ptr<Response> Response::CreateResponse(const ns1__Response& response, Log& log)
{
    std::unique_ptr<Response> result;
    if (SOAP_TYPE_ns1__HaveSolutionResponse == response.soap_type())
    {
        log.Debug(_T("\thas solution"));
        const ns1__HaveSolutionResponse& resp = static_cast<const ns1__HaveSolutionResponse&>(response);
        std::unique_ptr<HaveSolutionResponse> res(new HaveSolutionResponse);
        res->askConfirmation = resp.askConfirmation;
        res->type = resp.type;
        log.Debug(_T("\t\ttype\t%d"), res->type);
        if (resp.url)
        {
            res->url = resp.url->c_str();
            log.Debug(_T("\t\tURL\t%ls"), res->url.c_str());
        }
        if (resp.exe)
        {
            res->exe.assign(resp.exe->__ptr, resp.exe->__ptr + resp.exe->__size);
            log.Debug(_T("\t\thas EXE, size\t%d"), res->exe.size());
        }
        result = std::move(res);
    }
    else if (SOAP_TYPE_ns1__NeedSymbolsThenMiniDumpResponse == response.soap_type() || SOAP_TYPE_ns1__NeedMiniDumpResponse == response.soap_type())
    {
        result.reset(new NeedMiniDumpResponse);
    }
    else if (SOAP_TYPE_ns1__NeedFullDumpResponse == response.soap_type())
    {
        const ns1__NeedFullDumpResponse& resp = static_cast<const ns1__NeedFullDumpResponse&>(response);
        std::unique_ptr<NeedFullDumpResponse> res(new NeedFullDumpResponse);

        res->restrictedDumpType = resp.restrictedDumpType;
        res->attachUserInfo = resp.attachUserInfo;

        result = std::move(res);
    }
    else if (SOAP_TYPE_ns1__NeedMoreInfoResponse == response.soap_type())
    {
        const ns1__NeedMoreInfoResponse& resp = static_cast<const ns1__NeedMoreInfoResponse&>(response);
        std::unique_ptr<NeedMoreInfoResponse> res(new NeedMoreInfoResponse);
        if (resp.infoModule && resp.infoModule->__size > 0)
            res->infoModule.assign(resp.infoModule->__ptr, resp.infoModule->__ptr + resp.infoModule->__size);
        else
            res->infoModule.clear();
        if (resp.infoModuleCfg)
            res->infoModuleCfg = resp.infoModuleCfg->c_str();
        else
            res->infoModuleCfg = L"";
        result = std::move(res);
    }
    else if (SOAP_TYPE_ns1__StopResponse == response.soap_type())
    {
        result.reset(new StopResponse);
    }
    else if (SOAP_TYPE_ns1__ErrorResponse == response.soap_type())
    {
        const ns1__ErrorResponse& resp = static_cast<const ns1__ErrorResponse&>(response);
        std::unique_ptr<ErrorResponse> res(new ErrorResponse);
        res->error = *resp.error;
        log.Debug(_T("\terror\t\"%ls\""), res->error.c_str());
        result = std::move(res);
    }
    else
    {
        throw std::runtime_error("unknown response type");
    }

    if (response.clientID)
    {
        result->clientID = *response.clientID;
        log.Debug(_T("\tCliendID = \"%ls\""), result->clientID.c_str());
    }
    result->problemID = response.problemID ? *response.problemID : 0;
    result->dumpGroupID = response.dumpGroupID ? *response.dumpGroupID : 0;
    result->dumpID = response.dumpID ? *response.dumpID : 0;
    log.Debug(_T("\tproblemID = %d, dumpGroupID = %d, dumpID = %d"), result->problemID, result->dumpGroupID, result->dumpID);
    if (response.urlToProblem)
    {
        result->urlToProblem = *response.urlToProblem;
        log.Debug(_T("\tURL to problem = \"%ls\""), result->urlToProblem.c_str());
    }
    else
    {
        result->urlToProblem = L"";
    }
    if (response.context && response.context->__size > 0)
        result->context.assign(response.context->__ptr, response.context->__ptr + response.context->__size);
    else
        result->context.clear();

    return result;
}

_xop__Include* DumpUploaderWebServiceEx::CreateXopInclude(DeferredDelete& context, const std::vector<BYTE>& data)
{
    SOAP_SHARED_STRUCT(_xop__Include, ptr);
    context.emplace_back(ptr);

    _xop__Include* soapXop = ptr->get();
    if (!data.empty())
    {
        soapXop->__ptr = const_cast<BYTE*>(data.data());
        soapXop->__size = static_cast<int>(data.size());
    }
    else
    {
        soapXop->__ptr = NULL;
        soapXop->__size = 0;
    }
    soapXop->id = NULL;
    soapXop->options = NULL;
    soapXop->type = "binary";

    return soapXop;
}

_xop__Include* DumpUploaderWebServiceEx::CreateXopIncludeFromFile(DeferredDelete& context, const std::wstring& path)
{
    std::shared_ptr<std::vector<unsigned char>> data(std::make_shared<std::vector<unsigned char>>(ReadFile(path)));

    _xop__Include* soapXop = CreateXopInclude(context, *data.get());
    ((soap_helpers::SoapStuct<_xop__Include>*) context.back().get())->m_data = data;

    return soapXop;
}

DumpUploaderWebServiceEx::DumpUploaderWebServiceEx(Log& log_)
    : m_webService(5*60 /* Parsing a dump may require a lot of time (for example if symbols are uploaded from microsoft) */)
    , m_log(log_)
{
}

ns1__ClientLib* DumpUploaderWebServiceEx::CreateClientLib(DeferredDelete& context)
{
    SOAP_SHARED_STRUCT(ns1__ClientLib, ptr);
    context.emplace_back(ptr);

    ns1__ClientLib* soapClient = ptr->get();
    soapClient->type = ns4__ClientLib_ClientType__CrashHandler_1_0;
    soapClient->v1 = 0;
    soapClient->v2 = 0;
    soapClient->v3 = 0;
    soapClient->v4 = 0;
    soapClient->arch =
#ifdef _M_AMD64
        ns4__ClientLib_Architecture__x64
#else
        ns4__ClientLib_Architecture__x86
#endif
        ;

    TCHAR path[MAX_PATH];
    if (!GetModuleFileName(NULL, path, _countof(path)))
        return soapClient;

    std::vector<BYTE> verInfo(GetFileVersionInfoSize(path, NULL));
    VS_FIXEDFILEINFO* lpFileinfo = NULL;
    UINT uLen;
    if (verInfo.empty()
        || !GetFileVersionInfo(path, 0, static_cast<DWORD>(verInfo.size()), &verInfo[0])
        || !VerQueryValue(&verInfo[0], _T("\\"), (LPVOID*)&lpFileinfo, &uLen))
        return soapClient;
    soapClient->v1 = HIWORD(lpFileinfo->dwProductVersionMS);
    soapClient->v2 = LOWORD(lpFileinfo->dwProductVersionMS);
    soapClient->v3 = HIWORD(lpFileinfo->dwProductVersionLS);
    soapClient->v4 = LOWORD(lpFileinfo->dwProductVersionLS);
    m_log.Info(_T("Client lib %d.%d.%d.%d"), soapClient->v1, soapClient->v2, soapClient->v3, soapClient->v4);

    return soapClient;
}

ns1__Application* DumpUploaderWebServiceEx::CreateApp(DeferredDelete& context, const Application& app)
{
    SOAP_SHARED_STRUCT(ns1__Application, ptr);
    context.emplace_back(ptr);

    std::shared_ptr<std::wstring> pMainModule(std::make_shared<std::wstring>(app.processName));
    ptr->m_data = pMainModule;

    ns1__Application* soapApp = ptr->get();
    soapApp->applicationGUID = app.applicationGUID;
    soapApp->v1 = app.v[0];
    soapApp->v2 = app.v[1];
    soapApp->v3 = app.v[2];
    soapApp->v4 = app.v[3];
    soapApp->hotfix = app.hotfix;
    soapApp->mainModule = pMainModule.get();
    m_log.Info(_T("App %d.%d.%d.%d %ls"), soapApp->v1, soapApp->v2, soapApp->v3, soapApp->v4, soapApp->applicationGUID.c_str());
    return soapApp;
}

ns1__AppAdditionalInfo* DumpUploaderWebServiceEx::CreateAppAddInfo(DeferredDelete& context, std::wstring& appName, std::wstring& companyName)
{
    SOAP_SHARED_STRUCT(ns1__AppAdditionalInfo, ptr);
    context.emplace_back(ptr);

    ns1__AppAdditionalInfo* soapAppAddInfo = ptr->get();
    soapAppAddInfo->appName = &appName;
    soapAppAddInfo->companyName = &companyName;
    return soapAppAddInfo;
}

ns1__DumpAdditionalInfo* DumpUploaderWebServiceEx::CreateDumpAddInfo(DeferredDelete& context, const DumpAdditionalInfo &dumpAddInfo)
{
    SOAP_SHARED_STRUCT(ns1__DumpAdditionalInfo, ptr);
    context.emplace_back(ptr);

    ns1__DumpAdditionalInfo* soapAddInfo = ptr->get();
    soapAddInfo->crashDate = dumpAddInfo.crashDate;
    soapAddInfo->PCID = dumpAddInfo.PCID;
    soapAddInfo->submitterID = dumpAddInfo.submitterID;
    soapAddInfo->group = const_cast<std::wstring*>(&dumpAddInfo.group);
    soapAddInfo->description = const_cast<std::wstring*>(&dumpAddInfo.description);
    return soapAddInfo;
}

void DumpUploaderWebServiceEx::ThrowOnSoapFail(int res)
{
    if (res != SOAP_OK)
        throw SoapException(m_webService.error, (const char*)CW2A(m_webService.GetErrorText().c_str()));
}

std::unique_ptr<Response> DumpUploaderWebServiceEx::Hello(
    const Application& app,
    std::wstring appName,
    std::wstring companyName,
    const DumpAdditionalInfo& addInfo)
{
    m_log.Info(_T("Sending Hello..."));

    DeferredDelete deleteList;

    SOAP_STRUCT(_ns1__Hello, request);
    request->clientLib = CreateClientLib(deleteList);
    request->app = CreateApp(deleteList, app);
    request->appAddInfo = CreateAppAddInfo(deleteList, appName, companyName);
    request->addInfo = CreateDumpAddInfo(deleteList, addInfo);

    SOAP_STRUCT(_ns1__HelloResponse, response);
    ThrowOnSoapFail(m_webService.Hello(request, response));

    return Response::CreateResponse(*response->HelloResult, m_log);
}

std::unique_ptr<Response> DumpUploaderWebServiceEx::UploadMiniDump(const std::vector<BYTE>& context, const Application& app, const DumpAdditionalInfo& addInfo, const std::wstring& dumpFile)
{
    m_log.Info(_T("Sending UploadMiniDump..."));

    DeferredDelete deleteList;

    SOAP_STRUCT(_ns1__UploadMiniDump, request);
    request->client = CreateClientLib(deleteList);
    request->app = CreateApp(deleteList, app);
    request->addInfo = CreateDumpAddInfo(deleteList, addInfo);
    request->dump = CreateXopIncludeFromFile(deleteList, dumpFile);
    request->context = CreateXopInclude(deleteList, context);

    SOAP_STRUCT(_ns1__UploadMiniDumpResponse, response);
    ThrowOnSoapFail(m_webService.UploadMiniDump(request, response));

    return Response::CreateResponse(*response->UploadMiniDumpResult, m_log);
}

std::unique_ptr<Response> DumpUploaderWebServiceEx::UploadFullDump(const std::vector<BYTE>& context, const Application& app, int miniDumpId, const std::wstring& fullDumpZipPath, IUploadProgress* uploadProgress)
{
    m_log.Info(_T("Sending UploadFullDump..."));

    DeferredDelete deleteList;

    SOAP_STRUCT(_ns1__UploadFullDump, request);
    request->client = CreateClientLib(deleteList);
    request->app = CreateApp(deleteList, app);
    request->miniDumpID = &miniDumpId;
    request->dumpInZip = CreateXopIncludeFromFile(deleteList, fullDumpZipPath);
    request->context = CreateXopInclude(deleteList, context);

    std::unique_ptr<ProgressNotifier> scopedProgress;
    if (uploadProgress)
        scopedProgress.reset(new ProgressNotifier(m_webService, request->dumpInZip->__size, *uploadProgress));

    SOAP_STRUCT(_ns1__UploadFullDumpResponse, response);
    ThrowOnSoapFail(m_webService.UploadFullDump(request, response));

    return Response::CreateResponse(*response->UploadFullDumpResult, m_log);
}

std::unique_ptr<Response> DumpUploaderWebServiceEx::UploadAdditionalInfo(const std::vector<BYTE>& context, const Application& app, int miniDumpId, const std::wstring& addInfoFile, IUploadProgress* uploadProgress)
{
    m_log.Info(_T("Sending UploadAdditionalInfo..."));

    DeferredDelete deleteList;

    SOAP_STRUCT(_ns1__UploadAdditionalInfo, request);
    request->client = CreateClientLib(deleteList);
    request->app = CreateApp(deleteList, app);
    request->miniDumpID = &miniDumpId;
    request->info = CreateXopIncludeFromFile(deleteList, addInfoFile);
    request->context = CreateXopInclude(deleteList, context);

    std::unique_ptr<ProgressNotifier> scopedProgress;
    if (uploadProgress)
        scopedProgress.reset(new ProgressNotifier(m_webService, request->info->__size, *uploadProgress));

    SOAP_STRUCT(_ns1__UploadAdditionalInfoResponse, response);
    ThrowOnSoapFail(m_webService.UploadAdditionalInfo(request, response));

    return Response::CreateResponse(*response->UploadAdditionalInfoResult, m_log);
}

std::unique_ptr<Response> DumpUploaderWebServiceEx::RejectedToSendAdditionalInfo(const std::vector<BYTE>& context, const Application& app, int miniDumpId)
{
    m_log.Info(_T("Sending SendAdditionalInfoUploadRejected..."));

    DeferredDelete deleteList;

    SOAP_STRUCT(_ns1__RejectedToSendAdditionalInfo, request);
    request->client = CreateClientLib(deleteList);
    request->app = CreateApp(deleteList, app);
    request->miniDumpID = &miniDumpId;
    request->context = CreateXopInclude(deleteList, context);

    SOAP_STRUCT(_ns1__RejectedToSendAdditionalInfoResponse, response);
    ThrowOnSoapFail(m_webService.RejectedToSendAdditionalInfo(request, response));

    return Response::CreateResponse(*response->RejectedToSendAdditionalInfoResult, m_log);
}
