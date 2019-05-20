// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2019 Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2014-2019 TortoiseGit
// Copyright (C) VLC project (http://videolan.org)
// - pgp parsing code was copied from src/misc/update(_crypto)?.c
// Copyright (C) The Internet Society (1998).  All Rights Reserved.
// - crc_octets() was lamely copied from rfc 2440

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
#include "stdafx.h"
#include "UpdateCrypto.h"
#include "FormatMessageWrapper.h"
#include <atlenc.h>
#define NEED_SIGNING_KEY
#include "../version.h"
#include "TempFile.h"
#include "SmartHandle.h"

#define packet_type(c) ((c & 0x3c) >> 2)      /* 0x3C = 00111100 */
#define packet_header_len(c) ((c & 0x03) + 1) /* number of bytes in a packet header */

#ifdef TGIT_UPDATECRYPTO_DSA
#ifndef TGIT_UPDATECRYPTO_SHA1
#error TGIT_UPDATECRYPTO_SHA1 required for DSA
#endif
#endif

static inline int scalar_number(const uint8_t *p, int header_len)
{
	ASSERT(header_len == 1 || header_len == 2 || header_len == 4);

	if (header_len == 1)
		return p[0];
	else if (header_len == 2)
		return (p[0] << 8) + p[1];
	else if (header_len == 4)
		return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
	else
		abort();
}

/* number of data bytes in a MPI */
static int mpi_len(const uint8_t* mpi)
{
	return (scalar_number(mpi, 2) + 7) / 8;
}

static size_t read_mpi(uint8_t* dst, const uint8_t* buf, size_t buflen, size_t bits)
{
	if (buflen < 2)
		return 0;

	size_t n = mpi_len(buf);

	if (n * 8 > bits)
		return 0;

	n += 2;

	if (buflen < n)
		return 0;

	memcpy(dst, buf, n);
	return n;
}

#define READ_MPI(d, bits) do \
	{ \
		size_t n = read_mpi(d, p_buf, i_packet_len - i_read, bits); \
		if (!n) goto error; \
		p_buf += n; \
		i_read += n; \
	} while(0)

/* Base64 decoding */
static size_t b64_decode_binary_to_buffer(uint8_t *p_dst, size_t i_dst, const char *p_src, size_t srcLen)
{
	int len = static_cast<int>(i_dst);
	if (!Base64Decode(p_src, static_cast<int>(srcLen), p_dst, &len))
		return 0;
	return len;
}

/*
 * crc_octets() was lamely copied from rfc 2440
 * Copyright (C) The Internet Society (1998).  All Rights Reserved.
 */
#define CRC24_INIT 0xB704CEL
#define CRC24_POLY 0x1864CFBL

static long crc_octets(uint8_t *octets, size_t len)
{
	long crc = CRC24_INIT;
	while (len--)
	{
		crc ^= (*octets++) << 16;
		for (int i = 0; i < 8; ++i)
		{
			crc <<= 1;
			if (crc & 0x1000000)
				crc ^= CRC24_POLY;
		}
	}
	return crc & 0xFFFFFFL;
}

static ALG_ID map_digestalgo(uint8_t digest_algo)
{
	switch (digest_algo)
	{
#ifdef TGIT_UPDATECRYPTO_SHA1
	case DIGEST_ALGO_SHA1:
		return CALG_SHA1;
#endif
	case DIGEST_ALGO_SHA256:
		return CALG_SHA_256;
	case DIGEST_ALGO_SHA384:
		return CALG_SHA_384;
	case DIGEST_ALGO_SHA512:
		return CALG_SHA_512;
	default:
		return 0;
	}
}

static DWORD map_algo(uint8_t digest_algo)
{
	switch (digest_algo)
	{
#ifdef TGIT_UPDATECRYPTO_DSA
	case PUBLIC_KEY_ALGO_DSA:
		return PROV_DSS;
#endif
	case PUBLIC_KEY_ALGO_RSA:
		return PROV_RSA_AES; // needed for SHA2, see http://msdn.microsoft.com/en-us/library/windows/desktop/aa387447%28v=vs.85%29.aspx
	default:
		return 0;
	}
}

/*
 * fill a signature_packet_v4_t from signature packet data
 * verify that it was used with a DSA or RSA public key
 */
