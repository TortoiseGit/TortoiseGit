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

#include "StdAfx.h"
#include "Serializer.h"

CString Serializer::GetHex() const
{
    CString res;
    for (size_t i = 0, size = m_storage.size(); i < size; ++i)
        res.AppendFormat(_T("%02X"), (DWORD)m_storage[i]);
    return res;
}

Serializer::Serializer() : m_buf(NULL), m_size(0) {}

Serializer::Serializer(const CString& hex)
{
    size_t bufLen = hex.GetLength() / 2;
    for (size_t i = 0; i < bufLen; ++i)
    {
        DWORD t;
        if (1 != swscanf_s((LPCTSTR)hex + i * 2, L"%02X", &t))
            throw std::runtime_error("invalid buffer");
        m_storage.push_back((BYTE)t);
    }
    m_buf = &m_storage[0];
    m_size = m_storage.size();
}

Serializer::Serializer(const BYTE* buf, size_t size) : m_buf(buf), m_size(size) {}

Serializer& Serializer::SerSimpleType(BYTE* ptr, size_t size)
{
    if (m_buf)
    {
        if (m_size < size)
            throw std::runtime_error("invalid buffer");
        memcpy(ptr, m_buf, size);
        m_buf += size;
        m_size -= size;
    }
    else
    {
        m_storage.insert(m_storage.end(), ptr, ptr + size);
    }
    return *this;
}

Serializer& Serializer::operator << (CStringA& val)
{
    DWORD len = val.GetLength();
    SerSimpleType((BYTE*)&len, sizeof(len));
    SerSimpleType((BYTE*)val.GetBuffer(len), len * sizeof(char));
    val.ReleaseBuffer(len);
    return *this;
}

Serializer& Serializer::operator << (CStringW& val)
{
    DWORD len = val.GetLength();
    SerSimpleType((BYTE*)&len, sizeof(len));
    SerSimpleType((BYTE*)val.GetBuffer(len), len * sizeof(wchar_t));
    val.ReleaseBuffer(len);
    return *this;
}

Serializer& operator << (Serializer& ser, MINIDUMP_EXCEPTION_INFORMATION& val)
{
    ser.SerSimpleType((BYTE*)&val.ExceptionPointers, sizeof(val.ExceptionPointers));
    return ser << val.ThreadId << val.ClientPointers;
}

template <typename T1, typename T2>
Serializer& operator << (Serializer& ser, std::pair<T1, T2>& val)
{
    return ser << val.first << val.second;
}

template <typename T>
Serializer& operator << (Serializer& ser, std::vector<T>& val)
{
    size_t size = val.size();
    ser.SerSimpleType((BYTE*)&size, sizeof(size));
    if (ser.IsReading())
        val.resize(size);
    for (size_t i = 0;  i < val.size(); ++i)
        ser << val[i];
    return ser;
}

template <typename K, typename V>
Serializer& operator << (Serializer& ser, std::map<K, V>& val)
{
    size_t size = val.size();
    ser.SerSimpleType((BYTE*)&size, sizeof(size));
    if (ser.IsReading())
    {
        for (size_t i = 0;  i < size; ++i)
        {
            std::pair<K, V> item;
            ser << item;
            val.insert(item);
        }
    }
    else
    {
        for (std::map<K, V>::iterator it = val.begin(); it != val.end(); ++it)
        {
            std::pair<K, V> item = *it;
            ser << item;
        }
    }
    return ser;
}

Serializer& operator << (Serializer& ser, Config& cfg)
{
    return ser << cfg.Prefix
        << cfg.AppName
        << cfg.Company
        << cfg.PrivacyPolicyUrl
        << cfg.V[0] << cfg.V[1] << cfg.V[2] << cfg.V[3]
        << cfg.Hotfix
        << cfg.ApplicationGUID
        << cfg.ServiceMode
        << cfg.LeaveDumpFilesInTempFolder
        << cfg.OpenProblemInBrowser
        << cfg.UseWER
        << cfg.SubmitterID
        << cfg.SendAdditionalDataWithoutApproval
        << cfg.FullDumpType
        << cfg.ProcessName
        << cfg.LangFilePath
        << cfg.SendRptPath
        << cfg.DbgHelpPath
        << cfg.FilesToAttach
        << cfg.UserInfo
        << cfg.CustomInfo;
}

Serializer& operator << (Serializer& ser, Params& param)
{
    return ser << param.Process
        << param.ProcessId
        << param.ExceptInfo
        << param.WasAssert
        << param.ReportReady
        << param.Group;
}