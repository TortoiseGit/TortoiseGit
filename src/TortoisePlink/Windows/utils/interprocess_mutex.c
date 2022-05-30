/*
 * Lock and unlock a mutex with a globally visible name. Used to
 * synchronise between unrelated processes, such as two
 * connection-sharing PuTTYs deciding which will be the upstream.
 */

#include "putty.h"
#include "security-api.h"

HANDLE lock_interprocess_mutex(const char *mutexname, char **error)
{
    PSECURITY_DESCRIPTOR psd = NULL;
    PACL acl = NULL;
    HANDLE mutex = NULL;

    if (should_have_security() && !make_private_security_descriptor(
            MUTEX_ALL_ACCESS, &psd, &acl, error))
        goto out;

    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = psd;
    sa.bInheritHandle = false;

    mutex = CreateMutex(&sa, false, mutexname);
    if (!mutex) {
        *error = dupprintf("CreateMutex(\"%s\") failed: %s",
                           mutexname, win_strerror(GetLastError()));
        goto out;
    }

    WaitForSingleObject(mutex, INFINITE);

  out:
    if (psd)
        LocalFree(psd);
    if (acl)
        LocalFree(acl);

    return mutex;
}

void unlock_interprocess_mutex(HANDLE mutex)
{
    ReleaseMutex(mutex);
    CloseHandle(mutex);
}
