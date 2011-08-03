/*
 * settings.c: read and write saved sessions. (platform-independent)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "putty.h"
#include "storage.h"

/* The cipher order given here is the default order. */
static const struct keyvalwhere ciphernames[] = {
    { "aes",        CIPHER_AES,             -1, -1 },
    { "blowfish",   CIPHER_BLOWFISH,        -1, -1 },
    { "3des",       CIPHER_3DES,            -1, -1 },
    { "WARN",       CIPHER_WARN,            -1, -1 },
    { "arcfour",    CIPHER_ARCFOUR,         -1, -1 },
    { "des",        CIPHER_DES,             -1, -1 }
};

static const struct keyvalwhere kexnames[] = {
    { "dh-gex-sha1",        KEX_DHGEX,      -1, -1 },
    { "dh-group14-sha1",    KEX_DHGROUP14,  -1, -1 },
    { "dh-group1-sha1",     KEX_DHGROUP1,   -1, -1 },
    { "rsa",                KEX_RSA,        KEX_WARN, -1 },
    { "WARN",               KEX_WARN,       -1, -1 }
};

/*
 * All the terminal modes that we know about for the "TerminalModes"
 * setting. (Also used by config.c for the drop-down list.)
 * This is currently precisely the same as the set in ssh.c, but could
 * in principle differ if other backends started to support tty modes
 * (e.g., the pty backend).
 */
const char *const ttymodes[] = {
    "INTR",	"QUIT",     "ERASE",	"KILL",     "EOF",
    "EOL",	"EOL2",     "START",	"STOP",     "SUSP",
    "DSUSP",	"REPRINT",  "WERASE",	"LNEXT",    "FLUSH",
    "SWTCH",	"STATUS",   "DISCARD",	"IGNPAR",   "PARMRK",
    "INPCK",	"ISTRIP",   "INLCR",	"IGNCR",    "ICRNL",
    "IUCLC",	"IXON",     "IXANY",	"IXOFF",    "IMAXBEL",
    "ISIG",	"ICANON",   "XCASE",	"ECHO",     "ECHOE",
    "ECHOK",	"ECHONL",   "NOFLSH",	"TOSTOP",   "IEXTEN",
    "ECHOCTL",	"ECHOKE",   "PENDIN",	"OPOST",    "OLCUC",
    "ONLCR",	"OCRNL",    "ONOCR",	"ONLRET",   "CS7",
    "CS8",	"PARENB",   "PARODD",	NULL
};

/*
 * Convenience functions to access the backends[] array
 * (which is only present in tools that manage settings).
 */

Backend *backend_from_name(const char *name)
{
    Backend **p;
    for (p = backends; *p != NULL; p++)
	if (!strcmp((*p)->name, name))
	    return *p;
    return NULL;
}

Backend *backend_from_proto(int proto)
{
    Backend **p;
    for (p = backends; *p != NULL; p++)
	if ((*p)->protocol == proto)
	    return *p;
    return NULL;
}

int get_remote_username(Config *cfg, char *user, size_t len)
{
    if (*cfg->username) {
	strncpy(user, cfg->username, len);
	user[len-1] = '\0';
    } else {
	if (cfg->username_from_env) {
	    /* Use local username. */
	    char *luser = get_username();
	    if (luser) {
		strncpy(user, luser, len);
		user[len-1] = '\0';
		sfree(luser);
	    } else {
		*user = '\0';
	    }
	} else {
	    *user = '\0';
	}
    }
    return (*user != '\0');
}

static void gpps(void *handle, const char *name, const char *def,
		 char *val, int len)
{
    if (!read_setting_s(handle, name, val, len)) {
	char *pdef;

	pdef = platform_default_s(name);
	if (pdef) {
	    strncpy(val, pdef, len);
	    sfree(pdef);
	} else {
	    strncpy(val, def, len);
	}

	val[len - 1] = '\0';
    }
}

/*
 * gppfont and gppfile cannot have local defaults, since the very
 * format of a Filename or Font is platform-dependent. So the
 * platform-dependent functions MUST return some sort of value.
 */
static void gppfont(void *handle, const char *name, FontSpec *result)
{
    if (!read_setting_fontspec(handle, name, result))
	*result = platform_default_fontspec(name);
}
static void gppfile(void *handle, const char *name, Filename *result)
{
    if (!read_setting_filename(handle, name, result))
	*result = platform_default_filename(name);
}

static void gppi(void *handle, char *name, int def, int *i)
{
    def = platform_default_i(name, def);
    *i = read_setting_i(handle, name, def);
}

/*
 * Read a set of name-value pairs in the format we occasionally use:
 *   NAME\tVALUE\0NAME\tVALUE\0\0 in memory
 *   NAME=VALUE,NAME=VALUE, in storage
 * `def' is in the storage format.
 */
static void gppmap(void *handle, char *name, char *def, char *val, int len)
{
    char *buf = snewn(2*len, char), *p, *q;
    gpps(handle, name, def, buf, 2*len);
    p = buf;
    q = val;
    while (*p) {
	while (*p && *p != ',') {
	    int c = *p++;
	    if (c == '=')
		c = '\t';
	    if (c == '\\')
		c = *p++;
	    *q++ = c;
	}
	if (*p == ',')
	    p++;
	*q++ = '\0';
    }
    *q = '\0';
    sfree(buf);
}

/*
 * Write a set of name/value pairs in the above format.
 */
static void wmap(void *handle, char const *key, char const *value, int len)
{
    char *buf = snewn(2*len, char), *p;
    const char *q;
    p = buf;
    q = value;
    while (*q) {
	while (*q) {
	    int c = *q++;
	    if (c == '=' || c == ',' || c == '\\')
		*p++ = '\\';
	    if (c == '\t')
		c = '=';
	    *p++ = c;
	}
	*p++ = ',';
	q++;
    }
    *p = '\0';
    write_setting_s(handle, key, buf);
    sfree(buf);
}

static int key2val(const struct keyvalwhere *mapping,
                   int nmaps, char *key)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (!strcmp(mapping[i].s, key)) return mapping[i].v;
    return -1;
}

