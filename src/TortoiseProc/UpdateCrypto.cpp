// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2019 Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2014-2019, 2021-2023, 2025-2026 TortoiseGit
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
#include <atlenc.h>
#include <span>
#include <optional>
#define NEED_SIGNING_KEY
#include "../version.h"
#include "TempFile.h"
#include "SmartHandle.h"

[[nodiscard]] static bool checked_memcmp(const std::span<const uint8_t> dst, const std::span<const uint8_t> src)
{
	if (src.size() != dst.size())
		return false;
	return std::memcmp(src.data(), dst.data(), src.size()) == 0;
}

[[nodiscard]] static bool checked_memcpy(const std::span<uint8_t> dst, const std::span<const uint8_t> src, bool exactLengthMatch = true)
{
	if (exactLengthMatch && src.size() != dst.size())
		return false;
	else if (src.size() > dst.size())
		return false;

	std::memcpy(dst.data(), src.data(), src.size());
	return true;
}

constexpr auto packet_type(uint8_t c) { return (c & 0x3c) >> 2; } /* 0x3C = 00111100 */

/* number of bytes in a packet header */
constexpr static std::optional<uint32_t> packet_header_len(const uint8_t c)
{
	switch (c & 0x03)
	{
	case 0: return 1;
	case 1: return 2;
	case 2: return 4;
	case 3: return {}; // indeterminate length unsupported
	}
	return {};
}

