// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2014, 2019 Sven Strickroth <email@cs-ware.de>
// Copyright (C) VLC project (http://videolan.org)

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include <stdint.h>
#include <WinCrypt.h>
#include "UpdateDownloader.h"

enum    /* Public key algorithms */
{
	PUBLIC_KEY_ALGO_RSA = 0x01,
	PUBLIC_KEY_ALGO_DSA = 0x11
};

enum    /* Digest algorithms */
{
	DIGEST_ALGO_SHA1    = 0x02,
	DIGEST_ALGO_SHA256  = 0x08,
	DIGEST_ALGO_SHA384  = 0x09,
	DIGEST_ALGO_SHA512  = 0x0A,
};

enum    /* Packet types */
{
	SIGNATURE_PACKET    = 0x02,
	PUBLIC_KEY_PACKET   = 0x06,
	USER_ID_PACKET      = 0x0d
};

enum    /* Signature types */
{
	BINARY_SIGNATURE        = 0x00,
	TEXT_SIGNATURE          = 0x01,

	/* Public keys signatures */
	GENERIC_KEY_SIGNATURE   = 0x10, /* No assumption of verification */
	PERSONA_KEY_SIGNATURE   = 0x11, /* No verification has been made */
	CASUAL_KEY_SIGNATURE    = 0x12, /* Some casual verification */
	POSITIVE_KEY_SIGNATURE  = 0x13  /* Substantial verification */
};

enum    /* Signature subpacket types */
{
	ISSUER_SUBPACKET    = 0x10
};

struct public_key_packet_t
{
	uint8_t version;      /* we use only version 4 */
	uint8_t timestamp[4]; /* creation time of the key */
	uint8_t algo;         /* we only use DSA or RSA */
	/* the multi precision integers, with their 2 bytes length header */
	union {
#ifdef TGIT_UPDATECRYPTO_DSA
		struct {
			uint8_t p[2 + 128];
			uint8_t q[2 + 20];
			uint8_t g[2 + 128];
			uint8_t y[2 + 128];
		} dsa;
#endif
		struct {
			uint8_t n[2 + 4096 / 8];
			uint8_t e[2 + 4096 / 8];
		} rsa;
	} sig;
};

/* used for public key and file signatures */
struct signature_packet_t
{
	uint8_t version; /* 3 or 4 */

	uint8_t type;
	uint8_t public_key_algo;    /* DSA or RSA */
	uint8_t digest_algo;

	uint8_t hash_verification[2];
	uint8_t issuer_longid[8];

	union   /* version specific data */
	{
		struct
		{
			uint8_t hashed_data_len[2];     /* scalar number */
			uint8_t *hashed_data;           /* hashed_data_len bytes */
			uint8_t unhashed_data_len[2];   /* scalar number */
			uint8_t *unhashed_data;         /* unhashed_data_len bytes */
		} v4;
	} specific;

/* The part below is made of consecutive MPIs, their number and size being
 * public-key-algorithm dependent.
 */
	union {
#ifdef TGIT_UPDATECRYPTO_DSA
		struct {
			uint8_t r[2 + 20];
			uint8_t s[2 + 20];
		} dsa;
#endif
		struct {
			uint8_t s[2 + 4096 / 8];
		} rsa;
	} algo_specific;
};

typedef struct public_key_packet_t public_key_packet_t;
typedef struct signature_packet_t signature_packet_t;

struct public_key_t
{
	uint8_t longid[8];       /* Long id */
	uint8_t *psz_username;    /* USER ID */

	public_key_packet_t key;       /* Public key packet */

	signature_packet_t sig;     /* Signature packet, by the embedded key */
};

typedef struct public_key_t public_key_t;

#ifdef TGIT_UPDATECRYPTO_DSA
typedef struct _DSAKEY
{
  BLOBHEADER blobheader;
  DSSPUBKEY_VER3 dsspubkeyver3;
  BYTE p[128]; // prime modulus
  BYTE q[20]; // large factor of P-1
  BYTE g[128]; // the generator parameter
  BYTE y[128]; // (G^X) mod P
} DSAKEY;
#endif

typedef struct _RSAKEY
{
	BLOBHEADER blobheader;
	RSAPUBKEY rsapubkey;
	BYTE n[4096 / 8];
} RSAKEY;

int VerifyIntegrity(const CString &filename, const CString &signatureFilename, CUpdateDownloader *updateDownloader);
