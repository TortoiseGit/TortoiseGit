#define FILEVER				1,8,7,0
#define PRODUCTVER			FILEVER
#define STRFILEVER			"1.8.7.0"
#define STRPRODUCTVER		STRFILEVER

#define TGIT_VERMAJOR		1
#define TGIT_VERMINOR		8
#define TGIT_VERMICRO		7
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
 * TortoiseGit DSA PGP Public Key used to sign releases
 *****************************************************************************/

/* We trust this public key, and by extension, also keys signed by it. */

/* NOTE:
 * We need a 1024 bits DSA key.
 * Don't forget to upload the key to http://download.tortoisegit.org/keys/
 */

#ifdef NEED_SIGNING_KEY
/*
 * TortoiseGit Release Signing Key
 */
static const uint8_t tortoisegit_public_key_longid[8] = {
	0x33, 0xF7, 0x5D, 0xCF, 0x2B, 0xC0, 0xD3, 0x62
};

/* gpg --export --armor "<id>"|sed -e s/^/\"/ -e s/\$/\\\\n\"/ */
static const uint8_t tortoisegit_public_key[] = {
	"-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
	"Version: GnuPG v1.4.11\n"
	"\n"
	"mQGiBFH80rsRBACHXxk2UUSeEIwyV0lKDBX87tKbeSyV5trqyTI6tlyC3F2+ueEd\n"
	"D16mZR54wc5VQ1O0uIqBNRF78qJkjjZWUQt2VcPBWD9HiFIzjPGkSy+wk3EbvF/N\n"
	"VAVLVcVMhBq6bBXVryQkwKUsbrXbdMVzSdOfHJvRjuZ12Z3cwa45OxpeuwCggc8l\n"
	"L93mhV93sw6+Nz+MPO2qBi0D/RFW3rRqp8IoDNSOVhesP7fP9WXnkaSM7eTMx/Wd\n"
	"hXpbeQVCSXiJhVQvKHw5hHuqNYZ54o0PEfgfm2i4+iEpflaa5sAzE+Pvy8VdnR3i\n"
	"7sXDtC0s52s2nkNdIcv33Y4QUGNGPjafTLqvR/+DLXq+VI3/L5Goqy78mpn5cOu/\n"
	"rYoQA/9hAv85QSL+XmxejGzw8KYdLqJixtKUob68kbuy+KG7fpOBMM30m3YkDgI6\n"
	"moGN6JJGEpiGl4Z7coyhOIQo/WSBxpGx+OInozjlV4Xk8T5AW1pG3vP4W3KwXfoo\n"
	"mdM16UMqzrLT+WEAdYKR+dHYsN6ttmlDVExgfF9c9SqoKDfU/7QfVG9ydG9pc2VH\n"
	"aXQgUmVsZWFzZSBTaWduaW5nIEtleYhiBBMRAgAiBQJR/NK7AhsjBgsJCAcDAgYV\n"
	"CAIJCgsEFgIDAQIeAQIXgAAKCRAz913PK8DTYm/YAJ4zehwjlvMsDt5va0HPB0HF\n"
	"FxTBcACfa7gXDOuEKwNux07K0cYOz9bM+n6IRgQQEQIABgUCUfzS8wAKCRAWRmek\n"
	"9anUxKqKAKDI3cqd73jLAZ2AjZHR2yyLqxnJLQCgvZ+/VGgmyINSKdpHmKq1qk42\n"
	"ttw=\n"
	"=L+nv\n"
	"-----END PGP PUBLIC KEY BLOCK-----\n"
};
#endif