constexpr static inline std::optional<uint32_t> scalar_number(const std::span<const uint8_t> p)
{
	if (p.size() == 1)
		return p[0];
	else if (p.size() == 2)
		return (p[0] << 8) + p[1];
	else if (p.size() == 4)
		return (static_cast<uint32_t>(p[0]) << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
	return {};
}

/* Bounds-checked cursor over an immutable byte span.
 * All reads return false or empty span on underflow. */
class SpanReader
{
public:
	explicit SpanReader(std::span<const uint8_t> data) : m_buf(data) {}
	SpanReader(const SpanReader&) = delete;
	SpanReader& operator=(const SpanReader&) = delete;

	[[nodiscard]] bool empty() const { return m_buf.empty(); }

	/* Consume and return one byte. Returns false on underflow. */
	[[nodiscard]] bool read_byte(uint8_t& out)
	{
		if (m_buf.empty())
			return false;
		out = m_buf[0];
		m_buf = m_buf.subspan(1);
		return true;
	}

	/* Consume exactly n bytes and return them as a span. Returns empty span on underflow. */
	[[nodiscard]] std::span<const uint8_t> read_bytes(const size_t n)
	{
		if (n > m_buf.size())
			return {};
		auto chunk = m_buf.first(n);
		m_buf = m_buf.subspan(n);
		return chunk;
	}

	/* Consume exactly n bytes and return them as a span. Returns empty span on underflow or when no length is available in the optional. */
	[[nodiscard]] std::span<const uint8_t> read_bytes(const std::optional<uint32_t> n)
	{
		if (!n)
			return {};
		return read_bytes(*n);
	}

	/* Read exactly as many bytes as the destination can hold and copies the data. Returns false on underflow. */
	[[nodiscard]] bool read_bytes(const std::span<uint8_t> dest)
	{
		ASSERT(!dest.empty());
		auto chunk = read_bytes(dest.size());
		if (chunk.empty())
			return false;
		std::memcpy(dest.data(), chunk.data(), dest.size());
		return true;
	}

private:
	std::span<const uint8_t> m_buf;
};

/* number of data bytes in a MPI */
static constexpr std::optional<uint32_t> mpi_len(std::span<const uint8_t, 2> mpi)
{
	return scalar_number(mpi).transform([](const uint32_t value) { return (value + 7) / 8;});
}

[[nodiscard]] static bool read_mpi(mpi_t& dst, SpanReader& r)
{
	if (!r.read_bytes(dst.len))
		return false;

	const auto n = mpi_len(dst.len);
	if (!n)
		return false;

	if (*n > MAX_MPI_BITS_RSA / 8)
		return false;

	const auto bytes = r.read_bytes(n);
	if (bytes.empty())
		return false;

	dst.number.assign_range(bytes);

	return true;
}

/* Base64 decoding */
[[nodiscard]] static size_t b64_decode_binary_to_buffer(std::span<uint8_t> dst, std::string_view src)
{
	if (dst.size() >= INT_MAX || src.size() >= INT_MAX)
		return 0;
	int len = static_cast<int>(dst.size());
	if (!Base64Decode(src.data(), static_cast<int>(src.size()), dst.data(), &len))
		return 0;
	return len;
}

/*
 * crc_octets() was lamely copied from rfc 2440
 * Copyright (C) The Internet Society (1998).  All Rights Reserved.
 */
static constexpr long crc_octets(const std::span<const uint8_t> octets)
{
	constexpr long CRC24_INIT = 0xB704CEL;
	constexpr long CRC24_POLY = 0x1864CFBL;

	long crc = CRC24_INIT;
	for (const auto byte : octets)
	{
		crc ^= byte << 16;
		for (int i = 0; i < 8; ++i)
		{
			crc <<= 1;
			if (crc & 0x1000000)
				crc ^= CRC24_POLY;
		}
	}
	return crc & 0xFFFFFFL;
}

static constexpr std::optional<ALG_ID> map_digestalgo(const uint8_t digest_algo)
{
	switch (digest_algo)
	{
	case DIGEST_ALGO_SHA256:
		return CALG_SHA_256;
	case DIGEST_ALGO_SHA384:
		return CALG_SHA_384;
	case DIGEST_ALGO_SHA512:
		return CALG_SHA_512;
	default:
		return {};
	}
}

static constexpr std::optional<DWORD> map_algo(const uint8_t digest_algo)
{
	switch (digest_algo)
	{
	case PUBLIC_KEY_ALGO_RSA:
		return PROV_RSA_AES; // needed for SHA2, see http://msdn.microsoft.com/en-us/library/windows/desktop/aa387447%28v=vs.85%29.aspx
	default:
		return {};
	}
}

/*
 * fill a signature_packet_v4_t from signature packet data
 * verify that it was used with a RSA public key
 */
[[nodiscard]] static bool parse_signature_v4_packet(signature_packet_t& sig, SpanReader& r)
{
	if (!r.read_byte(sig.type))
		return false;
	if (!r.read_byte(sig.public_key_algo))
		return false;
	if (!r.read_byte(sig.digest_algo))
		return false;
	if (!map_algo(sig.public_key_algo))
		return false;

	/* Hashed data length (2 bytes, big-endian) */
	if (!r.read_bytes(sig.specific.v4.hashed_data_len))
		return false;
	const auto hashed_data = r.read_bytes(scalar_number(sig.specific.v4.hashed_data_len));
	if (hashed_data.empty())
		return false;
	sig.specific.v4.hashed_data.assign_range(hashed_data);

	/* Unhashed data length (2 bytes, big-endian) */
	const auto unhashed_len_bytes = r.read_bytes(2);
	if (unhashed_len_bytes.empty())
		return false;
	const auto unhashed_data = r.read_bytes(scalar_number(unhashed_len_bytes));
	if (unhashed_data.empty())
		return false;

	/* Hash check (2 bytes) */
	if (!r.read_bytes(sig.hash_verification))
		return false;

	/* Walk unhashed subpackets to find the issuer key-id.
	 * Use a fresh SpanReader over the already-copied unhashed block. */
	SpanReader sp(unhashed_data);
	while (!sp.empty())
	{
		/* Decode subpacket length */
		const auto subpacket_len = [&sp]() -> std::optional<uint32_t> {
			uint8_t first;
			if (!sp.read_byte(first))
				return {};

			if (first < 192)
				return first;
			else if (first < 224)
			{
				uint8_t second;
				if (!sp.read_byte(second))
					return {};
				return (static_cast<uint32_t>(first - 192) << 8) + second + 192;
			}
			else if (first == 255) /* first == 0xFF: 4-byte length follows */
			{
				const auto len_bytes = sp.read_bytes(4);
				if (len_bytes.empty())
					return {};
				return (static_cast<uint32_t>(len_bytes[0]) << 24) | (static_cast<uint32_t>(len_bytes[1]) << 16) | (static_cast<uint32_t>(len_bytes[2]) << 8) | static_cast<uint32_t>(len_bytes[3]);
			}
			else
				return {}; // Partial Body Lengths are unsupported
		}();
		const auto subpacket = sp.read_bytes(subpacket_len);
		if (subpacket.empty())
			return false;

		if (subpacket[0] == ISSUER_SUBPACKET)
		{
			/* subpacket body = type byte (1) + 8-byte key-id */
			if (subpacket.size() != 1 + sizeof(sig.issuer_longid))
				return false;

			return checked_memcpy(sig.issuer_longid, subpacket.subspan(1, sizeof(sig.issuer_longid)));
		}
	}

	return false; /* issuer subpacket not found */
}

static std::optional<signature_packet_t> parse_signature_packet(std::span<const uint8_t> packet)
{
	SpanReader r(packet);

	signature_packet_t sig{};
	if (!r.read_byte(sig.version))
		return {};

	switch (sig.version)
	{
		case 4:
			if (!parse_signature_v4_packet(sig, r))
				return {};
			break;
		default:
			return {};
	}

	if (!map_algo(sig.public_key_algo))
		return {};

	if (!map_digestalgo(sig.digest_algo))
		return {};

	switch (sig.type)
	{
		case BINARY_SIGNATURE:
		case TEXT_SIGNATURE:
		case GENERIC_KEY_SIGNATURE:
		case PERSONA_KEY_SIGNATURE:
		case CASUAL_KEY_SIGNATURE:
		case POSITIVE_KEY_SIGNATURE:
			break;
		default:
			return {};
	}

	/* Read the algorithm-specific MPI(s) from what remains */
	switch (sig.public_key_algo)
	{
	case PUBLIC_KEY_ALGO_RSA:
		if (!read_mpi(sig.algo_specific.rsa.s, r))
			return {};
		break;
	default:
		return {};
	}

	if (!r.empty()) /* all bytes must be consumed */
		return {};

	return sig;
}

/*
 * Transform an armored document in binary format
 * Used on public keys and signatures
 */
static std::vector<uint8_t> pgp_unarmor(std::string_view input)
{
	if (input.size() >= INT_MAX)
		return {};
	auto buffer = std::vector<uint8_t>(Base64DecodeGetRequiredLength(static_cast<int>(input.size())));
	auto obuf = std::span<uint8_t>(buffer);

	size_t o_written = 0;
	bool header_skipped = false;
	while (!input.empty() && input.front() != '=')
	{
		if (input.front() == '\r' || input.front() == '\n')
		{
			input.remove_prefix(1);
			continue;
		}

		const size_t len = input.find_first_of("\r\n");
		if (len == std::string_view::npos)
			return {};
		const std::string_view line = input.substr(0, len);

		if (!header_skipped)
		{
			if (line.starts_with("-----BEGIN PGP"))
				header_skipped = true;

			input.remove_prefix(line.size());
			continue;
		}

		if (line.starts_with("Version:"))
		{
			input.remove_prefix(line.size());
			continue;
		}

		{
			const auto written = b64_decode_binary_to_buffer(obuf.subspan(o_written), line);
			if (written == 0)
				return {};
			o_written += written;
		}

		input.remove_prefix(line.size());

		if (line.back() == '=')
			break;
	}
	buffer.resize(o_written);

	// Skip CR/LF before CRC
	while (!input.empty() && (input.front() == '\r' || input.front() == '\n'))
		input.remove_prefix(1);

	/* XXX: the CRC is OPTIONAL, really require it ? */
	// CRC line must start with '='
	if (input.empty() || input.front() != '=')
		return {};
	input.remove_prefix(1);

	// CRC24 = 3 bytes = 4 base64 characters
	if (input.size() < 4)
		return {};

	uint8_t crc[3];
	if (b64_decode_binary_to_buffer(std::span<uint8_t>(crc), input.substr(0, 4)) != 3)
		return {};

	const long crc_actual = crc_octets(buffer);
	const long crc_expected = (static_cast<long>(crc[0]) << 16) | (static_cast<long>(crc[1]) << 8) | static_cast<long>(crc[2]);
	if (crc_expected != crc_actual)
		return {};

	return buffer;
}

/*
 * fill a public_key_packet_t structure from public key packet data
 * verify that it is a version 4 public key packet, using RSA
 */
static std::optional<public_key_packet_t> parse_public_key_packet(std::span<const uint8_t> buf)
{
	SpanReader r(buf);
	public_key_packet_t key{};
	if (!r.read_byte(key.version))
		return {};
	if (key.version != 4)
		return {};

	/* XXX: warn when timestamp is > date ? */
	if (!r.read_bytes(key.timestamp))
		return {};

	if (!r.read_byte(key.algo))
		return {};

	switch (key.algo)
	{
	case PUBLIC_KEY_ALGO_RSA:
		if (!read_mpi(key.sig.rsa.n, r))
			return {};
		if (!read_mpi(key.sig.rsa.e, r))
			return {};
		break;
	default:
		return {};
	}
	if (!r.empty()) /* all bytes must be consumed */
		return {};

	return key;
}

/*
 * fill a public_key_t with public key data, including:
 *   * public key packet
 *   * signature packet issued by key which long id is sig_issuer
 *   * user id packet
 */
static std::optional<public_key_t> parse_public_key(std::span<const uint8_t> key_data, const std::optional<const std::span<const uint8_t, 8>> sig_issuer)
{
	public_key_t public_key{};
	SpanReader r(key_data);
	while (!r.empty())
	{
		/* Each packet starts with a tag byte */
		uint8_t tag_byte;
		if (!r.read_byte(tag_byte))
			return {};
		if (!(tag_byte & 0x80) || (tag_byte & 0x40)) /* must be old-format packet */
			return {};

		const int i_type = packet_type(tag_byte);

		const auto header_bytes = r.read_bytes(packet_header_len(tag_byte));
		if (header_bytes.empty())
			return {};

		const auto packet = r.read_bytes(scalar_number(header_bytes));
		if (packet.empty())
			return {};

		switch (i_type)
		{
			case PUBLIC_KEY_PACKET:
			{
				auto key = parse_public_key_packet(packet);
				if (!key)
					return {};
				public_key.key = std::move(*key);
				break;
			}

			case SIGNATURE_PACKET: /* we accept only v4 signatures here */
				if (public_key.sig || !sig_issuer)
					break;
				if (auto sig = parse_signature_packet(packet); sig)
				{
					if (!checked_memcmp(sig->issuer_longid, *sig_issuer))
						break;
					public_key.sig = std::move(*sig);
				}
				break;

			case USER_ID_PACKET:
				if (public_key.psz_username) /* save only the first User ID */
					break;
				if (packet.empty())
					return {};
				public_key.psz_username = { packet.cbegin(), packet.cend() };
				break;

			default:
				break;
		}
	}

	if (public_key.key.version == 0 || !public_key.psz_username)
		return {};

	if (sig_issuer && !public_key.sig)
		return {};

	return public_key;
}

static std::vector<uint8_t> load_pgp_file(const CString& filename)
{
	CAutoFILE pFile = _wfsopen(filename, L"rb", SH_DENYWR);
	if (!pFile)
		return {};

	constexpr size_t kMaxSigSize = 65536;
	auto buffer = std::vector<uint8_t>(kMaxSigSize);
	const size_t length = fread(buffer.data(), sizeof(uint8_t), kMaxSigSize, pFile);
	if (length < 8 || length == kMaxSigSize || ferror(pFile))
		return {};
	buffer.resize(length);

	/* If the first byte is ASCII (< 0x80), the data is armored — unarmor it first */
	if (!(buffer[0] & 0x80))
	{
		static_assert(sizeof(uint8_t) == sizeof(const char));
		std::string_view armored{ reinterpret_cast<const char*>(buffer.data()), buffer.size() };
		return pgp_unarmor(armored);
	}

	return buffer;
}

static std::optional<signature_packet_t> LoadSignature(const CString& signatureFilename)
{
	auto buffer = load_pgp_file(signatureFilename);
	if (buffer.empty())
		return {};

	/* Parse outer packet envelope using SpanReader */
	SpanReader r(buffer);

	uint8_t tag_byte;
	if (!r.read_byte(tag_byte))
		return {};
	if (!(tag_byte & 0x80) || (tag_byte & 0x40)) /* must be old-format packet */
		return {};

	if (packet_type(tag_byte) != SIGNATURE_PACKET)
		return {};

	const auto header_bytes = r.read_bytes(packet_header_len(tag_byte));
	if (header_bytes.empty())
		return {};

	const auto packet = r.read_bytes(scalar_number(header_bytes));
	if (packet.empty())
		return {};

	if (!r.empty()) /* all bytes must be consumed */
		return {};

	auto sig = parse_signature_packet(packet);
	if (!sig)
		return {};

	if (sig->type != BINARY_SIGNATURE && sig->type != TEXT_SIGNATURE)
		return {};

	return sig;
}

[[nodiscard]] static bool CryptHashChar(HCRYPTHASH hHash, const int c)
{
	return CryptHashData(hHash, reinterpret_cast<const BYTE*>(&c), 1, 0);
}

[[nodiscard]] static bool CryptHashSizeT(HCRYPTHASH hHash, const size_t number)
{
	if (!CryptHashChar(hHash, (number >> 24) & 0xff))
		return false;
	if (!CryptHashChar(hHash, (number >> 16) & 0xff))
		return false;
	if (!CryptHashChar(hHash, (number >> 8) & 0xff))
		return false;
	if (!CryptHashChar(hHash, number & 0xff))
		return false;
	return true;
}

/* final part of the hash */
[[nodiscard]] static bool hash_finish(HCRYPTHASH hHash, const signature_packet_t& sig)
{
	if (sig.version == 4)
	{
		if (!CryptHashChar(hHash, sig.version))
			return false;
		if (!CryptHashChar(hHash, sig.type))
			return false;
		if (!CryptHashChar(hHash, sig.public_key_algo))
			return false;
		if (!CryptHashChar(hHash, sig.digest_algo))
			return false;
		if (!CryptHashData(hHash, sig.specific.v4.hashed_data_len, sizeof(sig.specific.v4.hashed_data_len), 0))
			return false;
		if (sig.specific.v4.hashed_data.empty())
			return false;
		if (sig.specific.v4.hashed_data.size() > DWORD_MAX || !CryptHashData(hHash, sig.specific.v4.hashed_data.data(), static_cast<DWORD>(sig.specific.v4.hashed_data.size()), 0))
			return false;
		size_t i_len = sig.specific.v4.hashed_data.size();

		if (!CryptHashChar(hHash, 0x04))
			return false;
		if (!CryptHashChar(hHash, 0xFF))
			return false;

		i_len += sizeof(sig.version) + sizeof(sig.type) + sizeof(sig.public_key_algo) + sizeof(sig.digest_algo) + sizeof(sig.specific.v4.hashed_data_len);

		return CryptHashSizeT(hHash, i_len);
	}
	else
	{ /* RFC 4880 only tells about versions 3 and 4, and we only support version 4 */
		return false;
	}
}

/*
 * Generate a hash on a public key, to verify a signature made on that hash
 * Note that we need the signature (v4) to compute the hash
 */
[[nodiscard]] static bool hash_from_public_key(HCRYPTHASH hHash, const public_key_t& pkey)
{
	if (!pkey.sig || pkey.sig->version != 4)
		return false;

	if (pkey.sig->type < GENERIC_KEY_SIGNATURE || pkey.sig->type > POSITIVE_KEY_SIGNATURE)
		return false;

	DWORD i_size = 0;

	switch (pkey.key.algo)
	{
	case PUBLIC_KEY_ALGO_RSA:
		i_size = sizeof(pkey.key.version) + sizeof(pkey.key.algo) + sizeof(pkey.key.timestamp) + 2 * sizeof(pkey.key.sig.rsa.n.len) + static_cast<DWORD>(pkey.key.sig.rsa.n.size()) + static_cast<DWORD>(pkey.key.sig.rsa.e.size());
		break;
	default:
		return false;
	}

	if (!CryptHashChar(hHash, 0x99))
		return false;

	if (!CryptHashChar(hHash, (i_size >> 8) & 0xff))
		return false;
	if (!CryptHashChar(hHash, i_size & 0xff))
		return false;

	if (!CryptHashChar(hHash, pkey.key.version))
		return false;
	if (!CryptHashData(hHash, pkey.key.timestamp, sizeof(pkey.key.timestamp), 0))
		return false;
	if (!CryptHashChar(hHash, pkey.key.algo))
		return false;

	switch (pkey.key.algo)
	{
	case PUBLIC_KEY_ALGO_RSA:
		if (!CryptHashData(hHash, pkey.key.sig.rsa.n.len, sizeof(pkey.key.sig.rsa.n.len), 0))
			return false;
		if (pkey.key.sig.rsa.n.size() > MAX_MPI_BITS_RSA / 8 || !CryptHashData(hHash, pkey.key.sig.rsa.n.data(), static_cast<DWORD>(pkey.key.sig.rsa.n.size()), 0))
			return false;
		if (!CryptHashData(hHash, pkey.key.sig.rsa.e.len, sizeof(pkey.key.sig.rsa.e.len), 0))
			return false;
		if (pkey.key.sig.rsa.e.size() > MAX_MPI_BITS_RSA / 8 || !CryptHashData(hHash, pkey.key.sig.rsa.e.data(), static_cast<DWORD>(pkey.key.sig.rsa.e.size()), 0))
			return false;
		break;
	default:
		return false;
	}

	if (!CryptHashChar(hHash, 0xb4))
		return false;

	if (!pkey.psz_username)
		return false;
	if (!CryptHashSizeT(hHash, pkey.psz_username->size()))
		return false;
	if (pkey.psz_username->size() > DWORD_MAX || !CryptHashData(hHash, pkey.psz_username->data(), static_cast<DWORD>(pkey.psz_username->size()), 0))
		return false;

	return hash_finish(hHash, *pkey.sig);
}

[[nodiscard]] static bool hash_from_file(HCRYPTHASH hHash, const CString& filename, const signature_packet_t& sig)
{
	CAutoFILE pFile = _wfsopen(filename, L"rb", SH_DENYWR);
	if (!pFile)
		return false;

	char buf[4096];
	int read = 0;
	bool last_was_cr = false;
	while ((read = static_cast<int>(fread(buf, sizeof(char), sizeof(buf), pFile))) > 0)
	{
		if (sig.type == TEXT_SIGNATURE)
		{
			// just implement very simple approach
			for (size_t i = 0; i < read; ++i)
			{
				const auto c = buf[i];

				if (c == '\n' && !last_was_cr && !CryptHashChar(hHash, '\r'))
					return false;

				if (!CryptHashChar(hHash, c))
					return false;

				last_was_cr = (c == '\r');
			}
			continue;
		}

		if (!CryptHashData(hHash, reinterpret_cast<BYTE*>(buf), read, 0))
			return false;
	}
	if (ferror(pFile))
		return false;

	return hash_finish(hHash, sig);
}

[[nodiscard]] static bool check_hash(HCRYPTHASH hHash, const signature_packet_t& sig)
{
	DWORD hashLen;
	DWORD hashLenLen = sizeof(DWORD);
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashLen), &hashLenLen, 0))
		return false;

	auto pHash = std::make_unique<BYTE[]>(hashLen);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, pHash.get(), &hashLen, 0))
		return false;

	return hashLen >= 2 && pHash[0] == sig.hash_verification[0] && pHash[1] == sig.hash_verification[1];
}