static const char *val2key(const struct keyvalwhere *mapping,
                           int nmaps, int val)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (mapping[i].v == val) return mapping[i].s;
    return NULL;
}

/*
 * Helper function to parse a comma-separated list of strings into
 * a preference list array of values. Any missing values are added
 * to the end and duplicates are weeded.
 * XXX: assumes vals in 'mapping' are small +ve integers
 */
static void gprefs(void *sesskey, char *name, char *def,
		   const struct keyvalwhere *mapping, int nvals,
		   int *array)
{
    char commalist[256];
    char *p, *q;
    int i, j, n, v, pos;
    unsigned long seen = 0;	       /* bitmap for weeding dups etc */

    /*
     * Fetch the string which we'll parse as a comma-separated list.
     */
    gpps(sesskey, name, def, commalist, sizeof(commalist));

    /*
     * Go through that list and convert it into values.
     */
    n = 0;
    p = commalist;
    while (1) {
        while (*p && *p == ',') p++;
        if (!*p)
            break;                     /* no more words */

        q = p;
        while (*p && *p != ',') p++;
        if (*p) *p++ = '\0';

        v = key2val(mapping, nvals, q);
        if (v != -1 && !(seen & (1 << v))) {
	    seen |= (1 << v);
	    array[n++] = v;
	}
    }

    /*
     * Now go through 'mapping' and add values that weren't mentioned
     * in the list we fetched. We may have to loop over it multiple
     * times so that we add values before other values whose default
     * positions depend on them.
     */
    while (n < nvals) {
        for (i = 0; i < nvals; i++) {
	    assert(mapping[i].v < 32);

	    if (!(seen & (1 << mapping[i].v))) {
                /*
                 * This element needs adding. But can we add it yet?
                 */
                if (mapping[i].vrel != -1 && !(seen & (1 << mapping[i].vrel)))
                    continue;          /* nope */

                /*
                 * OK, we can work out where to add this element, so
                 * do so.
                 */
                if (mapping[i].vrel == -1) {
                    pos = (mapping[i].where < 0 ? n : 0);
                } else {
                    for (j = 0; j < n; j++)
                        if (array[j] == mapping[i].vrel)
                            break;
                    assert(j < n);     /* implied by (seen & (1<<vrel)) */
                    pos = (mapping[i].where < 0 ? j : j+1);
                }

                /*
                 * And add it.
                 */
                for (j = n-1; j >= pos; j--)
                    array[j+1] = array[j];
                array[pos] = mapping[i].v;
                n++;
            }
        }
    }
}

/* 
 * Write out a preference list.
 */
static void wprefs(void *sesskey, char *name,
		   const struct keyvalwhere *mapping, int nvals,
		   int *array)
{
    char *buf, *p;
    int i, maxlen;

    for (maxlen = i = 0; i < nvals; i++) {
	const char *s = val2key(mapping, nvals, array[i]);
	if (s) {
            maxlen += 1 + strlen(s);
        }
    }

    buf = snewn(maxlen, char);
    p = buf;

    for (i = 0; i < nvals; i++) {
	const char *s = val2key(mapping, nvals, array[i]);
	if (s) {
            p += sprintf(p, "%s%s", (p > buf ? "," : ""), s);
	}
    }

    assert(p - buf == maxlen - 1);     /* maxlen counted the NUL */

    write_setting_s(sesskey, name, buf);

    sfree(buf);
}

char *save_settings(char *section, Config * cfg)
{
    void *sesskey;
    char *errmsg;

    sesskey = open_settings_w(section, &errmsg);
    if (!sesskey)
	return errmsg;
    save_open_settings(sesskey, cfg);
    close_settings_w(sesskey);
    return NULL;
}

