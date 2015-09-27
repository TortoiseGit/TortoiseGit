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

#include <exception>
#include <functional>
#include "..\..\CommonLibs\Log\log.h"
#include "..\DumpUploaderServiceLib\DumpUploaderWebService.h"
#include "DumpUploader.h"

struct soap;

namespace soap_helpers
{
    template <typename S>
    class SoapStuct
    {
        std::function<S* (soap*, int)> m_create;
        std::function<void (soap*, S*)> m_deleter;
        S* m_s;
        soap* m_webService;
        SoapStuct(const SoapStuct&);
        SoapStuct(const SoapStuct&& other);
    public:
        SoapStuct(soap* webService,
            std::function<S* (soap*, int)> create,
            std::function<void (soap*, S*)> deleter)
            : m_webService(webService)
            , m_create(create)
            , m_deleter(deleter)
        {
            m_s = m_create(m_webService, 1);
        }
        ~SoapStuct()
        {
            if (m_s)
                m_deleter(m_webService, m_s);
        }

        operator S*() { return m_s; }
        S* operator->() { return m_s; }
        S* get() { return m_s; }

        std::shared_ptr<void> m_data;
    };
#define SOAP_STRUCT(Str, Var) soap_helpers::SoapStuct<Str> Var(&m_webService, soap_new_##Str, soap_delete_##Str)
#define SOAP_SHARED_STRUCT(Str, Var) std::shared_ptr<soap_helpers::SoapStuct<Str>> Var(new soap_helpers::SoapStuct<Str>(&m_webService, soap_new_##Str, soap_delete_##Str))
}

namespace doctor_dump
{
    struct DumpAdditionalInfo
    {
        time_t crashDate;
        int PCID;
        int submitterID;
        std::wstring group;
        std::wstring description;
    };

    struct Response
    {
        std::wstring clientID;
        int problemID;
        int dumpGroupID;
        int dumpID;
        std::wstring urlToProblem;
        std::vector<BYTE> context;

        enum ResponseType {
            HaveSolutionResponseType,
            NeedMiniDumpResponseType,
            NeedFullDumpResponseType,
            NeedMoreInfoResponseType,
            StopResponseType,
            ErrorResponseType,
        };

        virtual ResponseType GetResponseType() const = 0;

        static std::unique_ptr<Response> CreateResponse(const ns1__Response& response, Log& log);
    };

    struct HaveSolutionResponse: public Response
    {
        bool askConfirmation;
        ns4__HaveSolutionResponse_SolutionType type;
        std::wstring url;
        std::vector<BYTE> exe;
        ResponseType GetResponseType() const override { return HaveSolutionResponseType; }
    };

    struct NeedMiniDumpResponse: public Response
    {
        ResponseType GetResponseType() const override { return NeedMiniDumpResponseType; }
    };

    struct NeedFullDumpResponse: public Response
    {
        DWORD restrictedDumpType;
        bool attachUserInfo;
        ResponseType GetResponseType() const override { return NeedFullDumpResponseType; }
    };

    struct NeedMoreInfoResponse: public Response
    {
        std::vector<BYTE> infoModule;
        std::wstring infoModuleCfg;
        ResponseType GetResponseType() const override { return NeedMoreInfoResponseType; }
    };

    struct StopResponse: public Response
    {
        ResponseType GetResponseType() const override { return StopResponseType; }
    };

    struct ErrorResponse: public Response
    {
        std::wstring error;
        ResponseType GetResponseType() const override { return ErrorResponseType; }
    };

    struct IUploadProgress
    {
        virtual void OnProgress(SIZE_T total, SIZE_T sent) = 0;
    };

    class SoapException: public std::runtime_error
    {
    public:
        explicit SoapException(int soapError, const std::string& message) : m_soapError(soapError), std::runtime_error(message) {}
        explicit SoapException(int soapError, const char* message) : m_soapError(soapError), std::runtime_error(message) {}
        int m_soapError;
        bool IsNetworkProblem() const { return m_soapError == SOAP_TCP_ERROR; }
    };

    class DumpUploaderWebServiceEx
    {
    public:
        DumpUploaderWebServiceEx(Log& log_);

        std::unique_ptr<Response> Hello(const Application& app, std::wstring appName, std::wstring companyName,const DumpAdditionalInfo& addInfo);
        std::unique_ptr<Response> UploadMiniDump(const std::vector<BYTE>& context, const Application& app, const DumpAdditionalInfo& addInfo, const std::wstring& dumpFile);
        std::unique_ptr<Response> UploadFullDump(const std::vector<BYTE>& context, const Application& app, int miniDumpId, const std::wstring& fullDumpZipPath, IUploadProgress* uploadProgress);
        std::unique_ptr<Response> UploadAdditionalInfo(const std::vector<BYTE>& context, const Application& app, int miniDumpId, const std::wstring& addInfoFile, IUploadProgress* uploadProgress);
        std::unique_ptr<Response> RejectedToSendAdditionalInfo(const std::vector<BYTE>& context, const Application& app, int miniDumpId);

    private:
        Log& m_log;
        DumpUploaderWebService m_webService;

        typedef std::vector<std::shared_ptr<void>> DeferredDelete;
        _xop__Include* CreateXopInclude(DeferredDelete& context, const std::vector<BYTE>& data);
        _xop__Include* CreateXopIncludeFromFile(DeferredDelete& context, const std::wstring& path);
        ns1__ClientLib* CreateClientLib(DeferredDelete& context);
        ns1__Application* CreateApp(DeferredDelete& context, const Application& appsrc);
        ns1__AppAdditionalInfo* CreateAppAddInfo(DeferredDelete& context, std::wstring& appName, std::wstring& companyName);
        ns1__DumpAdditionalInfo* CreateDumpAddInfo(DeferredDelete& context, const DumpAdditionalInfo &addInfo);
        void ThrowOnSoapFail(int res);
        static std::vector<unsigned char> ReadFile(const std::wstring& path);
    };

}