/*
* Verify an OpenPGP signature made with some RSA public key
*/
[[nodiscard]] static bool verify_signature_rsa(HCRYPTPROV hCryptProv, HCRYPTHASH hHash, const public_key_t& pkey, const signature_packet_t& sig)
{
	if (sig.algo_specific.rsa.s.empty() || sig.algo_specific.rsa.s.size() > pkey.key.sig.rsa.n.size() || pkey.key.sig.rsa.n.size() > MAX_MPI_BITS_RSA / 8)
		return false;

	RSAKEY rsakey{};
	rsakey.blobheader.bType = PUBLICKEYBLOB; // 0x06
	rsakey.blobheader.bVersion = CUR_BLOB_VERSION; // 0x02
	rsakey.blobheader.aiKeyAlg = CALG_RSA_KEYX;
	rsakey.rsapubkey.magic = 0x31415352;// ASCII for RSA1
	rsakey.rsapubkey.bitlen = static_cast<DWORD>(pkey.key.sig.rsa.n.size()) * 8;
	if (pkey.key.sig.rsa.e.empty() || pkey.key.sig.rsa.e.size() > sizeof(rsakey.rsapubkey.pubexp)) // generic check that e is not empty and fits into pubexp in principle
		return false;
	rsakey.rsapubkey.pubexp = 65537; // gnupg only uses this
	if (pkey.key.sig.rsa.e.size() != 3 || pkey.key.sig.rsa.e.number[0] != 1 || pkey.key.sig.rsa.e.number[1] != 0 || pkey.key.sig.rsa.e.number[2] != 1) // make sure e is actually 65537
		return false;

	if (!checked_memcpy(rsakey.n, pkey.key.sig.rsa.n.number, false))
		return false;
	std::reverse(rsakey.n, rsakey.n + pkey.key.sig.rsa.n.size());

	HCRYPTKEY hPubKey;
	if (CryptImportKey(hCryptProv, reinterpret_cast<BYTE*>(&rsakey), sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + static_cast<DWORD>(pkey.key.sig.rsa.n.size()), 0, 0, &hPubKey) == 0)
		return false;
	SCOPE_EXIT{ CryptDestroyKey(hPubKey); };

	/* sig.algo_specific.rsa.s.size() might be shorter than pkey.key.sig.rsa.n.size(),
	 * but CrytoAPI requires that both have same length,
	 * thus, use pkey.key.sig.rsa.n.size() as buffer length (pSig; it's safe as pkey.key.sig.rsa.n.size() cannot be longer than the buffer),
	 * but do not copy/reverse NULs at the end of sig.algo_specific.rsa.s into pSig
	 */
	auto pSig = std::make_unique<BYTE[]>(pkey.key.sig.rsa.n.size());
	memcpy(pSig.get(), sig.algo_specific.rsa.s.data(), sig.algo_specific.rsa.s.size());
	std::reverse(pSig.get(), pSig.get() + sig.algo_specific.rsa.s.size());
	return CryptVerifySignature(hHash, pSig.get(), static_cast<DWORD>(pkey.key.sig.rsa.n.size()), hPubKey, nullptr, 0);
}