static size_t parse_signature_v4_packet(signature_packet_t *p_sig, const uint8_t *p_buf, size_t i_sig_len)
{
	size_t i_read = 1; /* we already read the version byte */

	if (i_sig_len < 10) /* signature is at least 10 bytes + the 2 MPIs */
		return 0;

	p_sig->type = *p_buf++; i_read++;

	p_sig->public_key_algo = *p_buf++; i_read++;

	p_sig->digest_algo = *p_buf++; i_read++;
	if (!map_algo(p_sig->public_key_algo))
		return 0;

	memcpy(p_sig->specific.v4.hashed_data_len, p_buf, 2);
	p_buf += 2; i_read += 2;

	size_t i_hashed_data_len = scalar_number(p_sig->specific.v4.hashed_data_len, 2);
	i_read += i_hashed_data_len;
	if (i_read + 4 > i_sig_len || i_hashed_data_len > i_sig_len)
		return 0;

	p_sig->specific.v4.hashed_data = static_cast<uint8_t*>(malloc(i_hashed_data_len));
	if (!p_sig->specific.v4.hashed_data)
		return 0;
	memcpy(p_sig->specific.v4.hashed_data, p_buf, i_hashed_data_len);
	p_buf += i_hashed_data_len;

	memcpy(p_sig->specific.v4.unhashed_data_len, p_buf, 2);
	p_buf += 2; i_read += 2;

	size_t i_unhashed_data_len = scalar_number(p_sig->specific.v4.unhashed_data_len, 2);
	i_read += i_unhashed_data_len;
	if (i_read + 2 > i_sig_len || i_unhashed_data_len > i_sig_len)
		return 0;

	p_sig->specific.v4.unhashed_data = static_cast<uint8_t*>(malloc(i_unhashed_data_len));
	if (!p_sig->specific.v4.unhashed_data)
		return 0;

	memcpy(p_sig->specific.v4.unhashed_data, p_buf, i_unhashed_data_len);
	p_buf += i_unhashed_data_len;

	memcpy(p_sig->hash_verification, p_buf, 2);
	p_buf += 2; i_read += 2;

	uint8_t *p, *max_pos;
	p = p_sig->specific.v4.unhashed_data;
	max_pos = p + i_unhashed_data_len;

	for (;;)
	{
		if (p > max_pos)
			return 0;

		size_t i_subpacket_len;
		if (*p < 192)
		{
			if (p + 1 > max_pos)
				return 0;
			i_subpacket_len = *p++;
		}
		else if (*p < 255)
		{
			if (p + 2 > max_pos)
				return 0;
			i_subpacket_len = static_cast<size_t>(*p++ - 192) << 8;
			i_subpacket_len += *p++ + 192;
		}
		else
		{
			if (p + 4 > max_pos)
				return 0;
			i_subpacket_len = size_t(*++p) << 24;
			i_subpacket_len += static_cast<size_t>(*++p) << 16;
			i_subpacket_len += static_cast<size_t>(*++p) << 8;
			i_subpacket_len += *++p;
		}

		if (*p == ISSUER_SUBPACKET)
		{
			if (p + 9 > max_pos)
				return 0;

			memcpy(&p_sig->issuer_longid, p + 1, 8);

			return i_read;
		}

		if (i_subpacket_len > i_unhashed_data_len)
			return 0;

		p += i_subpacket_len;
	}
}

static int parse_signature_packet(signature_packet_t *p_sig, const uint8_t *p_buf, size_t i_packet_len)
{
	if (!i_packet_len) /* 1st sanity check, we need at least the version */
		return -1;

	p_sig->version = *p_buf++;

	size_t i_read;
	switch (p_sig->version)
	{
		case 4:
			p_sig->specific.v4.hashed_data = nullptr;
			p_sig->specific.v4.unhashed_data = nullptr;
			i_read = parse_signature_v4_packet(p_sig, p_buf, i_packet_len);
			break;
		default:
			return -1;
	}

	if (i_read == 0) /* signature packet parsing has failed */
		goto error;

	if (!map_algo(p_sig->public_key_algo))
		goto error;

	if (!map_digestalgo(p_sig->digest_algo))
		goto error;

	switch (p_sig->type)
	{
		case BINARY_SIGNATURE:
		case TEXT_SIGNATURE:
		case GENERIC_KEY_SIGNATURE:
		case PERSONA_KEY_SIGNATURE:
		case CASUAL_KEY_SIGNATURE:
		case POSITIVE_KEY_SIGNATURE:
			break;
		default:
			goto error;
	}

	p_buf--; /* rewind to the version byte */
	p_buf += i_read;

	switch (p_sig->public_key_algo)
	{
#ifdef TGIT_UPDATECRYPTO_DSA
	case PUBLIC_KEY_ALGO_DSA:
		READ_MPI(p_sig->algo_specific.dsa.r, 160);
		READ_MPI(p_sig->algo_specific.dsa.s, 160);
		break;
#endif
	case PUBLIC_KEY_ALGO_RSA:
		READ_MPI(p_sig->algo_specific.rsa.s, 4096);
		break;
	default:
		goto error;
	}

	if (i_read != i_packet_len)
		goto error;

	return 0;

error:
	if (p_sig->version == 4)
	{
		free(p_sig->specific.v4.hashed_data);
		free(p_sig->specific.v4.unhashed_data);
	}

	return -1;
}

/*
 * Transform an armored document in binary format
 * Used on public keys and signatures
 */
