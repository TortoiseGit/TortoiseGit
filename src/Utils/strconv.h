/*************************************************************************************
  This file is a part of CrashRpt library.

  Copyright (c) 2003, Michael Carruth
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   * Neither the name of the author nor the names of its contributors
     may be used to endorse or promote products derived from this software without
     specific prior written permission.


  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

// File: strconv.h
// Description: String conversion class
// Author: zexspectrum
// Date: 2009-2010

#ifndef _STRCONV_H
#define _STRCONV_H

#include <vector>

class strconv_t
{
public:
  strconv_t(){}
  ~strconv_t()
  {
    unsigned i;
    for (i = 0; i < m_ConvertedStrings.size(); ++i)
    {
      delete [] m_ConvertedStrings[i];
    }
  }

  LPCWSTR a2w(LPCSTR lpsz)
  {
    if(lpsz==NULL)
      return NULL;

    int count = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpsz, -1, NULL, 0);
    if(count==0)
      return NULL;

    void* pBuffer = (void*) new wchar_t[count];
    int result = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpsz, -1, (LPWSTR)pBuffer, count);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }

    m_ConvertedStrings.push_back(pBuffer);
    return (LPCWSTR)pBuffer;
  }

  LPCSTR w2a(LPCWSTR lpsz)
  {
    if(lpsz==NULL)
      return NULL;

    int count = WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, NULL, 0, NULL, NULL);
    if(count==0)
      return NULL;

    void* pBuffer = (void*) new char[count];
    int result = WideCharToMultiByte(CP_ACP, 0, lpsz, -1, (LPSTR)pBuffer, count, NULL, NULL);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }

    m_ConvertedStrings.push_back(pBuffer);
    return (LPCSTR)pBuffer;
  }

  // Converts UNICODE little endian string to UNICODE big endian
  LPCWSTR w2w_be(LPCWSTR lpsz, UINT cch)
  {
    if(lpsz==NULL)
      return NULL;

    WCHAR* pBuffer = new WCHAR[cch+1];
    for (UINT i = 0; i < cch; ++i)
    {
      // Swap bytes
      pBuffer[i] = (WCHAR)MAKEWORD((lpsz[i]>>8), (lpsz[i]&0xFF));
    }

    pBuffer[cch] = 0; // Zero terminator

    m_ConvertedStrings.push_back((void*)pBuffer);
    return (LPCWSTR)pBuffer;
  }

  LPCSTR a2utf8(LPCSTR lpsz)
  {
    if(lpsz==NULL)
      return NULL;

    // 1. Convert input ANSI string to widechar using
    // MultiByteToWideChar(CP_ACP, ...) function (CP_ACP
    // is current Windows system Ansi code page)

    // Calculate required buffer size
    int count = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpsz, -1, NULL, 0);
    if(count==0)
      return NULL;

    // Convert ANSI->UNICODE
    wchar_t* pBuffer = new wchar_t[count];
    int result = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpsz, -1, (LPWSTR)pBuffer, count);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }

    // 2. Convert output widechar string from previous call to
    // UTF-8 using WideCharToMultiByte(CP_UTF8, ...)  function

    LPCSTR pszResult = (LPCSTR)w2utf8(pBuffer);
    delete [] pBuffer;
    return pszResult;
  }

  LPCSTR w2utf8(LPCWSTR lpsz)
  {
    if(lpsz==NULL)
      return NULL;

    // Calculate required buffer size
    int count = WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, NULL, 0, NULL, NULL);
    if(count==0)
    {
      return NULL;
    }

    // Convert UNICODE->UTF8
    LPSTR pBuffer = new char[count];
    int result = WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, (LPSTR)pBuffer, count, NULL, NULL);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }

    m_ConvertedStrings.push_back(pBuffer);
    return (LPCSTR)pBuffer;
  }

  LPCWSTR utf82w(LPCSTR lpsz)
  {
    if(lpsz==NULL)
      return NULL;

    // Calculate required buffer size
    int count = MultiByteToWideChar(CP_UTF8, 0, lpsz, -1, NULL, 0);
    if(count==0)
    {
      return NULL;
    }

    // Convert UNICODE->UTF8
    LPWSTR pBuffer = new wchar_t[count];
    int result = MultiByteToWideChar(CP_UTF8, 0, lpsz, -1, (LPWSTR)pBuffer, count);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }

    m_ConvertedStrings.push_back(pBuffer);
    return (LPCWSTR)pBuffer;
  }

  LPCWSTR utf82w(LPCSTR pStr, UINT cch)
  {
    if(pStr==NULL)
      return NULL;

    // Calculate required buffer size
    int count = MultiByteToWideChar(CP_UTF8, 0, pStr, cch, NULL, 0);
    if(count==0)
    {
      return NULL;
    }

    // Convert UNICODE->UTF8
    LPWSTR pBuffer = new wchar_t[count+1];
    int result = MultiByteToWideChar(CP_UTF8, 0, pStr, cch, (LPWSTR)pBuffer, count);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }

    // Zero-terminate
    pBuffer[count]=0;

    m_ConvertedStrings.push_back(pBuffer);
    return (LPCWSTR)pBuffer;
  }

  LPCSTR utf82a(LPCSTR lpsz)
  {
    return w2a(utf82w(lpsz));
  }

  LPCTSTR utf82t(LPCSTR lpsz)
  {
#ifdef UNICODE
    return utf82w(lpsz);
#else
    return utf82a(lpsz);
#endif
  }

  LPCSTR t2a(LPCTSTR lpsz)
  {
#ifdef UNICODE
    return w2a(lpsz);
#else
    return lpsz;
#endif
  }

LPCWSTR t2w(LPCTSTR lpsz)
  {
#ifdef UNICODE
    return lpsz;
#else
    return a2w(lpsz);
#endif
  }

  LPCTSTR a2t(LPCSTR lpsz)
  {
#ifdef UNICODE
    return a2w(lpsz);
#else
    return lpsz;
#endif
  }

LPCTSTR w2t(LPCWSTR lpsz)
  {
#ifdef UNICODE
    return lpsz;
#else
    return w2a(lpsz);
#endif
  }

LPCSTR t2utf8(LPCTSTR lpsz)
  {
#ifdef UNICODE
    return w2utf8(lpsz);
#else
    return a2utf8(lpsz);
#endif
  }

private:
  std::vector<void*> m_ConvertedStrings;
};

#endif  //_STRCONV_H


