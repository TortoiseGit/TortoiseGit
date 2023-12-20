/*
 * Common pieces between the platform console frontend modules.
 */

#include <stdbool.h>
#include <stdarg.h>

#include "putty.h"
#include "misc.h"
#include "console.h"

//const char console_abandoned_msg[] = "Connection abandoned.\n";

const SeatDialogPromptDescriptions *console_prompt_descriptions(Seat *seat)
{
    static const SeatDialogPromptDescriptions descs = {
        .hk_accept_action = "hit Yes",
        .hk_connect_once_action = "hit No",
        .hk_cancel_action = "hit Cancel",
        .hk_cancel_action_Participle = "Hitting Cancel",
        .weak_accept_action = "hit Yes",
        .weak_cancel_action = "hit No",
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