void save_open_settings(void *sesskey, Config *cfg)
{
    int i;
    char *p;

    write_setting_i(sesskey, "Present", 1);
    write_setting_s(sesskey, "HostName", cfg->host);
    write_setting_filename(sesskey, "LogFileName", cfg->logfilename);
    write_setting_i(sesskey, "LogType", cfg->logtype);
    write_setting_i(sesskey, "LogFileClash", cfg->logxfovr);
    write_setting_i(sesskey, "LogFlush", cfg->logflush);
    write_setting_i(sesskey, "SSHLogOmitPasswords", cfg->logomitpass);
    write_setting_i(sesskey, "SSHLogOmitData", cfg->logomitdata);
    p = "raw";
    {
	const Backend *b = backend_from_proto(cfg->protocol);
	if (b)
	    p = b->name;
    }
    write_setting_s(sesskey, "Protocol", p);
    write_setting_i(sesskey, "PortNumber", cfg->port);
    /* The CloseOnExit numbers are arranged in a different order from
     * the standard FORCE_ON / FORCE_OFF / AUTO. */
    write_setting_i(sesskey, "CloseOnExit", (cfg->close_on_exit+2)%3);
    write_setting_i(sesskey, "WarnOnClose", !!cfg->warn_on_close);
    write_setting_i(sesskey, "PingInterval", cfg->ping_interval / 60);	/* minutes */
    write_setting_i(sesskey, "PingIntervalSecs", cfg->ping_interval % 60);	/* seconds */
    write_setting_i(sesskey, "TCPNoDelay", cfg->tcp_nodelay);
    write_setting_i(sesskey, "TCPKeepalives", cfg->tcp_keepalives);
    write_setting_s(sesskey, "TerminalType", cfg->termtype);
    write_setting_s(sesskey, "TerminalSpeed", cfg->termspeed);
    wmap(sesskey, "TerminalModes", cfg->ttymodes, lenof(cfg->ttymodes));

    /* Address family selection */
    write_setting_i(sesskey, "AddressFamily", cfg->addressfamily);

    /* proxy settings */
    write_setting_s(sesskey, "ProxyExcludeList", cfg->proxy_exclude_list);
    write_setting_i(sesskey, "ProxyDNS", (cfg->proxy_dns+2)%3);
    write_setting_i(sesskey, "ProxyLocalhost", cfg->even_proxy_localhost);
    write_setting_i(sesskey, "ProxyMethod", cfg->proxy_type);
    write_setting_s(sesskey, "ProxyHost", cfg->proxy_host);
    write_setting_i(sesskey, "ProxyPort", cfg->proxy_port);
    write_setting_s(sesskey, "ProxyUsername", cfg->proxy_username);
    write_setting_s(sesskey, "ProxyPassword", cfg->proxy_password);
    write_setting_s(sesskey, "ProxyTelnetCommand", cfg->proxy_telnet_command);
    wmap(sesskey, "Environment", cfg->environmt, lenof(cfg->environmt));
    write_setting_s(sesskey, "UserName", cfg->username);
    write_setting_i(sesskey, "UserNameFromEnvironment", cfg->username_from_env);
    write_setting_s(sesskey, "LocalUserName", cfg->localusername);
    write_setting_i(sesskey, "NoPTY", cfg->nopty);
    write_setting_i(sesskey, "Compression", cfg->compression);
    write_setting_i(sesskey, "TryAgent", cfg->tryagent);
    write_setting_i(sesskey, "AgentFwd", cfg->agentfwd);
    write_setting_i(sesskey, "GssapiFwd", cfg->gssapifwd);
    write_setting_i(sesskey, "ChangeUsername", cfg->change_username);
    wprefs(sesskey, "Cipher", ciphernames, CIPHER_MAX,
	   cfg->ssh_cipherlist);
    wprefs(sesskey, "KEX", kexnames, KEX_MAX, cfg->ssh_kexlist);
    write_setting_i(sesskey, "RekeyTime", cfg->ssh_rekey_time);
    write_setting_s(sesskey, "RekeyBytes", cfg->ssh_rekey_data);
    write_setting_i(sesskey, "SshNoAuth", cfg->ssh_no_userauth);
    write_setting_i(sesskey, "SshBanner", cfg->ssh_show_banner);
    write_setting_i(sesskey, "AuthTIS", cfg->try_tis_auth);
    write_setting_i(sesskey, "AuthKI", cfg->try_ki_auth);
    write_setting_i(sesskey, "AuthGSSAPI", cfg->try_gssapi_auth);
#ifndef NO_GSSAPI
    wprefs(sesskey, "GSSLibs", gsslibkeywords, ngsslibs,
	   cfg->ssh_gsslist);
    write_setting_filename(sesskey, "GSSCustom", cfg->ssh_gss_custom);
#endif
    write_setting_i(sesskey, "SshNoShell", cfg->ssh_no_shell);
    write_setting_i(sesskey, "SshProt", cfg->sshprot);
    write_setting_s(sesskey, "LogHost", cfg->loghost);
    write_setting_i(sesskey, "SSH2DES", cfg->ssh2_des_cbc);
    write_setting_filename(sesskey, "PublicKeyFile", cfg->keyfile);
    write_setting_s(sesskey, "RemoteCommand", cfg->remote_cmd);
    write_setting_i(sesskey, "RFCEnviron", cfg->rfc_environ);
    write_setting_i(sesskey, "PassiveTelnet", cfg->passive_telnet);
    write_setting_i(sesskey, "BackspaceIsDelete", cfg->bksp_is_delete);
    write_setting_i(sesskey, "RXVTHomeEnd", cfg->rxvt_homeend);
    write_setting_i(sesskey, "LinuxFunctionKeys", cfg->funky_type);
    write_setting_i(sesskey, "NoApplicationKeys", cfg->no_applic_k);
    write_setting_i(sesskey, "NoApplicationCursors", cfg->no_applic_c);
    write_setting_i(sesskey, "NoMouseReporting", cfg->no_mouse_rep);
    write_setting_i(sesskey, "NoRemoteResize", cfg->no_remote_resize);
    write_setting_i(sesskey, "NoAltScreen", cfg->no_alt_screen);
    write_setting_i(sesskey, "NoRemoteWinTitle", cfg->no_remote_wintitle);
    write_setting_i(sesskey, "RemoteQTitleAction", cfg->remote_qtitle_action);
    write_setting_i(sesskey, "NoDBackspace", cfg->no_dbackspace);
    write_setting_i(sesskey, "NoRemoteCharset", cfg->no_remote_charset);
    write_setting_i(sesskey, "ApplicationCursorKeys", cfg->app_cursor);
    write_setting_i(sesskey, "ApplicationKeypad", cfg->app_keypad);
    write_setting_i(sesskey, "NetHackKeypad", cfg->nethack_keypad);
    write_setting_i(sesskey, "AltF4", cfg->alt_f4);
    write_setting_i(sesskey, "AltSpace", cfg->alt_space);
    write_setting_i(sesskey, "AltOnly", cfg->alt_only);
    write_setting_i(sesskey, "ComposeKey", cfg->compose_key);
    write_setting_i(sesskey, "CtrlAltKeys", cfg->ctrlaltkeys);
    write_setting_i(sesskey, "TelnetKey", cfg->telnet_keyboard);
    write_setting_i(sesskey, "TelnetRet", cfg->telnet_newline);
    write_setting_i(sesskey, "LocalEcho", cfg->localecho);
    write_setting_i(sesskey, "LocalEdit", cfg->localedit);
    write_setting_s(sesskey, "Answerback", cfg->answerback);
    write_setting_i(sesskey, "AlwaysOnTop", cfg->alwaysontop);
    write_setting_i(sesskey, "FullScreenOnAltEnter", cfg->fullscreenonaltenter);
    write_setting_i(sesskey, "HideMousePtr", cfg->hide_mouseptr);
    write_setting_i(sesskey, "SunkenEdge", cfg->sunken_edge);
    write_setting_i(sesskey, "WindowBorder", cfg->window_border);
    write_setting_i(sesskey, "CurType", cfg->cursor_type);
    write_setting_i(sesskey, "BlinkCur", cfg->blink_cur);
    write_setting_i(sesskey, "Beep", cfg->beep);
    write_setting_i(sesskey, "BeepInd", cfg->beep_ind);
    write_setting_filename(sesskey, "BellWaveFile", cfg->bell_wavefile);
    write_setting_i(sesskey, "BellOverload", cfg->bellovl);
    write_setting_i(sesskey, "BellOverloadN", cfg->bellovl_n);
    write_setting_i(sesskey, "BellOverloadT", cfg->bellovl_t
#ifdef PUTTY_UNIX_H
		    * 1000
#endif
		    );
    write_setting_i(sesskey, "BellOverloadS", cfg->bellovl_s
#ifdef PUTTY_UNIX_H
		    * 1000
#endif
		    );
    write_setting_i(sesskey, "ScrollbackLines", cfg->savelines);
    write_setting_i(sesskey, "DECOriginMode", cfg->dec_om);
    write_setting_i(sesskey, "AutoWrapMode", cfg->wrap_mode);
    write_setting_i(sesskey, "LFImpliesCR", cfg->lfhascr);
    write_setting_i(sesskey, "CRImpliesLF", cfg->crhaslf);
    write_setting_i(sesskey, "DisableArabicShaping", cfg->arabicshaping);
    write_setting_i(sesskey, "DisableBidi", cfg->bidi);
    write_setting_i(sesskey, "WinNameAlways", cfg->win_name_always);
    write_setting_s(sesskey, "WinTitle", cfg->wintitle);
    write_setting_i(sesskey, "TermWidth", cfg->width);
    write_setting_i(sesskey, "TermHeight", cfg->height);
    write_setting_fontspec(sesskey, "Font", cfg->font);
    write_setting_i(sesskey, "FontQuality", cfg->font_quality);
    write_setting_i(sesskey, "FontVTMode", cfg->vtmode);
    write_setting_i(sesskey, "UseSystemColours", cfg->system_colour);
    write_setting_i(sesskey, "TryPalette", cfg->try_palette);
    write_setting_i(sesskey, "ANSIColour", cfg->ansi_colour);
    write_setting_i(sesskey, "Xterm256Colour", cfg->xterm_256_colour);
    write_setting_i(sesskey, "BoldAsColour", cfg->bold_colour);

    for (i = 0; i < 22; i++) {
	char buf[20], buf2[30];
	sprintf(buf, "Colour%d", i);
	sprintf(buf2, "%d,%d,%d", cfg->colours[i][0],
		cfg->colours[i][1], cfg->colours[i][2]);
	write_setting_s(sesskey, buf, buf2);
    }
    write_setting_i(sesskey, "RawCNP", cfg->rawcnp);
    write_setting_i(sesskey, "PasteRTF", cfg->rtf_paste);
    write_setting_i(sesskey, "MouseIsXterm", cfg->mouse_is_xterm);
    write_setting_i(sesskey, "RectSelect", cfg->rect_select);
    write_setting_i(sesskey, "MouseOverride", cfg->mouse_override);
    for (i = 0; i < 256; i += 32) {
	char buf[20], buf2[256];
	int j;
	sprintf(buf, "Wordness%d", i);
	*buf2 = '\0';
	for (j = i; j < i + 32; j++) {
	    sprintf(buf2 + strlen(buf2), "%s%d",
		    (*buf2 ? "," : ""), cfg->wordness[j]);
	}
	write_setting_s(sesskey, buf, buf2);
    }
    write_setting_s(sesskey, "LineCodePage", cfg->line_codepage);
    write_setting_i(sesskey, "CJKAmbigWide", cfg->cjk_ambig_wide);
    write_setting_i(sesskey, "UTF8Override", cfg->utf8_override);
    write_setting_s(sesskey, "Printer", cfg->printer);
    write_setting_i(sesskey, "CapsLockCyr", cfg->xlat_capslockcyr);
    write_setting_i(sesskey, "ScrollBar", cfg->scrollbar);
    write_setting_i(sesskey, "ScrollBarFullScreen", cfg->scrollbar_in_fullscreen);
    write_setting_i(sesskey, "ScrollOnKey", cfg->scroll_on_key);
    write_setting_i(sesskey, "ScrollOnDisp", cfg->scroll_on_disp);
    write_setting_i(sesskey, "EraseToScrollback", cfg->erase_to_scrollback);
    write_setting_i(sesskey, "LockSize", cfg->resize_action);
    write_setting_i(sesskey, "BCE", cfg->bce);
    write_setting_i(sesskey, "BlinkText", cfg->blinktext);
    write_setting_i(sesskey, "X11Forward", cfg->x11_forward);
    write_setting_s(sesskey, "X11Display", cfg->x11_display);
    write_setting_i(sesskey, "X11AuthType", cfg->x11_auth);
    write_setting_filename(sesskey, "X11AuthFile", cfg->xauthfile);
    write_setting_i(sesskey, "LocalPortAcceptAll", cfg->lport_acceptall);
    write_setting_i(sesskey, "RemotePortAcceptAll", cfg->rport_acceptall);
    wmap(sesskey, "PortForwardings", cfg->portfwd, lenof(cfg->portfwd));
    write_setting_i(sesskey, "BugIgnore1", 2-cfg->sshbug_ignore1);
    write_setting_i(sesskey, "BugPlainPW1", 2-cfg->sshbug_plainpw1);
    write_setting_i(sesskey, "BugRSA1", 2-cfg->sshbug_rsa1);
    write_setting_i(sesskey, "BugIgnore2", 2-cfg->sshbug_ignore2);
    write_setting_i(sesskey, "BugHMAC2", 2-cfg->sshbug_hmac2);
    write_setting_i(sesskey, "BugDeriveKey2", 2-cfg->sshbug_derivekey2);
    write_setting_i(sesskey, "BugRSAPad2", 2-cfg->sshbug_rsapad2);
    write_setting_i(sesskey, "BugPKSessID2", 2-cfg->sshbug_pksessid2);
    write_setting_i(sesskey, "BugRekey2", 2-cfg->sshbug_rekey2);
    write_setting_i(sesskey, "BugMaxPkt2", 2-cfg->sshbug_maxpkt2);
    write_setting_i(sesskey, "StampUtmp", cfg->stamp_utmp);
    write_setting_i(sesskey, "LoginShell", cfg->login_shell);
    write_setting_i(sesskey, "ScrollbarOnLeft", cfg->scrollbar_on_left);
    write_setting_fontspec(sesskey, "BoldFont", cfg->boldfont);
    write_setting_fontspec(sesskey, "WideFont", cfg->widefont);
    write_setting_fontspec(sesskey, "WideBoldFont", cfg->wideboldfont);
    write_setting_i(sesskey, "ShadowBold", cfg->shadowbold);
    write_setting_i(sesskey, "ShadowBoldOffset", cfg->shadowboldoffset);
    write_setting_s(sesskey, "SerialLine", cfg->serline);
    write_setting_i(sesskey, "SerialSpeed", cfg->serspeed);
    write_setting_i(sesskey, "SerialDataBits", cfg->serdatabits);
    write_setting_i(sesskey, "SerialStopHalfbits", cfg->serstopbits);
    write_setting_i(sesskey, "SerialParity", cfg->serparity);
    write_setting_i(sesskey, "SerialFlowControl", cfg->serflow);
    write_setting_s(sesskey, "WindowClass", cfg->winclass);
}

