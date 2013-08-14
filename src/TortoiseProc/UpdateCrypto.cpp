// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 Sven Strickroth <email@cs-ware.de>
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
#include "..\..\version.h"
#include "TempFile.h"

#define packet_type(c) ((c & 0x3c) >> 2)      /* 0x3C = 00111100 */
#define packet_header_len(c) ((c & 0x03) + 1) /* number of bytes in a packet header */

/* number of data bytes in a MPI */
#define mpi_len(mpi) ((scalar_number(mpi, 2) + 7) / 8)

#define READ_MPI(n, bits) do { \
	if (i_read + 2 > i_packet_len) \
		goto error; \
	int len = mpi_len(p_buf); \
	if (len > (bits) / 8 || i_read + 2 + len > i_packet_len) \
		goto error; \
	len += 2; \
	memcpy(n, p_buf, len); \
	p_buf += len; i_read += len; \
	} while(0)

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

/* Base64 decoding */
static size_t b64_decode_binary_to_buffer(uint8_t *p_dst, size_t i_dst, const char *p_src, size_t srcLen)
{
	int len = (int)i_dst;
	if (!Base64Decode(p_src, (int)srcLen, p_dst, &len))
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
	int i;
	while (len--)
	{
		crc ^= (*octets++) << 16;
		for (i = 0; i < 8; i++)
		{
			crc <<= 1;
			if (crc & 0x1000000)
				crc ^= CRC24_POLY;
		}
	}
	return crc & 0xFFFFFFL;
}

static size_t parse_signature_v3_packet(signature_packet_t *p_sig, const uint8_t *p_buf, size_t i_sig_len)
{
	size_t i_read = 1; /* we already read the version byte */

	if (i_sig_len < 19) /* signature is at least 19 bytes + the 2 MPIs */
		return 0;

	p_sig->specific.v3.hashed_data_len = *p_buf++; i_read++;
	if (p_sig->specific.v3.hashed_data_len != 5)
		return 0;

	p_sig->type = *p_buf++; i_read++;

	memcpy(p_sig->specific.v3.timestamp, p_buf, 4);
	p_buf += 4; i_read += 4;

	memcpy(p_sig->issuer_longid, p_buf, 8);
	p_buf += 8; i_read += 8;

	p_sig->public_key_algo = *p_buf++; i_read++;

	p_sig->digest_algo = *p_buf++; i_read++;

	p_sig->hash_verification[0] = *p_buf++; i_read++;
	p_sig->hash_verification[1] = *p_buf++; i_read++;

	ASSERT(i_read == 19);

	return i_read;
}

/*
 * fill a signature_packet_v4_t from signature packet data
 * verify that it was used with a DSA public key, using SHA-1 digest
 */
static size_t parse_signature_v4_packet(signature_packet_t *p_sig, const uint8_t *p_buf, size_t i_sig_len)
{
	size_t i_read = 1; /* we already read the version byte */

	if (i_sig_len < 10) /* signature is at least 10 bytes + the 2 MPIs */
		return 0;

	p_sig->type = *p_buf++; i_read++;

	p_sig->public_key_algo = *p_buf++; i_read++;

	p_sig->digest_algo = *p_buf++; i_read++;

	memcpy(p_sig->specific.v4.hashed_data_len, p_buf, 2);
	p_buf += 2; i_read += 2;

	size_t i_hashed_data_len = scalar_number(p_sig->specific.v4.hashed_data_len, 2);
	i_read += i_hashed_data_len;
	if (i_read + 4 > i_sig_len)
		return 0;

	p_sig->specific.v4.hashed_data = (uint8_t*) malloc(i_hashed_data_len);
	if (!p_sig->specific.v4.hashed_data)
		return 0;
	memcpy(p_sig->specific.v4.hashed_data, p_buf, i_hashed_data_len);
	p_buf += i_hashed_data_len;

	memcpy(p_sig->specific.v4.unhashed_data_len, p_buf, 2);
	p_buf += 2; i_read += 2;

	size_t i_unhashed_data_len = scalar_number(p_sig->specific.v4.unhashed_data_len, 2);
	i_read += i_unhashed_data_len;
	if (i_read + 2 > i_sig_len)
		return 0;

	p_sig->specific.v4.unhashed_data = (uint8_t*) malloc(i_unhashed_data_len);
	if (!p_sig->specific.v4.unhashed_data)
		return 0;

	memcpy(p_sig->specific.v4.unhashed_data, p_buf, i_unhashed_data_len);
	p_buf += i_unhashed_data_len;

	memcpy(p_sig->hash_verification, p_buf, 2);
	p_buf += 2; i_read += 2;

	uint8_t *p, *max_pos;
	p = p_sig->specific.v4.unhashed_data;
	max_pos = p + scalar_number(p_sig->specific.v4.unhashed_data_len, 2);

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
			i_subpacket_len = (*p++ - 192) << 8;
			i_subpacket_len += *p++ + 192;
		}
		else
		{
			if (p + 4 > max_pos)
				return 0;
			i_subpacket_len = *++p << 24;
			i_subpacket_len += *++p << 16;
			i_subpacket_len += *++p << 8;
			i_subpacket_len += *++p;
		}

		if (*p == ISSUER_SUBPACKET)
		{
			if (p + 9 > max_pos)
				return 0;

			memcpy(&p_sig->issuer_longid, p + 1, 8);

			return i_read;
		}

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
		case 3:
			i_read = parse_signature_v3_packet(p_sig, p_buf, i_packet_len);
			break;
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

	if (p_sig->public_key_algo != PUBLIC_KEY_ALGO_DSA)
		goto error;

	if (p_sig->digest_algo != DIGEST_ALGO_SHA1)
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

	READ_MPI(p_sig->r, 160);
	READ_MPI(p_sig->s, 160);

	ASSERT(i_read == i_packet_len);
	if (i_read < i_packet_len) /* some extra data, hm ? */
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

		p_opos += b64_decode_binary_to_buffer(p_opos, p_obuf - p_opos + i_obuf_len, p_ipos, (int)i_line_len);
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

	return (int)((l_crc2 == l_crc) ? p_opos - p_obuf : 0);
}

