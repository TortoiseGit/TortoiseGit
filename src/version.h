// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2018 - TortoiseGit

#define FILEVER				2,7,0,0
#define PRODUCTVER			FILEVER
#define STRFILEVER			"2.7.0.0"
#define STRPRODUCTVER		STRFILEVER

#define TGIT_VERMAJOR		2
#define TGIT_VERMINOR		7
#define TGIT_VERMICRO		0
#define TGIT_VERBUILD		0
#define TGIT_VERDATE		__DATE__

#ifdef _WIN64
#define TGIT_PLATFORM		"64 Bit"
#else
#define TGIT_PLATFORM		"32 Bit"
#endif

#define PREVIEW				0

/*
 * TortoiseGit crash handler
 * Enabling this causes the crash handler to upload stack traces to crash-server.com
 * to the TortoiseGit account. Enabling does not make sense if the TortoiseGit team
 * does not have access to the debug symbols!
 *
 * This only makes sense for official (preview) releases of the TortoiseGit team
 */
#define ENABLE_CRASHHANLDER	0

/*****************************************************************************
 * TortoiseGit PGP Public Key used to sign releases
 *****************************************************************************/

/* We trust this public key, and by extension, also keys signed by it. */

/* NOTE:
 * Don't forget to upload the key to http://download.tortoisegit.org/keys/
 */

#ifdef NEED_SIGNING_KEY
/*
 * TortoiseGit Release Signing Key
 */
static const uint8_t tortoisegit_public_key_longid[8] = {
	0xF7, 0xF1, 0x7B, 0x3F, 0x9D, 0xD9, 0x53, 0x9E
};

/* gpg --export --armor "<id>"|sed -e s/^/\"/ -e s/\$/\\\\n\"/ */
static const uint8_t tortoisegit_public_key[] = {
	"-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
	"Version: GnuPG v1\n"
	"\n"
	"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\n"
	"=SfFT\n"
	"-----END PGP PUBLIC KEY BLOCK-----\n"
};
#endif
