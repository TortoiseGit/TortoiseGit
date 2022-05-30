/*
 * Hardware-accelerated implementation of SHA-512 using Arm NEON.
 */

#include "ssh.h"
#include "sha512.h"

#if USE_ARM64_NEON_H
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif

static bool sha512_neon_available(void)
{
    /*
     * For Arm, we delegate to a per-platform detection function (see
     * explanation in aes-neon.c).
     */
    return platform_sha512_neon_available();
}

#if !HAVE_NEON_SHA512_INTRINSICS
/*
 * clang 12 and before do not provide the SHA-512 NEON intrinsics, but
 * do provide assembler support for the underlying instructions. So I
 * define the intrinsic functions myself, using inline assembler.
 */
static inline uint64x2_t vsha512su0q_u64(uint64x2_t x, uint64x2_t y)
{
    __asm__("sha512su0 %0.2D,%1.2D" : "+w" (x) : "w" (y));
    return x;
}
static inline uint64x2_t vsha512su1q_u64(uint64x2_t x, uint64x2_t y,
                                         uint64x2_t z)
{
    __asm__("sha512su1 %0.2D,%1.2D,%2.2D" : "+w" (x) : "w" (y), "w" (z));
    return x;
}
static inline uint64x2_t vsha512hq_u64(uint64x2_t x, uint64x2_t y,
                                       uint64x2_t z)
{
    __asm__("sha512h %0,%1,%2.2D" : "+w" (x) : "w" (y), "w" (z));
    return x;
}
static inline uint64x2_t vsha512h2q_u64(uint64x2_t x, uint64x2_t y,
                                        uint64x2_t z)
{
    __asm__("sha512h2 %0,%1,%2.2D" : "+w" (x) : "w" (y), "w" (z));
    return x;
}
#endif /* HAVE_NEON_SHA512_INTRINSICS */

typedef struct sha512_neon_core sha512_neon_core;
struct sha512_neon_core {
    uint64x2_t ab, cd, ef, gh;
};

static inline uint64x2_t sha512_neon_load_input(const uint8_t *p)
{
    return vreinterpretq_u64_u8(vrev64q_u8(vld1q_u8(p)));
}

static inline uint64x2_t sha512_neon_schedule_update(
    uint64x2_t m8, uint64x2_t m7, uint64x2_t m4, uint64x2_t m3, uint64x2_t m1)
{
    /*
     * vsha512su0q_u64() takes words from a long way back in the
     * schedule and performs the sigma_0 half of the computation of
     * the next two 64-bit message-schedule words.
     *
     * vsha512su1q_u64() combines the result of that with the sigma_1
     * steps, to output the finished version of those two words. The
     * total amount of input data it requires fits nicely into three
     * 128-bit vector registers, but one of those registers is
     * misaligned compared to the 128-bit chunks that the message
     * schedule is stored in. So we use vextq_u64 to make one of its
     * input words out of the second half of m4 and the first half of
     * m3.
     */
    return vsha512su1q_u64(vsha512su0q_u64(m8, m7), m1, vextq_u64(m4, m3, 1));
}

