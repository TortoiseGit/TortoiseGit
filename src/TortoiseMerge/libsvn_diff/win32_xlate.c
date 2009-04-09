/*
 * win32_xlate.c : Windows xlate stuff.
 *
 * ====================================================================
 * Copyright (c) 2007 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */

#ifdef WIN32

/* Define _WIN32_DCOM for CoInitializeEx(). */
#define _WIN32_DCOM

/* We must include windows.h ourselves or apr.h includes it for us with
   many ignore options set. Including Winsock is required to resolve IPv6
   compilation errors. APR_HAVE_IPV6 is only defined after including
   apr.h, so we can't detect this case here. */

/* winsock2.h includes windows.h */
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mlang.h>

#include <apr.h>
#include <apr_errno.h>
#include <apr_portable.h>

#include "svn_pools.h"
#include "svn_string.h"
#include "svn_utf.h"

#include "win32_xlate.h"

typedef struct win32_xlate_t
{
  UINT from_page_id;
  UINT to_page_id;
} win32_xlate_t;

static apr_status_t
get_page_id_from_name(UINT *page_id_p, const char *page_name, apr_pool_t *pool)
{
  IMultiLanguage * mlang = NULL;
  HRESULT hr;
  MIMECSETINFO page_info;
  WCHAR ucs2_page_name[128];

  if (page_name == SVN_APR_DEFAULT_CHARSET)
    {
        *page_id_p = CP_ACP;
        return APR_SUCCESS;
    }
  else if (page_name == SVN_APR_LOCALE_CHARSET)
    {
      OSVERSIONINFO ver_info;
      ver_info.dwOSVersionInfoSize = sizeof(ver_info);

      /* CP_THREAD_ACP supported only on Windows 2000 and later.*/
      if (GetVersionEx(&ver_info) && ver_info.dwMajorVersion >= 5
          && ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
          *page_id_p = CP_THREAD_ACP;
          return APR_SUCCESS;
        }

      /* CP_THREAD_ACP isn't supported on current system, so get locale
         encoding name from APR. */
      page_name = apr_os_locale_encoding(pool);
    }
  else if (!strcmp(page_name, "UTF-8"))
    {
      *page_id_p = CP_UTF8;
      return APR_SUCCESS;
    }

  /* Use codepage identifier nnn if the codepage name is in the form
     of "CPnnn".
     We need this code since apr_os_locale_encoding() and svn_cmdline_init()
     generates such codepage names even if they are not valid IANA charset
     name. */
  if ((page_name[0] == 'c' || page_name[0] == 'C')
      && (page_name[1] == 'p' || page_name[1] == 'P'))
    {
      *page_id_p = atoi(page_name + 2);
      return APR_SUCCESS;
    }

  hr = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IMultiLanguage, (void **) &mlang);

  if (FAILED(hr))
    return APR_EGENERAL;

  /* Convert page name to wide string. */
  MultiByteToWideChar(CP_UTF8, 0, page_name, -1, ucs2_page_name,
                      sizeof(ucs2_page_name) / sizeof(ucs2_page_name[0]));
  memset(&page_info, 0, sizeof(page_info));
  hr = mlang->lpVtbl->GetCharsetInfo(mlang, ucs2_page_name, &page_info);
  if (FAILED(hr))
    {
      mlang->lpVtbl->Release(mlang);
      return APR_EINVAL;
    }

  if (page_info.uiInternetEncoding)
    *page_id_p = page_info.uiInternetEncoding;
  else
    *page_id_p = page_info.uiCodePage;

  mlang->lpVtbl->Release(mlang);

  return APR_SUCCESS;
}

apr_status_t
svn_subr__win32_xlate_open(win32_xlate_t **xlate_p, const char *topage,
                           const char *frompage, apr_pool_t *pool)
{
  UINT from_page_id, to_page_id;
  apr_status_t apr_err = APR_SUCCESS;
  win32_xlate_t *xlate;
  HRESULT hr;

  /* First try to initialize for apartment-threaded object concurrency. */
  hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (hr == RPC_E_CHANGED_MODE)
    {
      /* COM already initalized for multi-threaded object concurrency. We are
         neutral to object concurrency so try to initalize it in the same way
         for us. */
      hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }

  if (FAILED(hr))
    return APR_EGENERAL;

  apr_err = get_page_id_from_name(&to_page_id, topage, pool);
  if (apr_err == APR_SUCCESS)
    apr_err = get_page_id_from_name(&from_page_id, frompage, pool);

  if (apr_err == APR_SUCCESS)
    {
      xlate = apr_palloc(pool, sizeof(*xlate));
      xlate->from_page_id = from_page_id;
      xlate->to_page_id = to_page_id;

      *xlate_p = xlate;
    }

  CoUninitialize();
  return apr_err;
}

apr_status_t
svn_subr__win32_xlate_to_stringbuf(win32_xlate_t *handle,
                                   const char *src_data,
                                   apr_size_t src_length,
                                   svn_stringbuf_t **dest,
                                   apr_pool_t *pool)
{
  WCHAR * wide_str;
  int retval, wide_size;

  *dest = svn_stringbuf_create("", pool);

  if (src_length == 0)
    return APR_SUCCESS;

  retval = MultiByteToWideChar(handle->from_page_id, 0, src_data, src_length,
                               NULL, 0);
  if (retval == 0)
    return apr_get_os_error();

  wide_size = retval;

  /* Allocate temporary buffer for small strings on stack instead of heap. */
  if (wide_size <= MAX_PATH)
    {
      wide_str = _alloca(wide_size * sizeof(WCHAR));
    }
  else
    {
      wide_str = apr_palloc(pool, wide_size * sizeof(WCHAR));
    }

  retval = MultiByteToWideChar(handle->from_page_id, 0, src_data, src_length,
                               wide_str, wide_size);

  if (retval == 0)
    return apr_get_os_error();

  retval = WideCharToMultiByte(handle->to_page_id, 0, wide_str, wide_size,
                               NULL, 0, NULL, NULL);

  if (retval == 0)
    return apr_get_os_error();

  /* Ensure that buffer is enough to hold result string and termination
     character. */
  svn_stringbuf_ensure(*dest, retval + 1);
  (*dest)->len = retval;

  retval = WideCharToMultiByte(handle->to_page_id, 0, wide_str, wide_size,
                               (*dest)->data, (*dest)->len, NULL, NULL);
  if (retval == 0)
    return apr_get_os_error();

  (*dest)->len = retval;
  return APR_SUCCESS;
}

#endif /* WIN32 */
