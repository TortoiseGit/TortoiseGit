/*
 * Return the full pathname of the global mutex that Pageant uses at
 * startup to atomically decide whether to be a server or a client.
 */

#include "putty.h"

char *agent_mutex_name(void)
{
    char *username = get_username();
    char *mutexname = dupprintf("Local\\pageant-mutex.%s", username);
    sfree(username);
    return mutexname;
}