static int pgp_unarmor(const char *p_ibuf, size_t i_ibuf_len, uint8_t *p_obuf, size_t i_obuf_len)
{
	const char *p_ipos = p_ibuf;
	uint8_t *p_opos = p_obuf;
	int i_end = 0;
	int i_header_skipped = 0;

	while (!i_end && p_ipos < p_ibuf + i_ibuf_len && *p_ipos != '=')
	{
		if (*p_ipos == '\r' || *p_ipos == '\n')
		{
			p_ipos++;
			continue;
		}

		size_t i_line_len = strcspn(p_ipos, "\r\n");
		if (i_line_len == 0)
			continue;

		if (!i_header_skipped)
		{
			if (!strncmp(p_ipos, "-----BEGIN PGP", 14))
				i_header_skipped = 1;

			p_ipos += i_line_len + 1;
			continue;
		}

		if (!strncmp(p_ipos, "Version:", 8))
		{
			p_ipos += i_line_len + 1;
			continue;
		}

		if (p_ipos[i_line_len - 1] == '=')
		{
			i_end = 1;
		}

		p_opos += b64_decode_binary_to_buffer(p_opos, p_obuf - p_opos + i_obuf_len, p_ipos, static_cast<int>(i_line_len));
		p_ipos += i_line_len + 1;
	}

	if (p_ipos + 1 < p_ibuf + i_ibuf_len && (*p_ipos == '\r' || *p_ipos == '\n'))
		p_ipos++;

	/* XXX: the CRC is OPTIONAL, really require it ? */
	if (p_ipos + 5 > p_ibuf + i_ibuf_len || *p_ipos++ != '=')
		return 0;

	uint8_t p_crc[3];
	if (b64_decode_binary_to_buffer(p_crc, sizeof(p_crc), p_ipos, 5) != 3)
		return 0;

	long l_crc = crc_octets(p_obuf, p_opos - p_obuf);
	long l_crc2 = (0 << 24) + (p_crc[0] << 16) + (p_crc[1] << 8) + p_crc[2];

	return static_cast<int>((l_crc2 == l_crc) ? p_opos - p_obuf : 0);
}

/*
 * fill a public_key_packet_t structure from public key packet data
 * verify that it is a version 4 public key packet, using DSA or RSA
 */
static int parse_public_key_packet(public_key_packet_t *p_key, const uint8_t *p_buf, size_t i_packet_len)
{
	if (i_packet_len < 6)
		return -1;

	size_t i_read = 0;

	p_key->version = *p_buf++; i_read++;
	if (p_key->version != 4)
		return -1;

	/* XXX: warn when timestamp is > date ? */
	memcpy(p_key->timestamp, p_buf, 4); p_buf += 4; i_read += 4;

	p_key->algo = *p_buf++; i_read++;
	switch (p_key->algo)
	{
#ifdef TGIT_UPDATECRYPTO_DSA
	case PUBLIC_KEY_ALGO_DSA:
		if (i_packet_len > 418) // we only support 1024-bit DSA keys and SHA1 signatures, see verify_signature_dsa
			return -1;
		READ_MPI(p_key->sig.dsa.p, 1024);
		READ_MPI(p_key->sig.dsa.q, 160);
		READ_MPI(p_key->sig.dsa.g, 1024);
		READ_MPI(p_key->sig.dsa.y, 1024);
		break;
#endif
	case PUBLIC_KEY_ALGO_RSA:
		READ_MPI(p_key->sig.rsa.n, 4096);
		READ_MPI(p_key->sig.rsa.e, 4096);
		break;
	default:
		return -1;
	}

	if (i_read == i_packet_len)
		return 0;

error:
	return -1;
}

/*
 * fill a public_key_t with public key data, including:
 *   * public key packet
 *   * signature packet issued by key which long id is p_sig_issuer
 *   * user id packet
 */