/*
* Verify an OpenPGP signature made with some public key
*/
[[nodiscard]] static bool verify_signature(HCRYPTPROV hCryptProv, HCRYPTHASH hHash, const public_key_t& key, const signature_packet_t& sign)
{
	if (sign.public_key_algo != key.key.algo)
		return false;

	switch (sign.public_key_algo)
	{
	case PUBLIC_KEY_ALGO_RSA:
		return verify_signature_rsa(hCryptProv, hHash, key, sign);
	default:
		return false;
	}
}

#ifndef GOOGLETEST_INCLUDE_GTEST_GTEST_H_
/*
 * download a public key (the last one) from TortoiseGit server, and parse it
 */
static std::optional<public_key_t> download_key(const std::span<const uint8_t, 8> longid, const std::span<const uint8_t, 8> signature_issuer, CUpdateDownloader* updateDownloader)
{
	ASSERT(updateDownloader);

	CString url;
	url.Format(L"https://download.tortoisegit.org/keys/%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X.asc", longid[0], longid[1], longid[2], longid[3], longid[4], longid[5], longid[6], longid[7]);

	CString tempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	if (updateDownloader->DownloadFile(url, tempfile, false))
		return {};

	const auto buffer = load_pgp_file(tempfile);
	if (buffer.size() < 8)
	{
		DeleteUrlCacheEntry(url);
		return {};
	}

	auto pkey = parse_public_key(buffer, signature_issuer);
	if (!pkey)
		return {};

	if (!checked_memcpy(pkey->longid, longid))
		return {};

	return pkey;
}
#endif