static inline void sha512_neon_round2(
    unsigned round_index, uint64x2_t schedule_words,
    uint64x2_t *ab, uint64x2_t *cd, uint64x2_t *ef, uint64x2_t *gh)
{
    /*
     * vsha512hq_u64 performs the Sigma_1 and Ch half of the
     * computation of two rounds of SHA-512 (including feeding back
     * one of the outputs from the first of those half-rounds into the
     * second one).
     *
     * vsha512h2q_u64 combines the result of that with the Sigma_0 and
     * Maj steps, and outputs one 128-bit vector that replaces the gh
     * piece of the input hash state, and a second that updates cd by
     * addition.
     *
     * Similarly to vsha512su1q_u64 above, some of the input registers
     * expected by these instructions are misaligned by 64 bits
     * relative to the chunks we've divided the hash state into, so we
     * have to start by making 'de' and 'fg' words out of our input
     * cd,ef,gh, using vextq_u64.
     *
     * Also, one of the inputs to vsha512hq_u64 is expected to contain
     * the results of summing gh + two round constants + two words of
     * message schedule, but the two words of the message schedule
     * have to be the opposite way round in the vector register from
     * the way that vsha512su1q_u64 output them. Hence, there's
     * another vextq_u64 in here that swaps the two halves of the
     * initial_sum vector register.
     *
     * (This also means that I don't have to prepare a specially
     * reordered version of the sha512_round_constants[] array: as
     * long as I'm unavoidably doing a swap at run time _anyway_, I
     * can load from the normally ordered version of that array, and
     * just take care to fold in that data _before_ the swap rather
     * than after.)
     */

    /* Load two round constants, with the first one in the low half */
    uint64x2_t round_constants = vld1q_u64(
        sha512_round_constants + round_index);

    /* Add schedule words to round constants */
    uint64x2_t initial_sum = vaddq_u64(schedule_words, round_constants);

    /* Swap that sum around so the word used in the first of the two
     * rounds is in the _high_ half of the vector, matching where h
     * lives in the gh vector */
    uint64x2_t swapped_initial_sum = vextq_u64(initial_sum, initial_sum, 1);

    /* Add gh to that, now that they're matching ways round */
    uint64x2_t sum = vaddq_u64(swapped_initial_sum, *gh);

    /* Make the misaligned de and fg words */
    uint64x2_t de = vextq_u64(*cd, *ef, 1);
    uint64x2_t fg = vextq_u64(*ef, *gh, 1);

    /* Now we're ready to put all the pieces together. The output from
     * vsha512h2q_u64 can be used directly as the new gh, and the
     * output from vsha512hq_u64 is simultaneously the intermediate
     * value passed to h2 and the thing you have to add on to cd. */
    uint64x2_t intermed = vsha512hq_u64(sum, fg, de);
    *gh = vsha512h2q_u64(intermed, *cd, *ab);
    *cd = vaddq_u64(*cd, intermed);
}

