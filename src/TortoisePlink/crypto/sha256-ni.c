/*
 * Hardware-accelerated implementation of SHA-256 using x86 SHA-NI.
 */

#include "ssh.h"
#include "sha256.h"

#include <wmmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#if HAVE_SHAINTRIN_H
#include <shaintrin.h>
#endif

#if defined(__clang__) || defined(__GNUC__)
#include <cpuid.h>
#define GET_CPU_ID_0(out)                               \
    __cpuid(0, (out)[0], (out)[1], (out)[2], (out)[3])
#define GET_CPU_ID_7(out)                                       \
    __cpuid_count(7, 0, (out)[0], (out)[1], (out)[2], (out)[3])
#else
#define GET_CPU_ID_0(out) __cpuid(out, 0)
#define GET_CPU_ID_7(out) __cpuidex(out, 7, 0)
#endif

static bool sha256_ni_available(void)
{
    unsigned int CPUInfo[4];
    GET_CPU_ID_0(CPUInfo);
    if (CPUInfo[0] < 7)
        return false;

    GET_CPU_ID_7(CPUInfo);
    return CPUInfo[1] & (1 << 29); /* Check SHA */
}

/* SHA256 implementation using new instructions
   The code is based on Jeffrey Walton's SHA256 implementation:
   https://github.com/noloader/SHA-Intrinsics
*/
static inline void sha256_ni_block(__m128i *core, const uint8_t *p)
{
    __m128i STATE0, STATE1;
    __m128i MSG, TMP;
    __m128i MSG0, MSG1, MSG2, MSG3;
    const __m128i *block = (const __m128i *)p;
    const __m128i MASK = _mm_set_epi64x(
        0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);

    /* Load initial values */
    STATE0 = core[0];
    STATE1 = core[1];

    /* Rounds 0-3 */
    MSG = _mm_loadu_si128(block);
    MSG0 = _mm_shuffle_epi8(MSG, MASK);
    MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(
                            0xE9B5DBA5B5C0FBCFULL, 0x71374491428A2F98ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

    /* Rounds 4-7 */
    MSG1 = _mm_loadu_si128(block + 1);
    MSG1 = _mm_shuffle_epi8(MSG1, MASK);
    MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(
                            0xAB1C5ED5923F82A4ULL, 0x59F111F13956C25BULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

    /* Rounds 8-11 */
    MSG2 = _mm_loadu_si128(block + 2);
    MSG2 = _mm_shuffle_epi8(MSG2, MASK);
    MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(
                            0x550C7DC3243185BEULL, 0x12835B01D807AA98ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

    /* Rounds 12-15 */
    MSG3 = _mm_loadu_si128(block + 3);
    MSG3 = _mm_shuffle_epi8(MSG3, MASK);
    MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(
                            0xC19BF1749BDC06A7ULL, 0x80DEB1FE72BE5D74ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
    MSG0 = _mm_add_epi32(MSG0, TMP);
    MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

    /* Rounds 16-19 */
    MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(
                            0x240CA1CC0FC19DC6ULL, 0xEFBE4786E49B69C1ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
    MSG1 = _mm_add_epi32(MSG1, TMP);
    MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

    /* Rounds 20-23 */
    MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(
                            0x76F988DA5CB0A9DCULL, 0x4A7484AA2DE92C6FULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
    MSG2 = _mm_add_epi32(MSG2, TMP);
    MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

    /* Rounds 24-27 */
    MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(
                            0xBF597FC7B00327C8ULL, 0xA831C66D983E5152ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
    MSG3 = _mm_add_epi32(MSG3, TMP);
    MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

    /* Rounds 28-31 */
    MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(
                            0x1429296706CA6351ULL,  0xD5A79147C6E00BF3ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
    MSG0 = _mm_add_epi32(MSG0, TMP);
    MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

    /* Rounds 32-35 */
    MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(
                            0x53380D134D2C6DFCULL, 0x2E1B213827B70A85ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
    MSG1 = _mm_add_epi32(MSG1, TMP);
    MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

    /* Rounds 36-39 */
    MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(
                            0x92722C8581C2C92EULL, 0x766A0ABB650A7354ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
    MSG2 = _mm_add_epi32(MSG2, TMP);
    MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

    /* Rounds 40-43 */
    MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(
                            0xC76C51A3C24B8B70ULL, 0xA81A664BA2BFE8A1ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
    MSG3 = _mm_add_epi32(MSG3, TMP);
    MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

    /* Rounds 44-47 */
    MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(
                            0x106AA070F40E3585ULL, 0xD6990624D192E819ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
    MSG0 = _mm_add_epi32(MSG0, TMP);
    MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

    /* Rounds 48-51 */
    MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(
                            0x34B0BCB52748774CULL, 0x1E376C0819A4C116ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
    MSG1 = _mm_add_epi32(MSG1, TMP);
    MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

    /* Rounds 52-55 */
    MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(
                            0x682E6FF35B9CCA4FULL, 0x4ED8AA4A391C0CB3ULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
    MSG2 = _mm_add_epi32(MSG2, TMP);
    MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

    /* Rounds 56-59 */
    MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(
                            0x8CC7020884C87814ULL, 0x78A5636F748F82EEULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
    MSG3 = _mm_add_epi32(MSG3, TMP);
    MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

    /* Rounds 60-63 */
    MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(
                            0xC67178F2BEF9A3F7ULL, 0xA4506CEB90BEFFFAULL));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    MSG = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

    /* Combine state */
    core[0] = _mm_add_epi32(STATE0, core[0]);
    core[1] = _mm_add_epi32(STATE1, core[1]);
}

typedef struct sha256_ni {
    /*
     * These two vectors store the 8 words of the SHA-256 state, but
     * not in the same order they appear in the spec: the first word
     * holds A,B,E,F and the second word C,D,G,H.
     */
    __m128i core[2];
    sha256_block blk;
    void *pointer_to_free;
    BinarySink_IMPLEMENTATION;
    ssh_hash hash;
} sha256_ni;

static void sha256_ni_write(BinarySink *bs, const void *vp, size_t len);

static sha256_ni *sha256_ni_alloc(void)
{
    /*
     * The __m128i variables in the context structure need to be
     * 16-byte aligned, but not all malloc implementations that this
     * code has to work with will guarantee to return a 16-byte
     * aligned pointer. So we over-allocate, manually realign the
     * pointer ourselves, and store the original one inside the
     * context so we know how to free it later.
     */
    void *allocation = smalloc(sizeof(sha256_ni) + 15);
    uintptr_t alloc_address = (uintptr_t)allocation;
    uintptr_t aligned_address = (alloc_address + 15) & ~15;
    sha256_ni *s = (sha256_ni *)aligned_address;
    s->pointer_to_free = allocation;
    return s;
}

static ssh_hash *sha256_ni_new(const ssh_hashalg *alg)
{
    const struct sha256_extra *extra = (const struct sha256_extra *)alg->extra;
    if (!check_availability(extra))
        return NULL;

    sha256_ni *s = sha256_ni_alloc();

    s->hash.vt = alg;
    BinarySink_INIT(s, sha256_ni_write);
    BinarySink_DELEGATE_INIT(&s->hash, s);

    return &s->hash;
}

static void sha256_ni_reset(ssh_hash *hash)
{
    sha256_ni *s = container_of(hash, sha256_ni, hash);

    /* Initialise the core vectors in their storage order */
    s->core[0] = _mm_set_epi64x(
        0x6a09e667bb67ae85ULL, 0x510e527f9b05688cULL);
    s->core[1] = _mm_set_epi64x(
        0x3c6ef372a54ff53aULL, 0x1f83d9ab5be0cd19ULL);

    sha256_block_setup(&s->blk);
}

static void sha256_ni_copyfrom(ssh_hash *hcopy, ssh_hash *horig)
{
    sha256_ni *copy = container_of(hcopy, sha256_ni, hash);
    sha256_ni *orig = container_of(horig, sha256_ni, hash);

    void *ptf_save = copy->pointer_to_free;
    *copy = *orig; /* structure copy */
    copy->pointer_to_free = ptf_save;

    BinarySink_COPIED(copy);
    BinarySink_DELEGATE_INIT(&copy->hash, copy);
}

static void sha256_ni_free(ssh_hash *hash)
{
    sha256_ni *s = container_of(hash, sha256_ni, hash);

    void *ptf = s->pointer_to_free;
    smemclr(s, sizeof(*s));
    sfree(ptf);
}

static void sha256_ni_write(BinarySink *bs, const void *vp, size_t len)
{
    sha256_ni *s = BinarySink_DOWNCAST(bs, sha256_ni);

    while (len > 0)
        if (sha256_block_write(&s->blk, &vp, &len))
            sha256_ni_block(s->core, s->blk.block);
}

static void sha256_ni_digest(ssh_hash *hash, uint8_t *digest)
{
    sha256_ni *s = container_of(hash, sha256_ni, hash);

    sha256_block_pad(&s->blk, BinarySink_UPCAST(s));

    /* Rearrange the words into the output order */
    __m128i feba = _mm_shuffle_epi32(s->core[0], 0x1B);
    __m128i dchg = _mm_shuffle_epi32(s->core[1], 0xB1);
    __m128i dcba = _mm_blend_epi16(feba, dchg, 0xF0);
    __m128i hgfe = _mm_alignr_epi8(dchg, feba, 8);

    /* Byte-swap them into the output endianness */
    const __m128i mask = _mm_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12);
    dcba = _mm_shuffle_epi8(dcba, mask);
    hgfe = _mm_shuffle_epi8(hgfe, mask);

    /* And store them */
    __m128i *output = (__m128i *)digest;
    _mm_storeu_si128(output, dcba);
    _mm_storeu_si128(output+1, hgfe);
}

SHA256_VTABLE(ni, "SHA-NI accelerated");