static int parse_public_key(const uint8_t *p_key_data, size_t i_key_len, public_key_t *p_key, const uint8_t *p_sig_issuer)
{
	const uint8_t *pos = p_key_data;
	const uint8_t *max_pos = pos + i_key_len;

	int i_status = 0;
#define PUBLIC_KEY_FOUND    0x01
#define USER_ID_FOUND       0x02
#define SIGNATURE_FOUND     0X04

	uint8_t *p_key_unarmored = nullptr;

	p_key->psz_username = nullptr;
	p_key->sig.specific.v4.hashed_data = nullptr;
	p_key->sig.specific.v4.unhashed_data = nullptr;

	if (!(*pos & 0x80))
	{   /* first byte is ASCII, unarmoring */
		p_key_unarmored = static_cast<uint8_t*>(malloc(i_key_len));
		if (!p_key_unarmored)
			return -1;
		int i_len = pgp_unarmor(reinterpret_cast<const char*>(p_key_data), i_key_len, p_key_unarmored, i_key_len);

		if (i_len == 0)
			goto error;

		pos = p_key_unarmored;
		max_pos = pos + i_len;
	}

	while (pos < max_pos)
	{
		if (!(*pos & 0x80) || *pos & 0x40)
			goto error;

		int i_type = packet_type(*pos);

		int i_header_len = packet_header_len(*pos++);
		if (pos + i_header_len > max_pos || (i_header_len != 1 && i_header_len != 2 && i_header_len != 4))
			goto error;

		int i_packet_len = scalar_number(pos, i_header_len);
		pos += i_header_len;

		if (pos + i_packet_len > max_pos || i_packet_len < 0 || static_cast<size_t>(i_packet_len) > i_key_len)
			goto error;

		switch (i_type)
		{
			case PUBLIC_KEY_PACKET:
				i_status |= PUBLIC_KEY_FOUND;
				if (parse_public_key_packet(&p_key->key, pos, i_packet_len) != 0)
					goto error;
				break;

			case SIGNATURE_PACKET: /* we accept only v4 signatures here */
				if (i_status & SIGNATURE_FOUND || !p_sig_issuer)
					break;
				if (parse_signature_packet(&p_key->sig, pos, i_packet_len) == 0)
				{
					if (p_key->sig.version != 4)
						break;
					if (memcmp( p_key->sig.issuer_longid, p_sig_issuer, 8))
					{
						free(p_key->sig.specific.v4.hashed_data);
						free(p_key->sig.specific.v4.unhashed_data);
						p_key->sig.specific.v4.hashed_data = nullptr;
						p_key->sig.specific.v4.unhashed_data = nullptr;
						break;
					}
					i_status |= SIGNATURE_FOUND;
				}
				break;

			case USER_ID_PACKET:
				if (p_key->psz_username) /* save only the first User ID */
					break;
				i_status |= USER_ID_FOUND;
				p_key->psz_username = static_cast<uint8_t*>(malloc(i_packet_len + 1));
				if (!p_key->psz_username)
					goto error;

				memcpy(p_key->psz_username, pos, i_packet_len);
				p_key->psz_username[i_packet_len] = '\0';
				break;

			default:
				break;
		}
		pos += i_packet_len;
	}
	free(p_key_unarmored);

	if (!(i_status & (PUBLIC_KEY_FOUND | USER_ID_FOUND)))
		return -1;

	if (p_sig_issuer && !(i_status & SIGNATURE_FOUND))
		return -1;

	return 0;

error:
	if (p_key->sig.version == 4)
	{
		free(p_key->sig.specific.v4.hashed_data);
		free(p_key->sig.specific.v4.unhashed_data);
	}
	free(p_key->psz_username);
	free(p_key_unarmored);
	return -1;
}

static int LoadSignature(const CString &signatureFilename, signature_packet_t *p_sig)
{
	FILE* pFile = _wfsopen(signatureFilename, L"rb", SH_DENYWR);
	if (!pFile)
		return -1;

	int size = 65536;
	auto buffer = std::make_unique<unsigned char[]>(size);
	int length = static_cast<int>(fread(buffer.get(), sizeof(char), size, pFile));
	fclose(pFile);
	if (length < 8)
		return -1;

	// is unpacking needed?
	if (static_cast<uint8_t>(buffer[0]) < 0x80)
	{
		auto unpacked = std::make_unique<unsigned char[]>(size);
		size = pgp_unarmor(reinterpret_cast<const char*>(buffer.get()), length, unpacked.get(), length);

		if (size < 2)
			return -1;

		buffer.swap(unpacked);
	}
	else
		size = length;

	if (packet_type(buffer[0]) != SIGNATURE_PACKET)
		return -1;

	DWORD i_header_len = packet_header_len(buffer[0]);
	if ((i_header_len != 1 && i_header_len != 2 && i_header_len != 4) || i_header_len + 1 > static_cast<DWORD>(size))
		return -1;

	DWORD i_len = scalar_number(static_cast<uint8_t*>(buffer.get() + 1), i_header_len);
	if (i_len + i_header_len + 1 != static_cast<DWORD>(size))
		return -1;

	if (parse_signature_packet(p_sig, static_cast<uint8_t*>(buffer.get() + 1 + i_header_len), i_len))
		return -1;

	if (p_sig->type != BINARY_SIGNATURE && p_sig->type != TEXT_SIGNATURE)
	{
		if (p_sig->version == 4)
		{
			free(p_sig->specific.v4.hashed_data);
			free(p_sig->specific.v4.unhashed_data);
		}
		return -1;
	}

	return 0;
}

static void CryptHashChar(HCRYPTHASH hHash, const int c)
{
	CryptHashData(hHash, reinterpret_cast<const BYTE*>(&c), 1, 0);
}

