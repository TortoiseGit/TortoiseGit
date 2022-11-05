/*
 * Common pieces between the platform console frontend modules.
 */

#include <stdbool.h>
#include <stdarg.h>

#include "putty.h"
#include "misc.h"
#include "console.h"

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

const SeatDialogPromptDescriptions *console_prompt_descriptions(Seat *seat)
{
    static const SeatDialogPromptDescriptions descs = {
        .hk_accept_action = "hit Yes",
        .hk_connect_once_action = "hit No",
        .hk_cancel_action = "hit Cancel",
        .hk_cancel_action_Participle = "Hitting Cancel",
    };
    return &descs;
}

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
