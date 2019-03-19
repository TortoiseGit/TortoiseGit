/*
 * wincons.c - various interactive-prompt routines shared between
 * the Windows console PuTTY tools
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "putty.h"
#include "storage.h"
#include "ssh.h"

#include "LoginDialog.h"

bool console_batch_mode = false;

/*
 * Clean up and exit.
 */
void cleanup_exit(int code)
{
    /*
     * Clean up.
     */
    sk_cleanup();

    random_save_seed();

    exit(code);
}

void modalfatalbox(const char *fmt, ...)
{
    va_list ap;
    char *stuff, morestuff[100];
    va_start(ap, fmt);
    stuff = dupvprintf(fmt, ap);
    va_end(ap);
    sprintf(morestuff, "%.70s Fatal Error", appname);
    MessageBox(GetParentHwnd(), stuff, morestuff,
        MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
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
    MessageBox(GetParentHwnd(), stuff, morestuff,
        MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
    sfree(stuff);
}

void console_connection_fatal(Seat *seat, const char *msg)
{
    char morestuff[100];
    sprintf(morestuff, "%.70s Fatal Error", appname);
    MessageBox(GetParentHwnd(), msg, morestuff,
        MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
    cleanup_exit(1);
}

void timer_change_notify(unsigned long next)
{
}

int console_verify_ssh_host_key(
    Seat *seat, const char *host, int port,
    const char *keytype, char *keystr, char *fingerprint,
    void (*callback)(void *ctx, int result), void *ctx)
{
    int ret;

    static const char absentmsg_batch[] =
	"The server's host key is not cached in the registry. You\n"
	"have no guarantee that the server is the computer you\n"
	"think it is.\n"
	"The server's %s key fingerprint is:\n"
	"%s\n"
	"Connection abandoned.\n";
    static const char absentmsg[] =
	"The server's host key is not cached in the registry. You\n"
	"have no guarantee that the server is the computer you\n"
	"think it is.\n"
	"The server's %s key fingerprint is:\n"
	"%s\n"
	"If you trust this host, hit Yes to add the key to\n"
	"PuTTY's cache and carry on connecting.\n"
	"If you want to carry on connecting just once, without\n"
	"adding the key to the cache, hit No.\n"
	"If you do not trust this host, hit Cancel to abandon the\n"
	"connection.\n";

    static const char wrongmsg_batch[] =
	"WARNING - POTENTIAL SECURITY BREACH!\n"
	"The server's host key does not match the one PuTTY has\n"
	"cached in the registry. This means that either the\n"
	"server administrator has changed the host key, or you\n"
	"have actually connected to another computer pretending\n"
	"to be the server.\n"
	"The new %s key fingerprint is:\n"
	"%s\n"
	"Connection abandoned.\n";
    static const char wrongmsg[] =
	"WARNING - POTENTIAL SECURITY BREACH!\n"
	"\n"
	"The server's host key does not match the one PuTTY has\n"
	"cached in the registry. This means that either the\n"
	"server administrator has changed the host key, or you\n"
	"have actually connected to another computer pretending\n"
	"to be the server.\n"
	"The new %s key fingerprint is:\n"
	"%s\n"
	"If you were expecting this change and trust the new key,\n"
	"hit Yes to update PuTTY's cache and continue connecting.\n"
	"If you want to carry on connecting but without updating\n"
	"the cache, hit No.\n"
	"If you want to abandon the connection completely, hit\n"
	"Cancel. Hitting Cancel is the ONLY guaranteed safe\n" "choice.\n";

    static const char abandoned[] = "Connection abandoned.\n";

	static const char mbtitle[] = "%s Security Alert";

    /*
     * Verify the key against the registry.
     */
    ret = verify_host_key(host, port, keytype, keystr);

    if (ret == 0)		       /* success - key matched OK */
	return 1;

    if (ret == 2) {		       /* key was different */
	int mbret;
	char *message, *title;

	message = dupprintf(wrongmsg, keytype, fingerprint);
	title = dupprintf(mbtitle, appname);

	mbret = MessageBox(GetParentHwnd(), message, title, MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3);
	sfree(message);
	sfree(title);
	if (mbret == IDYES) {
		store_host_key(host, port, keytype, keystr);
		return 1;
	}
	else if (mbret == IDNO) 
	{
		return 1;
	}
	else
		return 0;
	}

    if (ret == 1) {		       /* key was absent */
	int mbret;
	char *message, *title;
	message = dupprintf(absentmsg, keytype, fingerprint);
	title = dupprintf(mbtitle, appname);
	mbret = MessageBox(GetParentHwnd(), message, title,
		MB_ICONWARNING | MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3);
	sfree(message);
	sfree(title);
	if (mbret == IDYES)
	{
		store_host_key(host, port, keytype, keystr);
		return 1;
	}
	else if (mbret == IDNO)
	{
		return 1;
	}
	else
		return 0;
    }
	return 1;
}

int console_confirm_weak_crypto_primitive(
    Seat *seat, const char *algtype, const char *algname,
    void (*callback)(void *ctx, int result), void *ctx)
{
    static const char msg[] =
	"The first %s supported by the server is\n"
	"%s, which is below the configured warning threshold.\n"
	"Continue with connection? (y/n) ";

	static const char mbtitle[] = "%s Security Alert";

	int mbret;
	char *message, *title;

	message = dupprintf(msg, algtype, algname);
	title = dupprintf(mbtitle, appname);

	mbret = MessageBox(GetParentHwnd(), message, title, MB_ICONWARNING|MB_YESNO);
	sfree(message);
	sfree(title);
	if (mbret == IDYES)
		return 1;
	else
		return 0;
}

int console_confirm_weak_cached_hostkey(
    Seat *seat, const char *algname, const char *betteralgs,
    void (*callback)(void *ctx, int result), void *ctx)
{
    HANDLE hin;
    DWORD savemode, i;

    static const char msg[] =
	"The first host key type we have stored for this server\n"
	"is %s, which is below the configured warning threshold.\n"
	"The server also provides the following types of host key\n"
        "above the threshold, which we do not have stored:\n"
        "%s\n"
	"Continue with connection? (y/n) ";
    static const char msg_batch[] =
	"The first host key type we have stored for this server\n"
	"is %s, which is below the configured warning threshold.\n"
	"The server also provides the following types of host key\n"
        "above the threshold, which we do not have stored:\n"
        "%s\n"
	"Connection abandoned.\n";
    static const char abandoned[] = "Connection abandoned.\n";

    char line[32];

    int mbret;
    char *message, *title;
    static const char mbtitle[] = "%s Security Alert";

    message = dupprintf(msg, algname, betteralgs);
    title = dupprintf(mbtitle, appname);

    mbret = MessageBox(GetParentHwnd(), message, title, MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3);
    sfree(message);
    sfree(title);

    if (mbret == IDYES) {
	return 1;
    } else {
	return 0;
    }
}

bool is_interactive(void)
{
    return is_console_handle(GetStdHandle(STD_INPUT_HANDLE));
}

bool console_antispoof_prompt = true;
bool console_set_trust_status(Seat *seat, bool trusted)
{
    if (console_batch_mode || !is_interactive() || !console_antispoof_prompt) {
        /*
         * In batch mode, we don't need to worry about the server
         * mimicking our interactive authentication, because the user
         * already knows not to expect any.
         *
         * If standard input isn't connected to a terminal, likewise,
         * because even if the server did send a spoof authentication
         * prompt, the user couldn't respond to it via the terminal
         * anyway.
         *
         * We also vacuously return success if the user has purposely
         * disabled the antispoof prompt.
         */
        return true;
    }

    return false;
}

/*
 * Ask whether to wipe a session log file before writing to it.
 * Returns 2 for wipe, 1 for append, 0 for cancel (don't log).
 */
static int console_askappend(LogPolicy *lp, Filename *filename,
                             void (*callback)(void *ctx, int result),
                             void *ctx)
{
    HANDLE hin;
    DWORD savemode, i;

    static const char msgtemplate[] =
	"The session log file \"%.*s\" already exists.\n"
	"You can overwrite it with a new session log,\n"
	"append your session log to the end of it,\n"
	"or disable session logging for this session.\n"
	"Enter \"y\" to wipe the file, \"n\" to append to it,\n"
	"or just press Return to disable logging.\n"
	"Wipe the log file? (y/n, Return cancels logging) ";

    static const char msgtemplate_batch[] =
	"The session log file \"%.*s\" already exists.\n"
	"Logging will not be enabled.\n";

    char line[32];
    int mbret;
    char *message, *title;
    static const char mbtitle[] = "%s Session log";

    message = dupprintf(msgtemplate, FILENAME_MAX, filename->path);
    title = dupprintf(mbtitle, appname);

    mbret = MessageBox(GetParentHwnd(), message, title, MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3);
    sfree(message);
    sfree(title);

    if (mbret == IDYES)
	return 2;
    else if (mbret == IDNO)
	return 1;
    else
	return 0;
}

/*
 * Warn about the obsolescent key file format.
 * 
 * Uniquely among these functions, this one does _not_ expect a
 * frontend handle. This means that if PuTTY is ported to a
 * platform which requires frontend handles, this function will be
 * an anomaly. Fortunately, the problem it addresses will not have
 * been present on that platform, so it can plausibly be
 * implemented as an empty function.
 */
void old_keyfile_warning(void)
{
    static const char message[] =
	"You are loading an SSH-2 private key which has an\n"
	"old version of the file format. This means your key\n"
	"file is not fully tamperproof. Future versions of\n"
	"PuTTY may stop supporting this private key format,\n"
	"so we recommend you convert your key to the new\n"
	"format.\n"
	"\n"
	"Once the key is loaded into PuTTYgen, you can perform\n"
	"this conversion simply by saving it again.\n";

    fputs(message, stderr);
}

/*
 * Display the fingerprints of the PGP Master Keys to the user.
 */
void pgp_fingerprints(void)
{
    fputs("These are the fingerprints of the PuTTY PGP Master Keys. They can\n"
	  "be used to establish a trust path from this executable to another\n"
	  "one. See the manual for more information.\n"
	  "(Note: these fingerprints have nothing to do with SSH!)\n"
	  "\n"
	  "PuTTY Master Key as of " PGP_MASTER_KEY_YEAR
          " (" PGP_MASTER_KEY_DETAILS "):\n"
	  "  " PGP_MASTER_KEY_FP "\n\n"
	  "Previous Master Key (" PGP_PREV_MASTER_KEY_YEAR
          ", " PGP_PREV_MASTER_KEY_DETAILS "):\n"
	  "  " PGP_PREV_MASTER_KEY_FP "\n", stdout);
}

static void console_logging_error(LogPolicy *lp, const char *string)
{
    /* Ordinary Event Log entries are displayed in the same way as
     * logging errors, but only in verbose mode */
    fprintf(stderr, "%s\n", string);
    fflush(stderr);
}

static void console_eventlog(LogPolicy *lp, const char *string)
{
    /* Ordinary Event Log entries are displayed in the same way as
     * logging errors, but only in verbose mode */
    if (flags & FLAG_VERBOSE)
        console_logging_error(lp, string);
}

StripCtrlChars *console_stripctrl_new(
    Seat *seat, BinarySink *bs_out, SeatInteractionContext sic)
{
    return stripctrl_new(bs_out, false, 0);
}

static void console_write(HANDLE hout, ptrlen data)
{
    DWORD dummy;
    WriteFile(hout, data.ptr, data.len, &dummy, NULL);
}

int console_get_userpass_input(prompts_t *p)
{
    size_t curr_prompt;

    /*
     * Zero all the results, in case we abort half-way through.
     */
    {
	int i;
	for (i = 0; i < (int)p->n_prompts; i++)
            prompt_set_result(p->prompts[i], "");
    }

    if (console_batch_mode)
	return 0;

    for (curr_prompt = 0; curr_prompt < p->n_prompts; curr_prompt++) {
	prompt_t *pr = p->prompts[curr_prompt];
	if (!DoLoginDialog(pr->result, pr->resultsize-1, pr->prompt))
	return 0;
    }

    return 1; /* success */
}

static const LogPolicyVtable default_logpolicy_vt = {
    console_eventlog,
    console_askappend,
    console_logging_error,
};
LogPolicy default_logpolicy[1] = {{ &default_logpolicy_vt }};