void load_settings(char *section, Config * cfg)
{
    void *sesskey;

    sesskey = open_settings_r(section);
    load_open_settings(sesskey, cfg);
    close_settings_r(sesskey);

    if (cfg_launchable(cfg))
        add_session_to_jumplist(section);
}

void load_open_settings(void *sesskey, Config *cfg)
{
    int i;
    char prot[10];

    cfg->ssh_subsys = 0;	       /* FIXME: load this properly */
    cfg->remote_cmd_ptr = NULL;
    cfg->remote_cmd_ptr2 = NULL;
    cfg->ssh_nc_host[0] = '\0';

    gpps(sesskey, "HostName", "", cfg->host, sizeof(cfg->host));
    gppfile(sesskey, "LogFileName", &cfg->logfilename);
    gppi(sesskey, "LogType", 0, &cfg->logtype);
    gppi(sesskey, "LogFileClash", LGXF_ASK, &cfg->logxfovr);
    gppi(sesskey, "LogFlush", 1, &cfg->logflush);
    gppi(sesskey, "SSHLogOmitPasswords", 1, &cfg->logomitpass);
    gppi(sesskey, "SSHLogOmitData", 0, &cfg->logomitdata);

    gpps(sesskey, "Protocol", "default", prot, 10);
    cfg->protocol = default_protocol;
    cfg->port = default_port;
    {
	const Backend *b = backend_from_name(prot);
	if (b) {
	    cfg->protocol = b->protocol;
	    gppi(sesskey, "PortNumber", default_port, &cfg->port);
	}
    }

    /* Address family selection */
    gppi(sesskey, "AddressFamily", ADDRTYPE_UNSPEC, &cfg->addressfamily);

    /* The CloseOnExit numbers are arranged in a different order from
     * the standard FORCE_ON / FORCE_OFF / AUTO. */
    gppi(sesskey, "CloseOnExit", 1, &i); cfg->close_on_exit = (i+1)%3;
    gppi(sesskey, "WarnOnClose", 1, &cfg->warn_on_close);
    {
	/* This is two values for backward compatibility with 0.50/0.51 */
	int pingmin, pingsec;
	gppi(sesskey, "PingInterval", 0, &pingmin);
	gppi(sesskey, "PingIntervalSecs", 0, &pingsec);
	cfg->ping_interval = pingmin * 60 + pingsec;
    }
    gppi(sesskey, "TCPNoDelay", 1, &cfg->tcp_nodelay);
    gppi(sesskey, "TCPKeepalives", 0, &cfg->tcp_keepalives);
    gpps(sesskey, "TerminalType", "xterm", cfg->termtype,
	 sizeof(cfg->termtype));
    gpps(sesskey, "TerminalSpeed", "38400,38400", cfg->termspeed,
	 sizeof(cfg->termspeed));
    {
	/* This hardcodes a big set of defaults in any new saved
	 * sessions. Let's hope we don't change our mind. */
	int i;
	char *def = dupstr("");
	/* Default: all set to "auto" */
	for (i = 0; ttymodes[i]; i++) {
	    char *def2 = dupprintf("%s%s=A,", def, ttymodes[i]);
	    sfree(def);
	    def = def2;
	}
	gppmap(sesskey, "TerminalModes", def,
	       cfg->ttymodes, lenof(cfg->ttymodes));
	sfree(def);
    }

    /* proxy settings */
    gpps(sesskey, "ProxyExcludeList", "", cfg->proxy_exclude_list,
	 sizeof(cfg->proxy_exclude_list));
    gppi(sesskey, "ProxyDNS", 1, &i); cfg->proxy_dns = (i+1)%3;
    gppi(sesskey, "ProxyLocalhost", 0, &cfg->even_proxy_localhost);
    gppi(sesskey, "ProxyMethod", -1, &cfg->proxy_type);
    if (cfg->proxy_type == -1) {
        int i;
        gppi(sesskey, "ProxyType", 0, &i);
        if (i == 0)
            cfg->proxy_type = PROXY_NONE;
        else if (i == 1)
            cfg->proxy_type = PROXY_HTTP;
        else if (i == 3)
            cfg->proxy_type = PROXY_TELNET;
        else if (i == 4)
            cfg->proxy_type = PROXY_CMD;
        else {
            gppi(sesskey, "ProxySOCKSVersion", 5, &i);
            if (i == 5)
                cfg->proxy_type = PROXY_SOCKS5;
            else
                cfg->proxy_type = PROXY_SOCKS4;
        }
    }
    gpps(sesskey, "ProxyHost", "proxy", cfg->proxy_host,
	 sizeof(cfg->proxy_host));
    gppi(sesskey, "ProxyPort", 80, &cfg->proxy_port);
    gpps(sesskey, "ProxyUsername", "", cfg->proxy_username,
	 sizeof(cfg->proxy_username));
    gpps(sesskey, "ProxyPassword", "", cfg->proxy_password,
	 sizeof(cfg->proxy_password));
    gpps(sesskey, "ProxyTelnetCommand", "connect %host %port\\n",
	 cfg->proxy_telnet_command, sizeof(cfg->proxy_telnet_command));
    gppmap(sesskey, "Environment", "", cfg->environmt, lenof(cfg->environmt));
    gpps(sesskey, "UserName", "", cfg->username, sizeof(cfg->username));
    gppi(sesskey, "UserNameFromEnvironment", 0, &cfg->username_from_env);
    gpps(sesskey, "LocalUserName", "", cfg->localusername,
	 sizeof(cfg->localusername));
    gppi(sesskey, "NoPTY", 0, &cfg->nopty);
    gppi(sesskey, "Compression", 0, &cfg->compression);
    gppi(sesskey, "TryAgent", 1, &cfg->tryagent);
    gppi(sesskey, "AgentFwd", 0, &cfg->agentfwd);
    gppi(sesskey, "ChangeUsername", 0, &cfg->change_username);
    gppi(sesskey, "GssapiFwd", 0, &cfg->gssapifwd);
    gprefs(sesskey, "Cipher", "\0",
	   ciphernames, CIPHER_MAX, cfg->ssh_cipherlist);
    {
	/* Backward-compatibility: we used to have an option to
	 * disable gex under the "bugs" panel after one report of
	 * a server which offered it then choked, but we never got
	 * a server version string or any other reports. */
	char *default_kexes;
	gppi(sesskey, "BugDHGEx2", 0, &i); i = 2-i;
	if (i == FORCE_ON)
	    default_kexes = "dh-group14-sha1,dh-group1-sha1,rsa,WARN,dh-gex-sha1";
	else
	    default_kexes = "dh-gex-sha1,dh-group14-sha1,dh-group1-sha1,rsa,WARN";
	gprefs(sesskey, "KEX", default_kexes,
	       kexnames, KEX_MAX, cfg->ssh_kexlist);
    }
    gppi(sesskey, "RekeyTime", 60, &cfg->ssh_rekey_time);
    gpps(sesskey, "RekeyBytes", "1G", cfg->ssh_rekey_data,
	 sizeof(cfg->ssh_rekey_data));
    gppi(sesskey, "SshProt", 2, &cfg->sshprot);
    gpps(sesskey, "LogHost", "", cfg->loghost, sizeof(cfg->loghost));
    gppi(sesskey, "SSH2DES", 0, &cfg->ssh2_des_cbc);
    gppi(sesskey, "SshNoAuth", 0, &cfg->ssh_no_userauth);
    gppi(sesskey, "SshBanner", 1, &cfg->ssh_show_banner);
    gppi(sesskey, "AuthTIS", 0, &cfg->try_tis_auth);
    gppi(sesskey, "AuthKI", 1, &cfg->try_ki_auth);
    gppi(sesskey, "AuthGSSAPI", 1, &cfg->try_gssapi_auth);
#ifndef NO_GSSAPI
    gprefs(sesskey, "GSSLibs", "\0",
	   gsslibkeywords, ngsslibs, cfg->ssh_gsslist);
    gppfile(sesskey, "GSSCustom", &cfg->ssh_gss_custom);
#endif
    gppi(sesskey, "SshNoShell", 0, &cfg->ssh_no_shell);
    gppfile(sesskey, "PublicKeyFile", &cfg->keyfile);
    gpps(sesskey, "RemoteCommand", "", cfg->remote_cmd,
	 sizeof(cfg->remote_cmd));
    gppi(sesskey, "RFCEnviron", 0, &cfg->rfc_environ);
    gppi(sesskey, "PassiveTelnet", 0, &cfg->passive_telnet);
    gppi(sesskey, "BackspaceIsDelete", 1, &cfg->bksp_is_delete);
    gppi(sesskey, "RXVTHomeEnd", 0, &cfg->rxvt_homeend);
    gppi(sesskey, "LinuxFunctionKeys", 0, &cfg->funky_type);
    gppi(sesskey, "NoApplicationKeys", 0, &cfg->no_applic_k);
    gppi(sesskey, "NoApplicationCursors", 0, &cfg->no_applic_c);
    gppi(sesskey, "NoMouseReporting", 0, &cfg->no_mouse_rep);
    gppi(sesskey, "NoRemoteResize", 0, &cfg->no_remote_resize);
    gppi(sesskey, "NoAltScreen", 0, &cfg->no_alt_screen);
    gppi(sesskey, "NoRemoteWinTitle", 0, &cfg->no_remote_wintitle);
    {
	/* Backward compatibility */
	int no_remote_qtitle;
	gppi(sesskey, "NoRemoteQTitle", 1, &no_remote_qtitle);
	/* We deliberately interpret the old setting of "no response" as
	 * "empty string". This changes the behaviour, but hopefully for
	 * the better; the user can always recover the old behaviour. */
	gppi(sesskey, "RemoteQTitleAction",
	     no_remote_qtitle ? TITLE_EMPTY : TITLE_REAL,
	     &cfg->remote_qtitle_action);
    }
    gppi(sesskey, "NoDBackspace", 0, &cfg->no_dbackspace);
    gppi(sesskey, "NoRemoteCharset", 0, &cfg->no_remote_charset);
    gppi(sesskey, "ApplicationCursorKeys", 0, &cfg->app_cursor);
    gppi(sesskey, "ApplicationKeypad", 0, &cfg->app_keypad);
    gppi(sesskey, "NetHackKeypad", 0, &cfg->nethack_keypad);
    gppi(sesskey, "AltF4", 1, &cfg->alt_f4);
    gppi(sesskey, "AltSpace", 0, &cfg->alt_space);
    gppi(sesskey, "AltOnly", 0, &cfg->alt_only);
    gppi(sesskey, "ComposeKey", 0, &cfg->compose_key);
    gppi(sesskey, "CtrlAltKeys", 1, &cfg->ctrlaltkeys);
    gppi(sesskey, "TelnetKey", 0, &cfg->telnet_keyboard);
    gppi(sesskey, "TelnetRet", 1, &cfg->telnet_newline);
    gppi(sesskey, "LocalEcho", AUTO, &cfg->localecho);
    gppi(sesskey, "LocalEdit", AUTO, &cfg->localedit);
    gpps(sesskey, "Answerback", "PuTTY", cfg->answerback,
	 sizeof(cfg->answerback));
    gppi(sesskey, "AlwaysOnTop", 0, &cfg->alwaysontop);
    gppi(sesskey, "FullScreenOnAltEnter", 0, &cfg->fullscreenonaltenter);
    gppi(sesskey, "HideMousePtr", 0, &cfg->hide_mouseptr);
    gppi(sesskey, "SunkenEdge", 0, &cfg->sunken_edge);
    gppi(sesskey, "WindowBorder", 1, &cfg->window_border);
    gppi(sesskey, "CurType", 0, &cfg->cursor_type);
    gppi(sesskey, "BlinkCur", 0, &cfg->blink_cur);
    /* pedantic compiler tells me I can't use &cfg->beep as an int * :-) */
    gppi(sesskey, "Beep", 1, &cfg->beep);
    gppi(sesskey, "BeepInd", 0, &cfg->beep_ind);
    gppfile(sesskey, "BellWaveFile", &cfg->bell_wavefile);
    gppi(sesskey, "BellOverload", 1, &cfg->bellovl);
    gppi(sesskey, "BellOverloadN", 5, &cfg->bellovl_n);
    gppi(sesskey, "BellOverloadT", 2*TICKSPERSEC
#ifdef PUTTY_UNIX_H
				   *1000
#endif
				   , &i);
    cfg->bellovl_t = i
#ifdef PUTTY_UNIX_H
		    / 1000
#endif
	;
    gppi(sesskey, "BellOverloadS", 5*TICKSPERSEC
#ifdef PUTTY_UNIX_H
				   *1000
#endif
				   , &i);
    cfg->bellovl_s = i
#ifdef PUTTY_UNIX_H
		    / 1000
#endif
	;
    gppi(sesskey, "ScrollbackLines", 200, &cfg->savelines);
    gppi(sesskey, "DECOriginMode", 0, &cfg->dec_om);
    gppi(sesskey, "AutoWrapMode", 1, &cfg->wrap_mode);
    gppi(sesskey, "LFImpliesCR", 0, &cfg->lfhascr);
    gppi(sesskey, "CRImpliesLF", 0, &cfg->crhaslf);
    gppi(sesskey, "DisableArabicShaping", 0, &cfg->arabicshaping);
    gppi(sesskey, "DisableBidi", 0, &cfg->bidi);
    gppi(sesskey, "WinNameAlways", 1, &cfg->win_name_always);
    gpps(sesskey, "WinTitle", "", cfg->wintitle, sizeof(cfg->wintitle));
    gppi(sesskey, "TermWidth", 80, &cfg->width);
    gppi(sesskey, "TermHeight", 24, &cfg->height);
    gppfont(sesskey, "Font", &cfg->font);
    gppi(sesskey, "FontQuality", FQ_DEFAULT, &cfg->font_quality);
    gppi(sesskey, "FontVTMode", VT_UNICODE, (int *) &cfg->vtmode);
    gppi(sesskey, "UseSystemColours", 0, &cfg->system_colour);
    gppi(sesskey, "TryPalette", 0, &cfg->try_palette);
    gppi(sesskey, "ANSIColour", 1, &cfg->ansi_colour);
    gppi(sesskey, "Xterm256Colour", 1, &cfg->xterm_256_colour);
    gppi(sesskey, "BoldAsColour", 1, &cfg->bold_colour);

    for (i = 0; i < 22; i++) {
	static const char *const defaults[] = {
	    "187,187,187", "255,255,255", "0,0,0", "85,85,85", "0,0,0",
	    "0,255,0", "0,0,0", "85,85,85", "187,0,0", "255,85,85",
	    "0,187,0", "85,255,85", "187,187,0", "255,255,85", "0,0,187",
	    "85,85,255", "187,0,187", "255,85,255", "0,187,187",
	    "85,255,255", "187,187,187", "255,255,255"
	};
	char buf[20], buf2[30];
	int c0, c1, c2;
	sprintf(buf, "Colour%d", i);
	gpps(sesskey, buf, defaults[i], buf2, sizeof(buf2));
	if (sscanf(buf2, "%d,%d,%d", &c0, &c1, &c2) == 3) {
	    cfg->colours[i][0] = c0;
	    cfg->colours[i][1] = c1;
	    cfg->colours[i][2] = c2;
	}
    }
    gppi(sesskey, "RawCNP", 0, &cfg->rawcnp);
    gppi(sesskey, "PasteRTF", 0, &cfg->rtf_paste);
    gppi(sesskey, "MouseIsXterm", 0, &cfg->mouse_is_xterm);
    gppi(sesskey, "RectSelect", 0, &cfg->rect_select);
    gppi(sesskey, "MouseOverride", 1, &cfg->mouse_override);
    for (i = 0; i < 256; i += 32) {
	static const char *const defaults[] = {
	    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
	    "0,1,2,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1",
	    "1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2",
	    "1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1",
	    "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
	    "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
	    "2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2",
	    "2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2"
	};
	char buf[20], buf2[256], *p;
	int j;
	sprintf(buf, "Wordness%d", i);
	gpps(sesskey, buf, defaults[i / 32], buf2, sizeof(buf2));
	p = buf2;
	for (j = i; j < i + 32; j++) {
	    char *q = p;
	    while (*p && *p != ',')
		p++;
	    if (*p == ',')
		*p++ = '\0';
	    cfg->wordness[j] = atoi(q);
	}
    }
    /*
     * The empty default for LineCodePage will be converted later
     * into a plausible default for the locale.
     */
    gpps(sesskey, "LineCodePage", "", cfg->line_codepage,
	 sizeof(cfg->line_codepage));
    gppi(sesskey, "CJKAmbigWide", 0, &cfg->cjk_ambig_wide);
    gppi(sesskey, "UTF8Override", 1, &cfg->utf8_override);
    gpps(sesskey, "Printer", "", cfg->printer, sizeof(cfg->printer));
    gppi (sesskey, "CapsLockCyr", 0, &cfg->xlat_capslockcyr);
    gppi(sesskey, "ScrollBar", 1, &cfg->scrollbar);
    gppi(sesskey, "ScrollBarFullScreen", 0, &cfg->scrollbar_in_fullscreen);
    gppi(sesskey, "ScrollOnKey", 0, &cfg->scroll_on_key);
    gppi(sesskey, "ScrollOnDisp", 1, &cfg->scroll_on_disp);
    gppi(sesskey, "EraseToScrollback", 1, &cfg->erase_to_scrollback);
    gppi(sesskey, "LockSize", 0, &cfg->resize_action);
    gppi(sesskey, "BCE", 1, &cfg->bce);
    gppi(sesskey, "BlinkText", 0, &cfg->blinktext);
    gppi(sesskey, "X11Forward", 0, &cfg->x11_forward);
    gpps(sesskey, "X11Display", "", cfg->x11_display,
	 sizeof(cfg->x11_display));
    gppi(sesskey, "X11AuthType", X11_MIT, &cfg->x11_auth);
    gppfile(sesskey, "X11AuthFile", &cfg->xauthfile);

    gppi(sesskey, "LocalPortAcceptAll", 0, &cfg->lport_acceptall);
    gppi(sesskey, "RemotePortAcceptAll", 0, &cfg->rport_acceptall);
    gppmap(sesskey, "PortForwardings", "", cfg->portfwd, lenof(cfg->portfwd));
    gppi(sesskey, "BugIgnore1", 0, &i); cfg->sshbug_ignore1 = 2-i;
    gppi(sesskey, "BugPlainPW1", 0, &i); cfg->sshbug_plainpw1 = 2-i;
    gppi(sesskey, "BugRSA1", 0, &i); cfg->sshbug_rsa1 = 2-i;
    gppi(sesskey, "BugIgnore2", 0, &i); cfg->sshbug_ignore2 = 2-i;
    {
	int i;
	gppi(sesskey, "BugHMAC2", 0, &i); cfg->sshbug_hmac2 = 2-i;
	if (cfg->sshbug_hmac2 == AUTO) {
	    gppi(sesskey, "BuggyMAC", 0, &i);
	    if (i == 1)
		cfg->sshbug_hmac2 = FORCE_ON;
	}
    }
    gppi(sesskey, "BugDeriveKey2", 0, &i); cfg->sshbug_derivekey2 = 2-i;
    gppi(sesskey, "BugRSAPad2", 0, &i); cfg->sshbug_rsapad2 = 2-i;
    gppi(sesskey, "BugPKSessID2", 0, &i); cfg->sshbug_pksessid2 = 2-i;
    gppi(sesskey, "BugRekey2", 0, &i); cfg->sshbug_rekey2 = 2-i;
    gppi(sesskey, "BugMaxPkt2", 0, &i); cfg->sshbug_maxpkt2 = 2-i;
    cfg->ssh_simple = FALSE;
    gppi(sesskey, "StampUtmp", 1, &cfg->stamp_utmp);
    gppi(sesskey, "LoginShell", 1, &cfg->login_shell);
    gppi(sesskey, "ScrollbarOnLeft", 0, &cfg->scrollbar_on_left);
    gppi(sesskey, "ShadowBold", 0, &cfg->shadowbold);
    gppfont(sesskey, "BoldFont", &cfg->boldfont);
    gppfont(sesskey, "WideFont", &cfg->widefont);
    gppfont(sesskey, "WideBoldFont", &cfg->wideboldfont);
    gppi(sesskey, "ShadowBoldOffset", 1, &cfg->shadowboldoffset);
    gpps(sesskey, "SerialLine", "", cfg->serline, sizeof(cfg->serline));
    gppi(sesskey, "SerialSpeed", 9600, &cfg->serspeed);
    gppi(sesskey, "SerialDataBits", 8, &cfg->serdatabits);
    gppi(sesskey, "SerialStopHalfbits", 2, &cfg->serstopbits);
    gppi(sesskey, "SerialParity", SER_PAR_NONE, &cfg->serparity);
    gppi(sesskey, "SerialFlowControl", SER_FLOW_XONXOFF, &cfg->serflow);
    gpps(sesskey, "WindowClass", "", cfg->winclass, sizeof(cfg->winclass));
}

