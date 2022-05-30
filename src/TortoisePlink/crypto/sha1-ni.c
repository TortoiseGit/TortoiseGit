/*
 * Hardware-accelerated implementation of SHA-1 using x86 SHA-NI.
 */

#include "ssh.h"
#include "sha1.h"

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

static bool sha1_ni_available(void)
{
    unsigned int CPUInfo[4];
    GET_CPU_ID_0(CPUInfo);
    if (CPUInfo[0] < 7)
        return false;

    GET_CPU_ID_7(CPUInfo);
    return CPUInfo[1] & (1 << 29); /* Check SHA */
}

/* SHA1 implementation using new instructions
   The code is based on Jeffrey Walton's SHA1 implementation:
   https://github.com/noloader/SHA-Intrinsics
*/
static inline void sha1_ni_block(__m128i *core, const uint8_t *p)
{
    __m128i ABCD, E0, E1, MSG0, MSG1, MSG2, MSG3;
    const __m128i MASK = _mm_set_epi64x(
        0x0001020304050607ULL, 0x08090a0b0c0d0e0fULL);

    const __m128i *block = (const __m128i *)p;

    /* Load initial values */
    ABCD = core[0];
    E0 = core[1];

    /* Rounds 0-3 */
    MSG0 = _mm_loadu_si128(block);
    MSG0 = _mm_shuffle_epi8(MSG0, MASK);
    E0 = _mm_add_epi32(E0, MSG0);
    E1 = ABCD;
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);

    /* Rounds 4-7 */
    MSG1 = _mm_loadu_si128(block + 1);
    MSG1 = _mm_shuffle_epi8(MSG1, MASK);
    E1 = _mm_sha1nexte_epu32(E1, MSG1);
    E0 = ABCD;
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 0);
    MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);

    /* Rounds 8-11 */
    MSG2 = _mm_loadu_si128(block + 2);
    MSG2 = _mm_shuffle_epi8(MSG2, MASK);
    E0 = _mm_sha1nexte_epu32(E0, MSG2);
    E1 = ABCD;
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);
    MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
    MSG0 = _mm_xor_si128(MSG0, MSG2);

    /* Rounds 12-15 */
    MSG3 = _mm_loadu_si128(block + 3);
    MSG3 = _mm_shuffle_epi8(MSG3, MASK);
    E1 = _mm_sha1nexte_epu32(E1, MSG3);
    E0 = ABCD;
    MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 0);
    MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
    MSG1 = _mm_xor_si128(MSG1, MSG3);

    /* Rounds 16-19 */
    E0 = _mm_sha1nexte_epu32(E0, MSG0);
    E1 = ABCD;
    MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);
    MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
    MSG2 = _mm_xor_si128(MSG2, MSG0);

    /* Rounds 20-23 */
    E1 = _mm_sha1nexte_epu32(E1, MSG1);
    E0 = ABCD;
    MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
    MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
    MSG3 = _mm_xor_si128(MSG3, MSG1);

    /* Rounds 24-27 */
    E0 = _mm_sha1nexte_epu32(E0, MSG2);
    E1 = ABCD;
    MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 1);
    MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
    MSG0 = _mm_xor_si128(MSG0, MSG2);

    /* Rounds 28-31 */
    E1 = _mm_sha1nexte_epu32(E1, MSG3);
    E0 = ABCD;
    MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
    MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
    MSG1 = _mm_xor_si128(MSG1, MSG3);

    /* Rounds 32-35 */
    E0 = _mm_sha1nexte_epu32(E0, MSG0);
    E1 = ABCD;
    MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 1);
    MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
    MSG2 = _mm_xor_si128(MSG2, MSG0);

    /* Rounds 36-39 */
    E1 = _mm_sha1nexte_epu32(E1, MSG1);
    E0 = ABCD;
    MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
    MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
    MSG3 = _mm_xor_si128(MSG3, MSG1);

    /* Rounds 40-43 */
    E0 = _mm_sha1nexte_epu32(E0, MSG2);
    E1 = ABCD;
    MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
    MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
    MSG0 = _mm_xor_si128(MSG0, MSG2);

    /* Rounds 44-47 */
    E1 = _mm_sha1nexte_epu32(E1, MSG3);
    E0 = ABCD;
    MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 2);
    MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
    MSG1 = _mm_xor_si128(MSG1, MSG3);

    /* Rounds 48-51 */
    E0 = _mm_sha1nexte_epu32(E0, MSG0);
    E1 = ABCD;
    MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
    MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
    MSG2 = _mm_xor_si128(MSG2, MSG0);

    /* Rounds 52-55 */
    E1 = _mm_sha1nexte_epu32(E1, MSG1);
    E0 = ABCD;
    MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 2);
    MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
    MSG3 = _mm_xor_si128(MSG3, MSG1);

    /* Rounds 56-59 */
    E0 = _mm_sha1nexte_epu32(E0, MSG2);
    E1 = ABCD;
    MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
    MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
    MSG0 = _mm_xor_si128(MSG0, MSG2);

    /* Rounds 60-63 */
    E1 = _mm_sha1nexte_epu32(E1, MSG3);
    E0 = ABCD;
    MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);
    MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
    MSG1 = _mm_xor_si128(MSG1, MSG3);

    /* Rounds 64-67 */
    E0 = _mm_sha1nexte_epu32(E0, MSG0);
    E1 = ABCD;
    MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 3);
    MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
    MSG2 = _mm_xor_si128(MSG2, MSG0);

    /* Rounds 68-71 */
    E1 = _mm_sha1nexte_epu32(E1, MSG1);
    E0 = ABCD;
    MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);
    MSG3 = _mm_xor_si128(MSG3, MSG1);

    /* Rounds 72-75 */
    E0 = _mm_sha1nexte_epu32(E0, MSG2);
    E1 = ABCD;
    MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
    ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 3);

    /* Rounds 76-79 */
    E1 = _mm_sha1nexte_epu32(E1, MSG3);
    E0 = ABCD;
    ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);

    /* Combine state */
    core[0] = _mm_add_epi32(ABCD, core[0]);
    core[1] = _mm_sha1nexte_epu32(E0, core[1]);
}