/* final part of the hash */
static int hash_finish(HCRYPTHASH hHash, signature_packet_t *p_sig)
{
	if (p_sig->version == 4)
	{
		CryptHashChar(hHash, p_sig->version);
		CryptHashChar(hHash, p_sig->type);
		CryptHashChar(hHash, p_sig->public_key_algo);
		CryptHashChar(hHash, p_sig->digest_algo);
		CryptHashData(hHash, p_sig->specific.v4.hashed_data_len, 2, 0);
		unsigned int i_len = scalar_number(p_sig->specific.v4.hashed_data_len, 2);
		CryptHashData(hHash, p_sig->specific.v4.hashed_data, i_len, 0);

		CryptHashChar(hHash, 0x04);
		CryptHashChar(hHash, 0xFF);

		i_len += 6; /* hashed data + 6 bytes header */

		CryptHashChar(hHash, (i_len >> 24) & 0xff);
		CryptHashChar(hHash, (i_len >> 16) & 0xff);
		CryptHashChar(hHash, (i_len >> 8) & 0xff);
		CryptHashChar(hHash, (i_len) & 0xff);
	}
	else
	{  /* RFC 4880 only tells about versions 3 and 4 */
		return -1;
	}

	return 0;
}

/*
 * Generate a hash on a public key, to verify a signature made on that hash
 * Note that we need the signature (v4) to compute the hash
 */
static int hash_from_public_key(HCRYPTHASH hHash, public_key_t* p_pkey)
{
	if (p_pkey->sig.version != 4)
		return -1;

	if (p_pkey->sig.type < GENERIC_KEY_SIGNATURE || p_pkey->sig.type > POSITIVE_KEY_SIGNATURE)
		return -1;

	DWORD i_size = 0;
#ifdef TGIT_UPDATECRYPTO_DSA
	unsigned int i_p_len = 0, i_g_len = 0, i_q_len = 0, i_y_len = 0;
#endif
	unsigned int i_n_len = 0, i_e_len = 0;

	switch (p_pkey->key.algo)
	{
#ifdef TGIT_UPDATECRYPTO_DSA
	case PUBLIC_KEY_ALGO_DSA:
		i_p_len = mpi_len(p_pkey->key.sig.dsa.p);
		i_g_len = mpi_len(p_pkey->key.sig.dsa.g);
		i_q_len = mpi_len(p_pkey->key.sig.dsa.q);
		i_y_len = mpi_len(p_pkey->key.sig.dsa.y);

		i_size = 6 + 2 * 4 + i_p_len + i_g_len + i_q_len + i_y_len;
		break;
#endif
	case PUBLIC_KEY_ALGO_RSA:
		i_n_len = mpi_len(p_pkey->key.sig.rsa.n);
		i_e_len = mpi_len(p_pkey->key.sig.rsa.e);

		i_size = 6 + 2 * 2 + i_n_len + i_e_len;
		break;
	default:
		return -1;
	}

	CryptHashChar(hHash, 0x99);

	CryptHashChar(hHash, (i_size >> 8) & 0xff);
	CryptHashChar(hHash, i_size & 0xff);

	CryptHashChar(hHash, p_pkey->key.version);
	CryptHashData(hHash, p_pkey->key.timestamp, 4, 0);
	CryptHashChar(hHash, p_pkey->key.algo);

	switch (p_pkey->key.algo)
	{
#ifdef TGIT_UPDATECRYPTO_DSA
	case PUBLIC_KEY_ALGO_DSA:
		CryptHashData(hHash, reinterpret_cast<uint8_t*>(&p_pkey->key.sig.dsa.p), 2 + i_p_len, 0);
		CryptHashData(hHash, reinterpret_cast<uint8_t*>(&p_pkey->key.sig.dsa.q), 2 + i_q_len, 0);
		CryptHashData(hHash, reinterpret_cast<uint8_t*>(&p_pkey->key.sig.dsa.g), 2 + i_g_len, 0);
		CryptHashData(hHash, reinterpret_cast<uint8_t*>(&p_pkey->key.sig.dsa.y), 2 + i_y_len, 0);
		break;
#endif
	case PUBLIC_KEY_ALGO_RSA:
		CryptHashData(hHash, reinterpret_cast<uint8_t*>(&p_pkey->key.sig.rsa.n), 2 + i_n_len, 0);
		CryptHashData(hHash, reinterpret_cast<uint8_t*>(&p_pkey->key.sig.rsa.e), 2 + i_e_len, 0);
		break;
	}

	CryptHashChar(hHash, 0xb4);

	size_t i_len = strlen(reinterpret_cast<char*>(p_pkey->psz_username));

	CryptHashChar(hHash, (i_len >> 24) & 0xff);
	CryptHashChar(hHash, (i_len >> 16) & 0xff);
	CryptHashChar(hHash, (i_len >> 8) & 0xff);
	CryptHashChar(hHash, (i_len) & 0xff);

	CryptHashData(hHash, p_pkey->psz_username, static_cast<DWORD>(i_len), 0);

	return hash_finish(hHash, &p_pkey->sig);
}

