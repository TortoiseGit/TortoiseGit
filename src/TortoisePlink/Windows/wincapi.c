/*
 * wincapi.c: implementation of wincapi.h.
 */

#include "putty.h"

#if !defined NO_SECURITY

#define WINCAPI_GLOBAL
#include "wincapi.h"

int got_crypt(void)
{
    static int attempted = FALSE;
    static int successful;
    static HMODULE crypt;

    if (!attempted) {
        attempted = TRUE;
        crypt = load_system32_dll("crypt32.dll");
        successful = crypt &&
            GET_WINDOWS_FUNCTION(crypt, CryptProtectMemory);
    }
    return successful;
}

#endif /* !defined NO_SECURITY */
