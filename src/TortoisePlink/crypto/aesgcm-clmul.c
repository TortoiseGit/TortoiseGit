/*
 * Implementation of the GCM polynomial hash using the x86 CLMUL
 * extension, which provides 64x64->128 polynomial multiplication (or
 * 'carry-less', which is what the CL stands for).
 *
 * Follows the reference implementation in aesgcm-ref-poly.c; see
 * there for comments on the underlying technique. Here the comments
 * just discuss the x86-specific details.
 */

#include <wmmintrin.h>
#include <tmmintrin.h>

#if defined(__clang__) || defined(__GNUC__)
#include <cpuid.h>
#define GET_CPU_ID(out) __cpuid(1, (out)[0], (out)[1], (out)[2], (out)[3])
#else
#define GET_CPU_ID(out) __cpuid(out, 1)
#endif

#include "ssh.h"
#include "aesgcm.h"

typedef struct aesgcm_clmul {
    AESGCM_COMMON_FIELDS;
    __m128i var, acc, mask;
    void *ptr_to_free;
} aesgcm_clmul;

static bool aesgcm_clmul_available(void)
{
    /*
     * Determine if CLMUL is available on this CPU.
     */
    unsigned int CPUInfo[4];
    GET_CPU_ID(CPUInfo);
    return (CPUInfo[2] & (1 << 1));
}

/*
 * __m128i has to be aligned to 16 bytes, and x86 mallocs may not
 * guarantee that, so we must over-allocate to make sure a large
 * enough 16-byte region can be found, and ensure the aesgcm_clmul
 * struct pointer is at least that well aligned.
 */
#define SPECIAL_ALLOC
static aesgcm_clmul *aesgcm_clmul_alloc(void)
{
    char *p = smalloc(sizeof(aesgcm_clmul) + 15);
    uintptr_t ip = (uintptr_t)p;
    ip = (ip + 15) & ~15;
    aesgcm_clmul *ctx = (aesgcm_clmul *)ip;
    memset(ctx, 0, sizeof(aesgcm_clmul));
    ctx->ptr_to_free = p;
    return ctx;
}

#define SPECIAL_FREE
static void aesgcm_clmul_free(aesgcm_clmul *ctx)
{
    void *ptf = ctx->ptr_to_free;
    smemclr(ctx, sizeof(*ctx));
    sfree(ptf);
}

/* Helper function to reverse the 16 bytes in a 128-bit vector */
static inline __m128i mm_byteswap(__m128i vec)
{
    const __m128i reverse = _mm_set_epi64x(
        0x0001020304050607ULL, 0x08090a0b0c0d0e0fULL);
    return _mm_shuffle_epi8(vec, reverse);
}

/* Helper function to swap the two 64-bit words in a 128-bit vector */
static inline __m128i mm_wordswap(__m128i vec)
{
    return _mm_shuffle_epi32(vec, 0x4E);
}

/* Load and store a 128-bit vector in big-endian fashion */
static inline __m128i mm_load_be(const void *p)
{
    return mm_byteswap(_mm_loadu_si128(p));
}
static inline void mm_store_be(void *p, __m128i vec)
{
    _mm_storeu_si128(p, mm_byteswap(vec));
}

/*
 * Key setup is just like in aesgcm-ref-poly.c. There's no point using
 * vector registers to accelerate this, because it happens rarely.
 */
static void aesgcm_clmul_setkey_impl(aesgcm_clmul *ctx,
                                     const unsigned char *var)
{
    uint64_t hi = GET_64BIT_MSB_FIRST(var);
    uint64_t lo = GET_64BIT_MSB_FIRST(var + 8);

    uint64_t bit = 1 & (hi >> 63);
    hi = (hi << 1) ^ (lo >> 63);
    lo = (lo << 1) ^ bit;
    hi ^= 0xC200000000000000 & -bit;

    ctx->var = _mm_set_epi64x(hi, lo);
}

static inline void aesgcm_clmul_setup(aesgcm_clmul *ctx,
                                      const unsigned char *mask)
{
    ctx->mask = mm_load_be(mask);
    ctx->acc = _mm_set_epi64x(0, 0);
}

/*
 * Folding a coefficient into the accumulator is done by essentially
 * the algorithm in aesgcm-ref-poly.c. I don't speak these intrinsics
 * all that well, so in the parts where I needed to XOR half of one
 * vector into half of another, I did a lot of faffing about with
 * masks like 0xFFFFFFFFFFFFFFFF0000000000000000. Very likely this can
 * be streamlined by a better x86-speaker than me. Patches welcome.
 */
static inline void aesgcm_clmul_coeff(aesgcm_clmul *ctx,
                                      const unsigned char *coeff)
{
    ctx->acc = _mm_xor_si128(ctx->acc, mm_load_be(coeff));

    /* Compute ah^al and bh^bl by word-swapping each of a and b and
     * XORing with the original. That does more work than necessary -
     * you end up with each of the desired values repeated twice -
     * but I don't know of a neater way. */
    __m128i aswap = mm_wordswap(ctx->acc);
    __m128i vswap = mm_wordswap(ctx->var);
    aswap = _mm_xor_si128(ctx->acc, aswap);
    vswap = _mm_xor_si128(ctx->var, vswap);

    /* Do the three multiplications required by Karatsuba */
    __m128i md = _mm_clmulepi64_si128(aswap, vswap, 0x00);
    __m128i lo = _mm_clmulepi64_si128(ctx->acc, ctx->var, 0x00);
    __m128i hi = _mm_clmulepi64_si128(ctx->acc, ctx->var, 0x11);
    /* Combine lo and hi into md */
    md = _mm_xor_si128(md, lo);
    md = _mm_xor_si128(md, hi);

    /* Now we must XOR the high half of md into the low half of hi,
     * and the low half of md into the high half of hi. Simplest thing
     * is to swap the words of md (so that each one lines up with the
     * register it's going to end up in), and then mask one off in
     * each case. */
    md = mm_wordswap(md);
    lo = _mm_xor_si128(lo, _mm_and_si128(md, _mm_set_epi64x(~0ULL, 0ULL)));
    hi = _mm_xor_si128(hi, _mm_and_si128(md, _mm_set_epi64x(0ULL, ~0ULL)));

    /* The reduction stage is transformed similarly from the version
     * in aesgcm-ref-poly.c. */
    __m128i r1 = _mm_clmulepi64_si128(_mm_set_epi64x(0, 0xC200000000000000),
                                     lo, 0x00);
    r1 = mm_wordswap(r1);
    r1 = _mm_xor_si128(r1, lo);
    hi = _mm_xor_si128(hi, _mm_and_si128(r1, _mm_set_epi64x(~0ULL, 0ULL)));

    __m128i r2 = _mm_clmulepi64_si128(_mm_set_epi64x(0, 0xC200000000000000),
                                     r1, 0x10);
    hi = _mm_xor_si128(hi, r2);
    hi = _mm_xor_si128(hi, _mm_and_si128(r1, _mm_set_epi64x(0ULL, ~0ULL)));

    ctx->acc = hi;
}

static inline void aesgcm_clmul_output(aesgcm_clmul *ctx,
                                       unsigned char *output)
{
    mm_store_be(output, _mm_xor_si128(ctx->acc, ctx->mask));
    smemclr(&ctx->acc, 16);
    smemclr(&ctx->mask, 16);
}

#define AESGCM_FLAVOUR clmul
#define AESGCM_NAME "CLMUL accelerated"
#include "aesgcm-footer.h"