typedef struct sha1_ni {
    /*
     * core[0] stores the first four words of the SHA-1 state. core[1]
     * stores just the fifth word, in the vector lane at the highest
     * address.
     */
    __m128i core[2];
    sha1_block blk;
    void *pointer_to_free;
    BinarySink_IMPLEMENTATION;
    ssh_hash hash;
} sha1_ni;

static void sha1_ni_write(BinarySink *bs, const void *vp, size_t len);

static sha1_ni *sha1_ni_alloc(void)
{
    /*
     * The __m128i variables in the context structure need to be
     * 16-byte aligned, but not all malloc implementations that this
     * code has to work with will guarantee to return a 16-byte
     * aligned pointer. So we over-allocate, manually realign the
     * pointer ourselves, and store the original one inside the
     * context so we know how to free it later.
     */
    void *allocation = smalloc(sizeof(sha1_ni) + 15);
    uintptr_t alloc_address = (uintptr_t)allocation;
    uintptr_t aligned_address = (alloc_address + 15) & ~15;
    sha1_ni *s = (sha1_ni *)aligned_address;
    s->pointer_to_free = allocation;
    return s;
}

static ssh_hash *sha1_ni_new(const ssh_hashalg *alg)
{
    const struct sha1_extra *extra = (const struct sha1_extra *)alg->extra;
    if (!check_availability(extra))
        return NULL;

    sha1_ni *s = sha1_ni_alloc();

    s->hash.vt = alg;
    BinarySink_INIT(s, sha1_ni_write);
    BinarySink_DELEGATE_INIT(&s->hash, s);
    return &s->hash;
}

static void sha1_ni_reset(ssh_hash *hash)
{
    sha1_ni *s = container_of(hash, sha1_ni, hash);

    /* Initialise the core vectors in their storage order */
    s->core[0] = _mm_set_epi64x(
        0x67452301efcdab89ULL, 0x98badcfe10325476ULL);
    s->core[1] = _mm_set_epi32(0xc3d2e1f0, 0, 0, 0);

    sha1_block_setup(&s->blk);
}

static void sha1_ni_copyfrom(ssh_hash *hcopy, ssh_hash *horig)
{
    sha1_ni *copy = container_of(hcopy, sha1_ni, hash);
    sha1_ni *orig = container_of(horig, sha1_ni, hash);

    void *ptf_save = copy->pointer_to_free;
    *copy = *orig; /* structure copy */
    copy->pointer_to_free = ptf_save;

    BinarySink_COPIED(copy);
    BinarySink_DELEGATE_INIT(&copy->hash, copy);
}

static void sha1_ni_free(ssh_hash *hash)
{
    sha1_ni *s = container_of(hash, sha1_ni, hash);

    void *ptf = s->pointer_to_free;
    smemclr(s, sizeof(*s));
    sfree(ptf);
}

static void sha1_ni_write(BinarySink *bs, const void *vp, size_t len)
{
    sha1_ni *s = BinarySink_DOWNCAST(bs, sha1_ni);

    while (len > 0)
        if (sha1_block_write(&s->blk, &vp, &len))
            sha1_ni_block(s->core, s->blk.block);
}

static void sha1_ni_digest(ssh_hash *hash, uint8_t *digest)
{
    sha1_ni *s = container_of(hash, sha1_ni, hash);

    sha1_block_pad(&s->blk, BinarySink_UPCAST(s));

    /* Rearrange the first vector into its output order */
    __m128i abcd = _mm_shuffle_epi32(s->core[0], 0x1B);

    /* Byte-swap it into the output endianness */
    const __m128i mask = _mm_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12);
    abcd = _mm_shuffle_epi8(abcd, mask);

    /* And store it */
    _mm_storeu_si128((__m128i *)digest, abcd);

    /* Finally, store the leftover word */
    uint32_t e = _mm_extract_epi32(s->core[1], 3);
    PUT_32BIT_MSB_FIRST(digest + 16, e);
}

SHA1_VTABLE(ni, "SHA-NI accelerated");
