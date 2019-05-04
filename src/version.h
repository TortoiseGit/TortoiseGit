// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

#define FILEVER				2,8,0,0
#define PRODUCTVER			FILEVER
#define STRFILEVER			"2.8.0.0"
#define STRPRODUCTVER		STRFILEVER

#define TGIT_VERMAJOR		2
#define TGIT_VERMINOR		8
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
	"\n"
	"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\n"
	"wnjOFQ4oKIiU22XYajlcJ6s=\n"
	"=sjNE\n"
	"-----END PGP PUBLIC KEY BLOCK-----\n"
};
#endif