static inline void sha512_neon_block(sha512_neon_core *core, const uint8_t *p)
{
    uint64x2_t s0, s1, s2, s3, s4, s5, s6, s7;

    uint64x2_t ab = core->ab, cd = core->cd, ef = core->ef, gh = core->gh;

    s0 = sha512_neon_load_input(p + 16*0);
    sha512_neon_round2(0, s0, &ab, &cd, &ef, &gh);
    s1 = sha512_neon_load_input(p + 16*1);
    sha512_neon_round2(2, s1, &gh, &ab, &cd, &ef);
    s2 = sha512_neon_load_input(p + 16*2);
    sha512_neon_round2(4, s2, &ef, &gh, &ab, &cd);
    s3 = sha512_neon_load_input(p + 16*3);
    sha512_neon_round2(6, s3, &cd, &ef, &gh, &ab);
    s4 = sha512_neon_load_input(p + 16*4);
    sha512_neon_round2(8, s4, &ab, &cd, &ef, &gh);
    s5 = sha512_neon_load_input(p + 16*5);
    sha512_neon_round2(10, s5, &gh, &ab, &cd, &ef);
    s6 = sha512_neon_load_input(p + 16*6);
    sha512_neon_round2(12, s6, &ef, &gh, &ab, &cd);
    s7 = sha512_neon_load_input(p + 16*7);
    sha512_neon_round2(14, s7, &cd, &ef, &gh, &ab);
    s0 = sha512_neon_schedule_update(s0, s1, s4, s5, s7);
    sha512_neon_round2(16, s0, &ab, &cd, &ef, &gh);
    s1 = sha512_neon_schedule_update(s1, s2, s5, s6, s0);
    sha512_neon_round2(18, s1, &gh, &ab, &cd, &ef);
    s2 = sha512_neon_schedule_update(s2, s3, s6, s7, s1);
    sha512_neon_round2(20, s2, &ef, &gh, &ab, &cd);
    s3 = sha512_neon_schedule_update(s3, s4, s7, s0, s2);
    sha512_neon_round2(22, s3, &cd, &ef, &gh, &ab);
    s4 = sha512_neon_schedule_update(s4, s5, s0, s1, s3);
    sha512_neon_round2(24, s4, &ab, &cd, &ef, &gh);
    s5 = sha512_neon_schedule_update(s5, s6, s1, s2, s4);
    sha512_neon_round2(26, s5, &gh, &ab, &cd, &ef);
    s6 = sha512_neon_schedule_update(s6, s7, s2, s3, s5);
    sha512_neon_round2(28, s6, &ef, &gh, &ab, &cd);
    s7 = sha512_neon_schedule_update(s7, s0, s3, s4, s6);
    sha512_neon_round2(30, s7, &cd, &ef, &gh, &ab);
    s0 = sha512_neon_schedule_update(s0, s1, s4, s5, s7);
    sha512_neon_round2(32, s0, &ab, &cd, &ef, &gh);
    s1 = sha512_neon_schedule_update(s1, s2, s5, s6, s0);
    sha512_neon_round2(34, s1, &gh, &ab, &cd, &ef);
    s2 = sha512_neon_schedule_update(s2, s3, s6, s7, s1);
    sha512_neon_round2(36, s2, &ef, &gh, &ab, &cd);
    s3 = sha512_neon_schedule_update(s3, s4, s7, s0, s2);
    sha512_neon_round2(38, s3, &cd, &ef, &gh, &ab);
    s4 = sha512_neon_schedule_update(s4, s5, s0, s1, s3);
    sha512_neon_round2(40, s4, &ab, &cd, &ef, &gh);
    s5 = sha512_neon_schedule_update(s5, s6, s1, s2, s4);
    sha512_neon_round2(42, s5, &gh, &ab, &cd, &ef);
    s6 = sha512_neon_schedule_update(s6, s7, s2, s3, s5);
    sha512_neon_round2(44, s6, &ef, &gh, &ab, &cd);
    s7 = sha512_neon_schedule_update(s7, s0, s3, s4, s6);
    sha512_neon_round2(46, s7, &cd, &ef, &gh, &ab);
    s0 = sha512_neon_schedule_update(s0, s1, s4, s5, s7);
    sha512_neon_round2(48, s0, &ab, &cd, &ef, &gh);
    s1 = sha512_neon_schedule_update(s1, s2, s5, s6, s0);
    sha512_neon_round2(50, s1, &gh, &ab, &cd, &ef);
    s2 = sha512_neon_schedule_update(s2, s3, s6, s7, s1);
    sha512_neon_round2(52, s2, &ef, &gh, &ab, &cd);
    s3 = sha512_neon_schedule_update(s3, s4, s7, s0, s2);
    sha512_neon_round2(54, s3, &cd, &ef, &gh, &ab);
    s4 = sha512_neon_schedule_update(s4, s5, s0, s1, s3);
    sha512_neon_round2(56, s4, &ab, &cd, &ef, &gh);
    s5 = sha512_neon_schedule_update(s5, s6, s1, s2, s4);
    sha512_neon_round2(58, s5, &gh, &ab, &cd, &ef);
    s6 = sha512_neon_schedule_update(s6, s7, s2, s3, s5);
    sha512_neon_round2(60, s6, &ef, &gh, &ab, &cd);
    s7 = sha512_neon_schedule_update(s7, s0, s3, s4, s6);
    sha512_neon_round2(62, s7, &cd, &ef, &gh, &ab);
    s0 = sha512_neon_schedule_update(s0, s1, s4, s5, s7);
    sha512_neon_round2(64, s0, &ab, &cd, &ef, &gh);
    s1 = sha512_neon_schedule_update(s1, s2, s5, s6, s0);
    sha512_neon_round2(66, s1, &gh, &ab, &cd, &ef);
    s2 = sha512_neon_schedule_update(s2, s3, s6, s7, s1);
    sha512_neon_round2(68, s2, &ef, &gh, &ab, &cd);
    s3 = sha512_neon_schedule_update(s3, s4, s7, s0, s2);
    sha512_neon_round2(70, s3, &cd, &ef, &gh, &ab);
    s4 = sha512_neon_schedule_update(s4, s5, s0, s1, s3);
    sha512_neon_round2(72, s4, &ab, &cd, &ef, &gh);
    s5 = sha512_neon_schedule_update(s5, s6, s1, s2, s4);
    sha512_neon_round2(74, s5, &gh, &ab, &cd, &ef);
    s6 = sha512_neon_schedule_update(s6, s7, s2, s3, s5);
    sha512_neon_round2(76, s6, &ef, &gh, &ab, &cd);
    s7 = sha512_neon_schedule_update(s7, s0, s3, s4, s6);
    sha512_neon_round2(78, s7, &cd, &ef, &gh, &ab);

    core->ab = vaddq_u64(core->ab, ab);
    core->cd = vaddq_u64(core->cd, cd);
    core->ef = vaddq_u64(core->ef, ef);
    core->gh = vaddq_u64(core->gh, gh);
}

