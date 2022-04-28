/*
 * winproxy.c: Windows implementation of platform_new_connection(),
 * supporting an OpenSSH-like proxy command via the winhandl.c
 * mechanism.
 */

#include <stdio.h>
#include <assert.h>

#include "tree234.h"
#include "putty.h"
#include "network.h"
#include "proxy.h"

Socket *platform_new_connection(SockAddr *addr, const char *hostname,
                                int port, bool privport,
                                bool oobinline, bool nodelay, bool keepalive,
                                Plug *plug, Conf *conf)
{
    char *cmd;
    HANDLE us_to_cmd, cmd_from_us;
    HANDLE us_from_cmd, cmd_to_us;
    HANDLE us_from_cmd_err, cmd_err_to_us;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    if (conf_get_int(conf, CONF_proxy_type) != PROXY_CMD)
        return NULL;

    cmd = format_telnet_command(addr, port, conf);

    /* We are responsible for this and don't need it any more */
    sk_addr_free(addr);

    {
        char *msg = dupprintf("Starting local proxy command: %s", cmd);
        plug_log(plug, PLUGLOG_PROXY_MSG, NULL, 0, msg, 0);
        sfree(msg);
    }

    /*
     * Create the pipes to the proxy command, and spawn the proxy
     * command process.
     */
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;    /* default */
    sa.bInheritHandle = true;
    if (!CreatePipe(&us_from_cmd, &cmd_to_us, &sa, 0)) {
        sfree(cmd);
        return new_error_socket_fmt(
            plug, "Unable to create pipes for proxy command: %s",
            win_strerror(GetLastError()));
    }

    if (!CreatePipe(&cmd_from_us, &us_to_cmd, &sa, 0)) {
        sfree(cmd);
        CloseHandle(us_from_cmd);
        CloseHandle(cmd_to_us);
        return new_error_socket_fmt(
            plug, "Unable to create pipes for proxy command: %s",
            win_strerror(GetLastError()));
    }

    if (!CreatePipe(&us_from_cmd_err, &cmd_err_to_us, &sa, 0)) {
        sfree(cmd);
        CloseHandle(us_from_cmd);
        CloseHandle(cmd_to_us);
        CloseHandle(us_to_cmd);
        CloseHandle(cmd_from_us);
        return new_error_socket_fmt(
            plug, "Unable to create pipes for proxy command: %s",
            win_strerror(GetLastError()));
    }

    SetHandleInformation(us_to_cmd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(us_from_cmd, HANDLE_FLAG_INHERIT, 0);
    if (us_from_cmd_err != NULL)
        SetHandleInformation(us_from_cmd_err, HANDLE_FLAG_INHERIT, 0);

    si.cb = sizeof(si);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;
    si.hStdInput = cmd_from_us;
    si.hStdOutput = cmd_to_us;
    si.hStdError = cmd_err_to_us;
    CreateProcess(NULL, cmd, NULL, NULL, true,
                  CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS,
                  NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    sfree(cmd);

    CloseHandle(cmd_from_us);
    CloseHandle(cmd_to_us);

    if (cmd_err_to_us != NULL)
        CloseHandle(cmd_err_to_us);

    return make_handle_socket(us_to_cmd, us_from_cmd, us_from_cmd_err,
                              plug, false);
}

const char * proxy_reg_path = "Software\\TortoiseGit\\Git\\Servers\\global";

int conf_tgit_proxy(Conf * conf)
{
    int rc = -1;
    HKEY key = HKEY_CURRENT_USER;

    DWORD proxy_enabled = registry_get_dword(key, proxy_reg_path, "http-proxy-enable", 0);
    if (proxy_enabled == 0)
        return 0;

    char * proxy_ignore = registry_get_string(key, proxy_reg_path, "http-proxy-ignore");
    char * proxy_type = registry_get_string(key, proxy_reg_path, "http-proxy-type");
    char * proxy_host = registry_get_string(key, proxy_reg_path, "http-proxy-host");
    char * proxy_port = registry_get_string(key, proxy_reg_path, "http-proxy-port");
    char * proxy_username = registry_get_string(key, proxy_reg_path, "http-proxy-username");
    char * proxy_password = registry_get_string(key, proxy_reg_path, "http-proxy-password");

    if (!proxy_host || proxy_host[0] == 0)
        goto out;
    if (!proxy_port || proxy_port[0] == 0)
        goto out;

    conf_set_str(conf, CONF_proxy_host, proxy_host);
    conf_set_int(conf, CONF_proxy_port, atoi(proxy_port));

    if (proxy_ignore && proxy_ignore[0])
        conf_set_str(conf, CONF_proxy_exclude_list, proxy_ignore);

    conf_set_int(conf, CONF_proxy_dns, AUTO);
    conf_set_int(conf, CONF_proxy_type, PROXY_HTTP);

    if (proxy_type && proxy_type[0]) {
        if (stricmp(proxy_type, "socks") == 0) {
            conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS4);
        }
        else if (stricmp(proxy_type, "socks4") == 0) {
            conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS4);
        }
        else if (stricmp(proxy_type, "socks4a") == 0) {
            conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS4);
        }
        else if (stricmp(proxy_type, "socks5") == 0) {
            conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS5);
        }
        else if (stricmp(proxy_type, "socks5h") == 0) {
            conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS5);
            conf_set_int(conf, CONF_proxy_dns, FORCE_ON);
        }
    }

    if (proxy_username && proxy_username[0]) {
        conf_set_str(conf, CONF_proxy_username, proxy_username);
        if (proxy_password && proxy_password[0]) {
            conf_set_str(conf, CONF_proxy_password, proxy_password);
        }
    }

    rc = 1;

out:
    sfree(proxy_ignore);
    sfree(proxy_type);
    sfree(proxy_host);
    sfree(proxy_port);
    sfree(proxy_username);
    sfree(proxy_password);
    return rc;
}