static int hash_from_file(HCRYPTHASH hHash, CString filename, signature_packet_t* p_sig)
{
	CAutoFILE pFile = _wfsopen(filename, L"rb", SH_DENYWR);
	if (!pFile)
		return -1;

	char buf[4097] = { 0 };
	int read = 0;
	int nlHandling = 0;
	while ((read = static_cast<int>(fread(buf, sizeof(char), sizeof(buf) - 1, pFile))) > 0)
	{
		if (p_sig->type == TEXT_SIGNATURE)
		{
			buf[read] = '\0';
			char * psz_string = buf;
			while (*psz_string)
			{
				if (nlHandling == 1 && (*psz_string == '\r' || *psz_string == '\n'))
				{
					CryptHashChar(hHash, '\r');
					CryptHashChar(hHash, '\n');
					nlHandling = 2;
				}
				if (nlHandling == 2 && *psz_string == '\r')
				{
					psz_string++;
					nlHandling = 2;
					if (!*psz_string)
						break;
				}

				if ((nlHandling == 2 || nlHandling == 3) && *psz_string == '\n')
				{
					psz_string++;
					if (!*psz_string)
						break;
				}

				size_t i_len = strcspn(psz_string, "\r\n");

				if (i_len)
				{
					CryptHashData(hHash, reinterpret_cast<BYTE*>(psz_string), static_cast<DWORD>(i_len), 0);
					psz_string += i_len;
				}

				nlHandling = 1;
				if (*psz_string == '\r' || *psz_string == '\n')
				{
					CryptHashChar(hHash, '\r');
					CryptHashChar(hHash, '\n');
					nlHandling = 2;

					if (*psz_string == '\r')
					{
						psz_string++;
						nlHandling = 3;
					}

					if (*psz_string == '\n')
					{
						psz_string++;
						nlHandling = 0;
					}
				}
			}
		}
		else
			CryptHashData(hHash, reinterpret_cast<BYTE*>(buf), read, 0);
	}

	return hash_finish(hHash, p_sig);
}

static int check_hash(HCRYPTHASH hHash, signature_packet_t *p_sig)
{
	DWORD hashLen;
	DWORD hashLenLen = sizeof(DWORD);
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashLen), &hashLenLen, 0))
		return -1;

	auto pHash = std::make_unique<BYTE[]>(hashLen);
	CryptGetHashParam(hHash, HP_HASHVAL, pHash.get(), &hashLen, 0);

	if (pHash[0] != p_sig->hash_verification[0] || pHash[1] != p_sig->hash_verification[1])
		return -1;

	return 0;
}

/*
* Verify an OpenPGP signature made with some RSA public key
*/
static int verify_signature_rsa(HCRYPTPROV hCryptProv, HCRYPTHASH hHash, public_key_t& p_pkey, signature_packet_t& p_sig)
{
	int i_n_len = min(mpi_len(p_pkey.key.sig.rsa.n), static_cast<int>(sizeof(p_pkey.key.sig.rsa.n)) - 2);
	int i_s_len = min(mpi_len(p_sig.algo_specific.rsa.s), static_cast<int>(sizeof(p_sig.algo_specific.rsa.s)) - 2);

	if (i_s_len > i_n_len)
		return -1;

	RSAKEY rsakey;
	rsakey.blobheader.bType = PUBLICKEYBLOB; // 0x06
	rsakey.blobheader.bVersion = CUR_BLOB_VERSION; // 0x02
	rsakey.blobheader.reserved = 0;
	rsakey.blobheader.aiKeyAlg = CALG_RSA_KEYX;
	rsakey.rsapubkey.magic = 0x31415352;// ASCII for RSA1
	rsakey.rsapubkey.bitlen = i_n_len * 8;
	rsakey.rsapubkey.pubexp = 65537; // gnupg only uses this

	memcpy(rsakey.n, p_pkey.key.sig.rsa.n + 2, i_n_len); std::reverse(rsakey.n, rsakey.n + i_n_len);

	HCRYPTKEY hPubKey;
	if (CryptImportKey(hCryptProv, reinterpret_cast<BYTE*>(&rsakey), sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + i_n_len, 0, 0, &hPubKey) == 0)
		return -1;
	SCOPE_EXIT{ CryptDestroyKey(hPubKey); };

	/* i_s_len might be shorter than i_n_len,
	 * but CrytoAPI requires that both have same length,
	 * thus, use i_n_len as buffer length (pSig; it's safe as i_n_len cannot be longer than the buffer),
	 * but do not copy/reverse NULs at the end of p_sig.algo_specific.rsa.s into pSig
	 */
	auto pSig = std::make_unique<BYTE[]>(i_n_len);
	SecureZeroMemory(pSig.get(), i_n_len);
	memcpy(pSig.get(), p_sig.algo_specific.rsa.s + 2, i_s_len);
	std::reverse(pSig.get(), pSig.get() + i_s_len);
	if (!CryptVerifySignature(hHash, pSig.get(), i_n_len, hPubKey, nullptr, 0))
		return -1;

	return 0;
}