/*
 * fill a public_key_packet_t structure from public key packet data
 * verify that it is a version 4 public key packet, using DSA
 */
static int parse_public_key_packet(public_key_packet_t *p_key, const uint8_t *p_buf, size_t i_packet_len)
{
	if (i_packet_len > 418 || i_packet_len < 6)
		return -1;

	size_t i_read = 0;

	p_key->version = *p_buf++; i_read++;
	if (p_key->version != 4)
		return -1;

	/* XXX: warn when timestamp is > date ? */
	memcpy(p_key->timestamp, p_buf, 4); p_buf += 4; i_read += 4;

	p_key->algo = *p_buf++; i_read++;
	if (p_key->algo != PUBLIC_KEY_ALGO_DSA)
		return -1;

	READ_MPI(p_key->p, 1024);
	READ_MPI(p_key->q, 160);
	READ_MPI(p_key->g, 1024);
	READ_MPI(p_key->y, 1024);

	if (i_read != i_packet_len) /* some extra data eh ? */
		return -1;

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
		p_key_unarmored = (uint8_t*)malloc(i_key_len);
		if (!p_key_unarmored)
			return -1;
		int i_len = pgp_unarmor((char*)p_key_data, i_key_len, p_key_unarmored, i_key_len);

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

		if (pos + i_packet_len > max_pos)
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
				p_key->psz_username = (uint8_t*)malloc(i_packet_len + 1);
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
	FILE * pFile = _tfsopen(signatureFilename, _T("rb"), SH_DENYWR);
	if (pFile)
	{
		int size = 65536;
		std::unique_ptr<unsigned char[]> buffer(new unsigned char[size]);
		int length = 0;
		if ((length = (int)fread(buffer.get(), sizeof(char), size, pFile)) >= 8)
		{
			fclose(pFile);
			// is unpacking needed?
			if ((uint8_t)buffer[0] < 0x80)
			{
				std::unique_ptr<unsigned char[]> unpacked(new unsigned char[size]);
				size = pgp_unarmor((char *)buffer.get(), length, unpacked.get(), length);

				if (size < 2)
					return -1;

				buffer.swap(unpacked);
			}
			else
				size = length;

			if (packet_type(buffer[0]) != SIGNATURE_PACKET)
				return -1;

			DWORD i_header_len = packet_header_len(buffer[0]);
			if ((i_header_len != 1 && i_header_len != 2 && i_header_len != 4) || i_header_len + 1 > (DWORD)size)
				return -1;

			DWORD i_len = scalar_number((uint8_t *)(buffer.get() + 1), i_header_len);
			if (i_len + i_header_len + 1 != (DWORD)size)
				return -1;

			if (parse_signature_packet(p_sig, (uint8_t *)(buffer.get() + 1 + i_header_len), i_len))
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
		else
			fclose(pFile);
	}
	return -1;
}

static void CryptHashChar(HCRYPTHASH hHash, const int c)
{
	CryptHashData(hHash, (BYTE *)&c, 1, 0);
}

/* final part of the hash */
static int hash_finish(HCRYPTHASH hHash, signature_packet_t *p_sig)
{
	if (p_sig->version == 3)
	{
		CryptHashChar(hHash, p_sig->type);
		CryptHashData(hHash, (unsigned char*)&p_sig->specific.v3.timestamp, 4, 0);
	}
	else if (p_sig->version == 4)
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
 * Generate a SHA1 hash on a public key, to verify a signature made on that hash
 * Note that we need the signature (v4) to compute the hash
 */
static int hash_sha1_from_public_key(HCRYPTHASH hHash, public_key_t *p_pkey)
{
	if (p_pkey->sig.version != 4)
		return -1;

	if (p_pkey->sig.type < GENERIC_KEY_SIGNATURE || p_pkey->sig.type > POSITIVE_KEY_SIGNATURE)
		return -1;

	CryptHashChar(hHash, 0x99);

	unsigned int i_p_len = mpi_len(p_pkey->key.p);
	unsigned int i_g_len = mpi_len(p_pkey->key.g);
	unsigned int i_q_len = mpi_len(p_pkey->key.q);
	unsigned int i_y_len = mpi_len(p_pkey->key.y);

	DWORD i_size = 6 + 2*4 + i_p_len + i_g_len + i_q_len + i_y_len;

	CryptHashChar(hHash, (i_size >> 8) & 0xff);
	CryptHashChar(hHash, i_size & 0xff);

	CryptHashChar(hHash, p_pkey->key.version);
	CryptHashData(hHash, p_pkey->key.timestamp, 4, 0);
	CryptHashChar(hHash, p_pkey->key.algo);

	CryptHashData(hHash, (uint8_t*)&p_pkey->key.p, 2, 0);
	CryptHashData(hHash, (uint8_t*)&p_pkey->key.p + 2, i_p_len, 0);

	CryptHashData(hHash, (uint8_t*)&p_pkey->key.q, 2, 0);
	CryptHashData(hHash, (uint8_t*)&p_pkey->key.q + 2, i_q_len, 0);

	CryptHashData(hHash, (uint8_t*)&p_pkey->key.g, 2, 0);
	CryptHashData(hHash, (uint8_t*)&p_pkey->key.g + 2, i_g_len, 0);

	CryptHashData(hHash, (uint8_t*)&p_pkey->key.y, 2, 0);
	CryptHashData(hHash, (uint8_t*)&p_pkey->key.y + 2, i_y_len, 0);

	CryptHashChar(hHash, 0xb4);

	size_t i_len = strlen((char *)p_pkey->psz_username);

	CryptHashChar(hHash, (i_len >> 24) & 0xff);
	CryptHashChar(hHash, (i_len >> 16) & 0xff);
	CryptHashChar(hHash, (i_len >> 8) & 0xff);
	CryptHashChar(hHash, (i_len) & 0xff);

	CryptHashData(hHash, p_pkey->psz_username, (DWORD)i_len, 0);

	return hash_finish(hHash, &p_pkey->sig);
}

static int hash_sha1_from_file(HCRYPTHASH hHash, CString filename, signature_packet_t *p_sig)
{
	FILE * pFile = _tfsopen(filename, _T("rb"), SH_DENYWR);
	if (!pFile)
		return -1;

	char buf[4097];
	int read = 0;
	int nlHandling = 0;
	while ((read = (int)fread(buf, sizeof(char), sizeof(buf) - 1, pFile)) > 0)
	{
		if (p_sig->type == TEXT_SIGNATURE)
		{
			buf[read] = 0;
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
					CryptHashData(hHash, (BYTE *)psz_string, (DWORD)i_len, 0);
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
			CryptHashData(hHash, (BYTE *)buf, read, 0);
	}

	fclose(pFile);

	return hash_finish(hHash, p_sig);
}

static int check_hash(HCRYPTHASH hHash, signature_packet_t *p_sig)
{
	DWORD len = 20;
	unsigned char hash[20];
	CryptGetHashParam(hHash, HP_HASHVAL, hash, &len, 0);

	if (hash[0] != p_sig->hash_verification[0] || hash[1] != p_sig->hash_verification[1])
		return -1;

	return 0;
}

static int verify_signature(HCRYPTPROV hCryptProv, HCRYPTHASH hHash, public_key_t &p_pkey, signature_packet_t &p_sig)
{
	HCRYPTKEY hPubKey;
	// based on http://www.derkeiler.com/Newsgroups/microsoft.public.platformsdk.security/2004-10/0040.html
	DSAKEY dsakey;
	dsakey.blobheader.bType = PUBLICKEYBLOB; // 0x06
	dsakey.blobheader.bVersion = CUR_BLOB_VERSION + 1; // 0x03
	dsakey.blobheader.reserved = 0;
	dsakey.blobheader.aiKeyAlg = CALG_DSS_SIGN;
	dsakey.dsspubkeyver3.magic = 0x33535344; // ASCII of "DSS3";
	dsakey.dsspubkeyver3.bitlenP = 1024; // # of bits in prime modulus
	dsakey.dsspubkeyver3.bitlenQ = 160; // # of bits in prime q, 0 if not available
	dsakey.dsspubkeyver3.bitlenJ = 0; // # of bits in (p-1)/q, 0 if not available
	dsakey.dsspubkeyver3.DSSSeed.counter = 0xFFFFFFFF; // not available

	memcpy(dsakey.p, p_pkey.key.p + 2, sizeof(p_pkey.key.p) - 2); std::reverse(dsakey.p, dsakey.p + sizeof(dsakey.p));
	memcpy(dsakey.q, p_pkey.key.q + 2, sizeof(p_pkey.key.q) - 2); std::reverse(dsakey.q, dsakey.q + sizeof(dsakey.q));
	memcpy(dsakey.g, p_pkey.key.g + 2, sizeof(p_pkey.key.g) - 2); std::reverse(dsakey.g, dsakey.g + sizeof(dsakey.g));
	memcpy(dsakey.y, p_pkey.key.y + 2, sizeof(p_pkey.key.y) - 2); std::reverse(dsakey.y, dsakey.y + sizeof(dsakey.y));

	if (CryptImportKey(hCryptProv, (BYTE*)&dsakey, sizeof(dsakey), 0, 0, &hPubKey) == 0)
		return -1;

	unsigned char signature[40];
	memcpy(signature, p_sig.r + 2, 20);
	memcpy(signature + 20, p_sig.s + 2, 20);
	std::reverse(signature, signature + 20);
	std::reverse(signature + 20, signature + 40);
	if (!CryptVerifySignature(hHash, signature, sizeof(signature), hPubKey, nullptr, 0))
	{
		CryptDestroyKey(hPubKey);
		return -1;
	}

	CryptDestroyKey(hPubKey);
	return 0;
}

/*
 * download a public key (the last one) from TortoiseGit server, and parse it
 */
static public_key_t *download_key(const uint8_t *p_longid, const uint8_t *p_signature_issuer)
{
	CString url;
	url.Format(L"http://download.tortoisegit.org/keys/%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X.asc", p_longid[0], p_longid[1], p_longid[2], p_longid[3], p_longid[4], p_longid[5], p_longid[6], p_longid[7]);

	CString tempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	if (URLDownloadToFile(nullptr, url, tempfile, 0, nullptr) != S_OK)
		return nullptr;

	int size = 65536;
	std::unique_ptr<char[]> buffer(new char[size]);
	FILE * pFile = _tfsopen(tempfile, _T("rb"), SH_DENYWR);
	if (pFile)
	{
		int length = 0;
		if ((length = (int)fread(buffer.get(), sizeof(char), size, pFile)) >= 8)
		{
			fclose(pFile);
			size = length;
		}
		else
		{
			fclose(pFile);
			return nullptr;
		}
	}
	else
		return nullptr;

	public_key_t *p_pkey = (public_key_t*) malloc(sizeof(public_key_t));
	if (!p_pkey)
	{
		DeleteUrlCacheEntry(url);
		return nullptr;
	}

	memcpy(p_pkey->longid, p_longid, 8);

	if (parse_public_key((const uint8_t *)buffer.get(), size, p_pkey, p_signature_issuer))
	{
		free(p_pkey);
		return nullptr;
	}

	return p_pkey;
}

int VerifyIntegrity(const CString &filename, const CString &signatureFilename)
{
	signature_packet_t p_sig;
	memset(&p_sig, 0, sizeof(signature_packet_t));
	if (LoadSignature(signatureFilename, &p_sig))
		return -1;

	public_key_t p_pkey;
	memset(&p_pkey, 0, sizeof(public_key_t));
	if (parse_public_key(tortoisegit_public_key, sizeof(tortoisegit_public_key), &p_pkey, nullptr))
	{
		if (p_sig.version == 4)
		{
			free(p_sig.specific.v4.hashed_data);
			free(p_sig.specific.v4.unhashed_data);
		}
		return -1;
	}
	memcpy(p_pkey.longid, tortoisegit_public_key_longid, 8);

	HCRYPTPROV hCryptProv;
	if (!CryptAcquireContext(&hCryptProv, nullptr, nullptr, PROV_DSS, CRYPT_VERIFYCONTEXT))
	{
		if (p_sig.version == 4)
		{
			free(p_sig.specific.v4.hashed_data);
			free(p_sig.specific.v4.unhashed_data);
		}
		free(p_pkey.psz_username);
		return -1;
	}

	if (memcmp(p_sig.issuer_longid, p_pkey.longid, 8) != 0)
	{
		public_key_t *p_new_pkey = download_key(p_sig.issuer_longid, tortoisegit_public_key_longid);
		if (!p_new_pkey)
		{
			if (p_sig.version == 4)
			{
				free(p_sig.specific.v4.hashed_data);
				free(p_sig.specific.v4.unhashed_data);
			}
			free(p_pkey.psz_username);
			CryptReleaseContext(hCryptProv, 0);
			return -1;
		}

		HCRYPTHASH hHash;
		if (!CryptCreateHash(hCryptProv, CALG_SHA1, 0, 0, &hHash))
		{
			if (p_sig.version == 4)
			{
				free(p_sig.specific.v4.hashed_data);
				free(p_sig.specific.v4.unhashed_data);
			}
			free(p_pkey.psz_username);
			CryptReleaseContext(hCryptProv, 0);
			free(p_new_pkey->psz_username);
			if (p_new_pkey->sig.version == 4)
			{
				free(p_new_pkey->sig.specific.v4.hashed_data);
				free(p_new_pkey->sig.specific.v4.unhashed_data);
			}
			free(p_new_pkey);
			return -1;
		}

		if (hash_sha1_from_public_key(hHash, p_new_pkey))
		{
			if (p_sig.version == 4)
			{
				free(p_sig.specific.v4.hashed_data);
				free(p_sig.specific.v4.unhashed_data);
			}
			free(p_pkey.psz_username);
			CryptReleaseContext(hCryptProv, 0);
			free(p_new_pkey->psz_username);
			if (p_new_pkey->sig.version == 4)
			{
				free(p_new_pkey->sig.specific.v4.hashed_data);
				free(p_new_pkey->sig.specific.v4.unhashed_data);
			}
			free(p_new_pkey);
			CryptDestroyHash(hHash);
			return -1;
		}

		if (check_hash(hHash, &p_new_pkey->sig))
		{
			if (p_sig.version == 4)
			{
				free(p_sig.specific.v4.hashed_data);
				free(p_sig.specific.v4.unhashed_data);
			}
			free(p_pkey.psz_username);
			CryptReleaseContext(hCryptProv, 0);
			free(p_new_pkey->psz_username);
			if (p_new_pkey->sig.version == 4)
			{
				free(p_new_pkey->sig.specific.v4.hashed_data);
				free(p_new_pkey->sig.specific.v4.unhashed_data);
			}
			free(p_new_pkey);
			CryptDestroyHash(hHash);
			return -1;
		}

		if (verify_signature(hCryptProv, hHash, p_pkey, p_new_pkey->sig))
		{
			if (p_sig.version == 4)
			{
				free(p_sig.specific.v4.hashed_data);
				free(p_sig.specific.v4.unhashed_data);
			}
			free(p_pkey.psz_username);
			CryptReleaseContext(hCryptProv, 0);
			free(p_new_pkey->psz_username);
			if (p_new_pkey->sig.version == 4)
			{
				free(p_new_pkey->sig.specific.v4.hashed_data);
				free(p_new_pkey->sig.specific.v4.unhashed_data);
			}
			free(p_new_pkey);
			CryptDestroyHash(hHash);
			return -1;
		}
		else
		{
			CryptDestroyHash(hHash);
			free(p_pkey.psz_username);
			p_pkey = *p_new_pkey;
			if (p_pkey.sig.version == 4)
			{
				p_pkey.sig.version = 0;
				free(p_pkey.sig.specific.v4.hashed_data);
				free(p_pkey.sig.specific.v4.unhashed_data);
			}
			free(p_new_pkey);
		}
	}

	int nRetCode = -1;

	HCRYPTHASH hHash;
	if (!CryptCreateHash(hCryptProv, CALG_SHA1, 0, 0, &hHash))
		goto error;

	if (hash_sha1_from_file(hHash, filename, &p_sig))
		goto error;

	if (check_hash(hHash, &p_sig))
		goto error;

	if (verify_signature(hCryptProv, hHash, p_pkey, p_sig))
		goto error;

	nRetCode = 0;

error:
	CryptDestroyHash(hHash);
	CryptReleaseContext(hCryptProv, 0);

	free(p_pkey.psz_username);
	if (p_sig.version == 4)
	{
		free(p_sig.specific.v4.hashed_data);
		free(p_sig.specific.v4.unhashed_data);
	}

	return nRetCode;
}