typedef struct sha512_neon {
    sha512_neon_core core;
    sha512_block blk;
    BinarySink_IMPLEMENTATION;
    ssh_hash hash;
} sha512_neon;

static void sha512_neon_write(BinarySink *bs, const void *vp, size_t len);

static ssh_hash *sha512_neon_new(const ssh_hashalg *alg)
{
    const struct sha512_extra *extra = (const struct sha512_extra *)alg->extra;
    if (!check_availability(extra))
        return NULL;

    sha512_neon *s = snew(sha512_neon);

    s->hash.vt = alg;
    BinarySink_INIT(s, sha512_neon_write);
    BinarySink_DELEGATE_INIT(&s->hash, s);
    return &s->hash;
}

static void sha512_neon_reset(ssh_hash *hash)
{
    sha512_neon *s = container_of(hash, sha512_neon, hash);
    const struct sha512_extra *extra =
        (const struct sha512_extra *)hash->vt->extra;

    s->core.ab = vld1q_u64(extra->initial_state);
    s->core.cd = vld1q_u64(extra->initial_state+2);
    s->core.ef = vld1q_u64(extra->initial_state+4);
    s->core.gh = vld1q_u64(extra->initial_state+6);

    sha512_block_setup(&s->blk);
}

static void sha512_neon_copyfrom(ssh_hash *hcopy, ssh_hash *horig)
{
    sha512_neon *copy = container_of(hcopy, sha512_neon, hash);
    sha512_neon *orig = container_of(horig, sha512_neon, hash);

    *copy = *orig; /* structure copy */

    BinarySink_COPIED(copy);
    BinarySink_DELEGATE_INIT(&copy->hash, copy);
}

static void sha512_neon_free(ssh_hash *hash)
{
    sha512_neon *s = container_of(hash, sha512_neon, hash);
    smemclr(s, sizeof(*s));
    sfree(s);
}

static void sha512_neon_write(BinarySink *bs, const void *vp, size_t len)
{
    sha512_neon *s = BinarySink_DOWNCAST(bs, sha512_neon);

    while (len > 0)
        if (sha512_block_write(&s->blk, &vp, &len))
            sha512_neon_block(&s->core, s->blk.block);
}

static void sha512_neon_digest(ssh_hash *hash, uint8_t *digest)
{
    sha512_neon *s = container_of(hash, sha512_neon, hash);

    sha512_block_pad(&s->blk, BinarySink_UPCAST(s));

    vst1q_u8(digest,    vrev64q_u8(vreinterpretq_u8_u64(s->core.ab)));
    vst1q_u8(digest+16, vrev64q_u8(vreinterpretq_u8_u64(s->core.cd)));
    vst1q_u8(digest+32, vrev64q_u8(vreinterpretq_u8_u64(s->core.ef)));
    vst1q_u8(digest+48, vrev64q_u8(vreinterpretq_u8_u64(s->core.gh)));
}

static void sha384_neon_digest(ssh_hash *hash, uint8_t *digest)
{
    sha512_neon *s = container_of(hash, sha512_neon, hash);

    sha512_block_pad(&s->blk, BinarySink_UPCAST(s));

    vst1q_u8(digest,    vrev64q_u8(vreinterpretq_u8_u64(s->core.ab)));
    vst1q_u8(digest+16, vrev64q_u8(vreinterpretq_u8_u64(s->core.cd)));
    vst1q_u8(digest+32, vrev64q_u8(vreinterpretq_u8_u64(s->core.ef)));
}

SHA512_VTABLES(neon, "NEON accelerated");