#ifdef TGIT_UPDATECRYPTO_DSA
/*
* Verify an OpenPGP signature made with some DSA public key
*/
static int verify_signature_dsa(HCRYPTPROV hCryptProv, HCRYPTHASH hHash, public_key_t& p_pkey, signature_packet_t& p_sig)
{
	if (p_sig.digest_algo != DIGEST_ALGO_SHA1) // PROV_DSS only supports SHA1 signatures, see http://msdn.microsoft.com/en-us/library/windows/desktop/aa387434%28v=vs.85%29.aspx
		return -1;

	int i_p_len = min(mpi_len(p_pkey.key.sig.dsa.p), static_cast<int>(sizeof(p_pkey.key.sig.dsa.p)) - 2);
	int i_q_len = min(mpi_len(p_pkey.key.sig.dsa.q), static_cast<int>(sizeof(p_pkey.key.sig.dsa.q)) - 2);
	int i_g_len = min(mpi_len(p_pkey.key.sig.dsa.g), static_cast<int>(sizeof(p_pkey.key.sig.dsa.g)) - 2);
	int i_y_len = min(mpi_len(p_pkey.key.sig.dsa.y), static_cast<int>(sizeof(p_pkey.key.sig.dsa.y)) - 2);
	int i_r_len = min(mpi_len(p_sig.algo_specific.dsa.r), static_cast<int>(sizeof(p_sig.algo_specific.dsa.r)) - 2);
	int i_s_len = min(mpi_len(p_sig.algo_specific.dsa.s), static_cast<int>(sizeof(p_sig.algo_specific.dsa.s)) - 2);

	// CryptoAPI only supports 1024-bit DSA keys and SHA1 signatures
	if (i_p_len > 128 || i_q_len > 20 && i_g_len > 128 || i_y_len > 128 || i_r_len > 20 || i_s_len > 20)
		return -1;

	HCRYPTKEY hPubKey;
	// based on http://www.derkeiler.com/Newsgroups/microsoft.public.platformsdk.security/2004-10/0040.html
	DSAKEY dsakey = { 0 };
	dsakey.blobheader.bType = PUBLICKEYBLOB; // 0x06
	dsakey.blobheader.bVersion = CUR_BLOB_VERSION + 1; // 0x03
	dsakey.blobheader.reserved = 0;
	dsakey.blobheader.aiKeyAlg = CALG_DSS_SIGN;
	dsakey.dsspubkeyver3.magic = 0x33535344; // ASCII of "DSS3";
	dsakey.dsspubkeyver3.bitlenP = i_p_len * 8; // # of bits in prime modulus
	dsakey.dsspubkeyver3.bitlenQ = i_q_len * 8; // # of bits in prime q, 0 if not available
	dsakey.dsspubkeyver3.bitlenJ = 0; // # of bits in (p-1)/q, 0 if not available
	dsakey.dsspubkeyver3.DSSSeed.counter = 0xFFFFFFFF; // not available

	memcpy(dsakey.p, p_pkey.key.sig.dsa.p + 2, i_p_len); std::reverse(dsakey.p, dsakey.p + i_p_len);
	memcpy(dsakey.q, p_pkey.key.sig.dsa.q + 2, i_q_len); std::reverse(dsakey.q, dsakey.q + i_q_len);
	memcpy(dsakey.g, p_pkey.key.sig.dsa.g + 2, i_g_len); std::reverse(dsakey.g, dsakey.g + i_g_len);
	memcpy(dsakey.y, p_pkey.key.sig.dsa.y + 2, i_y_len); std::reverse(dsakey.y, dsakey.y + i_y_len);

	if (CryptImportKey(hCryptProv, reinterpret_cast<BYTE*>(&dsakey), sizeof(dsakey), 0, 0, &hPubKey) == 0)
		return -1;

	SCOPE_EXIT { CryptDestroyKey(hPubKey); };

	unsigned char signature[40] = { 0 };
	memcpy(signature, p_sig.algo_specific.dsa.r + 2, i_r_len);
	memcpy(signature + 20, p_sig.algo_specific.dsa.s + 2, i_s_len);
	std::reverse(signature, signature + i_r_len);
	std::reverse(signature + 20, signature + 20 + i_s_len);
	if (!CryptVerifySignature(hHash, signature, sizeof(signature), hPubKey, nullptr, 0))
		return -1;

	return 0;
}
#endif

/*
* Verify an OpenPGP signature made with some public key
*/
int verify_signature(HCRYPTPROV hCryptProv, HCRYPTHASH hHash, public_key_t& p_key, signature_packet_t& sign)
{
	switch (sign.public_key_algo)
	{
#ifdef TGIT_UPDATECRYPTO_DSA
	case PUBLIC_KEY_ALGO_DSA:
		return verify_signature_dsa(hCryptProv, hHash, p_key, sign);
#endif
	case PUBLIC_KEY_ALGO_RSA:
		return verify_signature_rsa(hCryptProv, hHash, p_key, sign);
	default:
		return -1;
	}
}

