// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2014, 2019, 2026 Sven Strickroth <email@cs-ware.de>
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
#include <optional>

inline constexpr size_t MAX_MPI_BITS_RSA = 4096;

enum    /* Public key algorithms */
{
	PUBLIC_KEY_ALGO_RSA = 0x01,
};

enum    /* Digest algorithms */
{
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

struct mpi_t
{
	uint8_t len[2]; /* scalar number */
	std::vector<uint8_t> number;

	[[nodiscard]] constexpr boolean empty() const { return number.empty(); }
	[[nodiscard]] constexpr size_t size() const { return number.size(); }
	[[nodiscard]] constexpr auto data() const { return number.data(); }
};

struct public_key_packet_t
{
	uint8_t version;      /* we use only version 4 */
	uint8_t timestamp[4]; /* creation time of the key */
	uint8_t algo; /* we only use RSA */
	/* the multi precision integers, with their 2 bytes length header */
	struct
	{
		struct {
			mpi_t n;
			mpi_t e;
		} rsa;
	} sig;
};

/* used for public key and file signatures */
struct signature_packet_t
{
	uint8_t version; /* only 4 */

	uint8_t type;
	uint8_t public_key_algo; /* only RSA is supported */
	uint8_t digest_algo;

	uint8_t hash_verification[2];
	uint8_t issuer_longid[8];

	struct /* version specific data */
	{
		struct
		{
			uint8_t hashed_data_len[2];     /* scalar number */
			std::vector<uint8_t> hashed_data; /* hashed_data_len bytes */
		} v4;
	} specific;

/* The part below is made of consecutive MPIs, their number and size being
 * public-key-algorithm dependent.
 */
	struct
	{
		struct {
			mpi_t s;
		} rsa;
	} algo_specific;
};


struct public_key_t
{
	uint8_t longid[8];       /* Long id */
	std::optional<std::vector<uint8_t>> psz_username; /* USER ID */

	public_key_packet_t key;       /* Public key packet */

	std::optional<signature_packet_t> sig; /* Signature packet, by the embedded key */
};

struct RSAKEY
{
	BLOBHEADER blobheader;
	RSAPUBKEY rsapubkey;
	BYTE n[MAX_MPI_BITS_RSA / 8];
};

[[nodiscard]] bool VerifyIntegrity(const CString& filename, const CString& signatureFilename, CUpdateDownloader* updateDownloader);
