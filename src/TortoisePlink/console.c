/*
 * Common pieces between the platform console frontend modules.
 */

#include <stdbool.h>
#include <stdarg.h>

#include "putty.h"
#include "misc.h"
#include "console.h"

#include "LoginDialog.h"

const char hk_absentmsg_common_fmt[] =
    "The server's host key is not cached. You have no guarantee\n"
    "that the server is the computer you think it is.\n"
    "The server's %s key fingerprint is:\n"
    "%s\n"
    "%s\n\n"
    "If you trust this host, hit Yes to add the key to\n"
    "PuTTY's cache and carry on connecting.\n"
    "If you want to carry on connecting just once, without\n"
    "adding the key to the cache, hit No.\n"
    "If you do not trust this host, hit Cancel to abandon the\n"
    "connection.\n";

const char hk_wrongmsg_common_fmt[] =
    "WARNING - POTENTIAL SECURITY BREACH!\n"
    "\n"
    "The server's host key does not match the one PuTTY has\n"
    "cached. This means that either the server administrator\n"
    "has changed the host key, or you have actually connected\n"
    "to another computer pretending to be the server.\n"
    "The new %s key fingerprint is:\n"
    "%s\n"
    "%s\n\n"
    "If you were expecting this change and trust the new key,\n"
    "hit Yes to update PuTTY's cache and continue connecting.\n"
    "If you want to carry on connecting but without updating\n"
    "the cache, hit No.\n"
    "If you want to abandon the connection completely, hit\n"
    "Cancel. Hitting Cancel is the ONLY guaranteed safe choice.\n";

const char weakcrypto_msg_common_fmt[] =
    "The first %s supported by the server is\n"
    "%s, which is below the configured warning threshold.\n";

const char weakhk_msg_common_fmt[] =
    "The first host key type we have stored for this server\n"
    "is %s, which is below the configured warning threshold.\n"
    "The server also provides the following types of host key\n"
    "above the threshold, which we do not have stored:\n"
    "%s\n";

//const char console_continue_prompt[] = "Continue with connection? (y/n) ";
//const char console_abandoned_msg[] = "Connection abandoned.\n";

bool console_batch_mode = false;

/*
 * Error message and/or fatal exit functions, all based on
 * console_print_error_msg which the platform front end provides.
 */
void modalfatalbox(const char *fmt, ...)
{
	va_list ap;
	char *stuff, morestuff[100];
	va_start(ap, fmt);
	stuff = dupvprintf(fmt, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Fatal Error", appname);
	MessageBox(GetParentHwnd(), stuff, morestuff, MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
	sfree(stuff);
    cleanup_exit(1);
}

void nonfatal(const char *fmt, ...)
{
	va_list ap;
	char *stuff, morestuff[100];
	va_start(ap, fmt);
	stuff = dupvprintf(fmt, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Error", appname);
	MessageBox(GetParentHwnd(), stuff, morestuff, MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
	sfree(stuff);
}

void console_connection_fatal(Seat *seat, const char *msg)
{
	char morestuff[100];
	sprintf(morestuff, "%.70s Fatal Error", appname);
	MessageBox(GetParentHwnd(), msg, morestuff, MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
    cleanup_exit(1);
}

/*
 * Console front ends redo their select() or equivalent every time, so
 * they don't need separate timer handling.
 */
void timer_change_notify(unsigned long next)
{
}
