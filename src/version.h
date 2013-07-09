#define FILEVER        1,8,4,0
#define PRODUCTVER     FILEVER
#define STRFILEVER     "1.8.4.0"
#define STRPRODUCTVER  STRFILEVER

#define TGIT_VERMAJOR             1
#define TGIT_VERMINOR             8
#define TGIT_VERMICRO             4
#define TGIT_VERBUILD			  0
#define TGIT_VERDATE			  __DATE__

#ifdef _WIN64
#define TGIT_PLATFORM		"64 Bit"
#else
#define TGIT_PLATFORM		"32 Bit"
#endif

#define PREVIEW		0

/*
 * TortoiseGit crash handler
 * Enabling this causes the crash handler to upload stack traces to crash-server.com
 * to the TortoiseGit account. Enabling does not make sense if the TortoiseGit team
 * does not have access to the debug symbols!
 *
 * This only makes sense for official (preview) releases of the TortoiseGit team
 */
#define ENABLE_CRASHHANLDER	0