#ifndef GTEST_INCLUDE_GTEST_GTEST_H_
/*
 * download a public key (the last one) from TortoiseGit server, and parse it
 */
static public_key_t *download_key(const uint8_t *p_longid, const uint8_t *p_signature_issuer, CUpdateDownloader *updateDownloader)
{
	ASSERT(updateDownloader);

	CString url;
	url.Format(L"http://download.tortoisegit.org/keys/%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X.asc", p_longid[0], p_longid[1], p_longid[2], p_longid[3], p_longid[4], p_longid[5], p_longid[6], p_longid[7]);

	CString tempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	if (updateDownloader->DownloadFile(url, tempfile, false))
		return nullptr;

	int size = 65536;
	auto buffer = std::make_unique<char[]>(size);
	FILE * pFile = _wfsopen(tempfile, L"rb", SH_DENYWR);
	if (pFile)
	{
		SCOPE_EXIT{ fclose(pFile); };
		int length = 0;
		if ((length = static_cast<int>(fread(buffer.get(), sizeof(char), size, pFile))) >= 8)
			size = length;
		else
			return nullptr;
	}
	else
		return nullptr;

	auto p_pkey = static_cast<public_key_t*>(malloc(sizeof(public_key_t)));
	if (!p_pkey)
	{
		DeleteUrlCacheEntry(url);
		return nullptr;
	}

	memcpy(p_pkey->longid, p_longid, 8);

	if (parse_public_key(reinterpret_cast<const uint8_t*>(buffer.get()), size, p_pkey, p_signature_issuer))
	{
		free(p_pkey);
		return nullptr;
	}

	return p_pkey;
}
#endif

int VerifyIntegrity(const CString &filename, const CString &signatureFilename, CUpdateDownloader *updateDownloader)
{
	signature_packet_t p_sig = { 0 };
	if (LoadSignature(signatureFilename, &p_sig))
		return -1;
	SCOPE_EXIT
	{
		if (p_sig.version == 4)
		{
			free(p_sig.specific.v4.hashed_data);
			free(p_sig.specific.v4.unhashed_data);
		}
	};

	public_key_t p_pkey = { 0 };
	if (parse_public_key(tortoisegit_public_key, sizeof(tortoisegit_public_key), &p_pkey, nullptr))
		return -1;
	SCOPE_EXIT { free(p_pkey.psz_username); };
	memcpy(p_pkey.longid, tortoisegit_public_key_longid, 8);

	HCRYPTPROV hCryptProv;
	if (!CryptAcquireContext(&hCryptProv, nullptr, nullptr, map_algo(p_pkey.key.algo), CRYPT_VERIFYCONTEXT))
		return -1;
	SCOPE_EXIT { CryptReleaseContext(hCryptProv, 0); };

	if (memcmp(p_sig.issuer_longid, p_pkey.longid, 8) != 0)
	{
		public_key_t *p_new_pkey = nullptr;
#ifndef GTEST_INCLUDE_GTEST_GTEST_H_
		if (updateDownloader)
			p_new_pkey = download_key(p_sig.issuer_longid, tortoisegit_public_key_longid, updateDownloader);
#else
		UNREFERENCED_PARAMETER(updateDownloader);
#endif
		if (!p_new_pkey)
			return -1;
		SCOPE_EXIT
		{
			if (p_new_pkey->sig.version == 4)
			{
				p_new_pkey->sig.version = 0;
				free(p_new_pkey->sig.specific.v4.hashed_data);
				free(p_new_pkey->sig.specific.v4.unhashed_data);
			}
			if (p_new_pkey == &p_pkey)
				return;
			free(p_new_pkey->psz_username);
			free(p_new_pkey);
		};

		HCRYPTHASH hHash;
		if (!CryptCreateHash(hCryptProv, map_digestalgo(p_sig.digest_algo), 0, 0, &hHash))
			return -1;
		SCOPE_EXIT { CryptDestroyHash(hHash); };

		if (hash_from_public_key(hHash, p_new_pkey))
			return -1;

		if (check_hash(hHash, &p_new_pkey->sig))
			return -1;

		if (verify_signature(hCryptProv, hHash, p_pkey, p_new_pkey->sig))
			return -1;

		free(p_pkey.psz_username);
		p_pkey = *p_new_pkey;
	}

	HCRYPTHASH hHash;
	if (!CryptCreateHash(hCryptProv, map_digestalgo(p_sig.digest_algo), 0, 0, &hHash))
		return -1;
	SCOPE_EXIT{ CryptDestroyHash(hHash); };

	if (hash_from_file(hHash, filename, &p_sig))
		return -1;

	if (check_hash(hHash, &p_sig))
		return -1;

	if (verify_signature(hCryptProv, hHash, p_pkey, p_sig))
		return -1;

	return 0;
}