void do_defaults(char *session, Config * cfg)
{
    load_settings(session, cfg);
}

static int sessioncmp(const void *av, const void *bv)
{
    const char *a = *(const char *const *) av;
    const char *b = *(const char *const *) bv;

    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(a, "Default Settings"))
	return -1;		       /* a comes first */
    if (!strcmp(b, "Default Settings"))
	return +1;		       /* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(a, b);	       /* otherwise, compare normally */
}

void get_sesslist(struct sesslist *list, int allocate)
{
    char otherbuf[2048];
    int buflen, bufsize, i;
    char *p, *ret;
    void *handle;

    if (allocate) {

	buflen = bufsize = 0;
	list->buffer = NULL;
	if ((handle = enum_settings_start()) != NULL) {
	    do {
		ret = enum_settings_next(handle, otherbuf, sizeof(otherbuf));
		if (ret) {
		    int len = strlen(otherbuf) + 1;
		    if (bufsize < buflen + len) {
			bufsize = buflen + len + 2048;
			list->buffer = sresize(list->buffer, bufsize, char);
		    }
		    strcpy(list->buffer + buflen, otherbuf);
		    buflen += strlen(list->buffer + buflen) + 1;
		}
	    } while (ret);
	    enum_settings_finish(handle);
	}
	list->buffer = sresize(list->buffer, buflen + 1, char);
	list->buffer[buflen] = '\0';

	/*
	 * Now set up the list of sessions. Note that "Default
	 * Settings" must always be claimed to exist, even if it
	 * doesn't really.
	 */

	p = list->buffer;
	list->nsessions = 1;	       /* "Default Settings" counts as one */
	while (*p) {
	    if (strcmp(p, "Default Settings"))
		list->nsessions++;
	    while (*p)
		p++;
	    p++;
	}

	list->sessions = snewn(list->nsessions + 1, char *);
	list->sessions[0] = "Default Settings";
	p = list->buffer;
	i = 1;
	while (*p) {
	    if (strcmp(p, "Default Settings"))
		list->sessions[i++] = p;
	    while (*p)
		p++;
	    p++;
	}

	qsort(list->sessions, i, sizeof(char *), sessioncmp);
    } else {
	sfree(list->buffer);
	sfree(list->sessions);
	list->buffer = NULL;
	list->sessions = NULL;
    }
}