bool VerifyIntegrity(const CString& filename, const CString& signatureFilename, CUpdateDownloader* updateDownloader)
{
	auto sig = LoadSignature(signatureFilename);
	if (!sig)
		return false;

	const auto unpacked_tgit_key = pgp_unarmor(std::string_view(tortoisegit_public_key, sizeof(tortoisegit_public_key)));
	if (unpacked_tgit_key.empty())
		return false;
	auto pkey = parse_public_key(unpacked_tgit_key, {});
	if (!pkey)
		return false;
	if (!checked_memcpy(pkey->longid, tortoisegit_public_key_longid))
		return false;

	HCRYPTPROV hCryptProv;
	if (const auto algo = map_algo(pkey->key.algo); !algo || !CryptAcquireContext(&hCryptProv, nullptr, nullptr, *algo, CRYPT_VERIFYCONTEXT))
		return false;
	SCOPE_EXIT { CryptReleaseContext(hCryptProv, 0); };

	if (!checked_memcmp(sig->issuer_longid, pkey->longid))
	{
		std::optional<public_key_t> new_pkey;
#ifndef GOOGLETEST_INCLUDE_GTEST_GTEST_H_
		if (updateDownloader)
			new_pkey = download_key(sig->issuer_longid, tortoisegit_public_key_longid, updateDownloader);
#else
		UNREFERENCED_PARAMETER(updateDownloader);
#endif
		if (!new_pkey || !new_pkey->sig)
			return false;

		HCRYPTHASH hHash;
		if (const auto algo = map_digestalgo(new_pkey->sig->digest_algo); !algo || !CryptCreateHash(hCryptProv, *algo, 0, 0, &hHash))
			return false;
		SCOPE_EXIT { CryptDestroyHash(hHash); };

		if (!hash_from_public_key(hHash, *new_pkey))
			return false;

		if (!check_hash(hHash, *new_pkey->sig))
			return false;

		if (!verify_signature(hCryptProv, hHash, *pkey, *new_pkey->sig))
			return false;

		pkey = std::move(*new_pkey);
	}

	HCRYPTHASH hHash;
	if (const auto algo = map_digestalgo(sig->digest_algo); !algo || !CryptCreateHash(hCryptProv, *algo, 0, 0, &hHash))
		return false;
	SCOPE_EXIT{ CryptDestroyHash(hHash); };

	if (!hash_from_file(hHash, filename, *sig))
		return false;

	if (!check_hash(hHash, *sig))
		return false;

	return verify_signature(hCryptProv, hHash, *pkey, *sig);
}
