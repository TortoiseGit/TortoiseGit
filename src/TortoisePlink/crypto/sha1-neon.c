/*
 * Hardware-accelerated implementation of SHA-1 using Arm NEON.
 */

#include "ssh.h"
#include "sha1.h"

#if USE_ARM64_NEON_H
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif

static bool sha1_neon_available(void)
{
    /*
     * For Arm, we delegate to a per-platform detection function (see
     * explanation in aes-neon.c).
     */
    return platform_sha1_neon_available();
}

typedef struct sha1_neon_core sha1_neon_core;
struct sha1_neon_core {
    uint32x4_t abcd;
    uint32_t e;
};

static inline uint32x4_t sha1_neon_load_input(const uint8_t *p)
{
    return vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(p)));
}

static inline uint32x4_t sha1_neon_schedule_update(
    uint32x4_t m4, uint32x4_t m3, uint32x4_t m2, uint32x4_t m1)
{
    return vsha1su1q_u32(vsha1su0q_u32(m4, m3, m2), m1);
}

/*
 * SHA-1 has three different kinds of round, differing in whether they
 * use the Ch, Maj or Par functions defined above. Each one uses a
 * separate NEON instruction, so we define three inline functions for
 * the different round types using this macro.
 *
 * The two batches of Par-type rounds also use a different constant,
 * but that's passed in as an operand, so we don't need a fourth
 * inline function just for that.
 */
#define SHA1_NEON_ROUND_FN(type)                                        \
    static inline sha1_neon_core sha1_neon_round4_##type(               \
        sha1_neon_core old, uint32x4_t sched, uint32x4_t constant)      \
    {                                                                   \
        sha1_neon_core new;                                             \
        uint32x4_t round_input = vaddq_u32(sched, constant);            \
        new.abcd = vsha1##type##q_u32(old.abcd, old.e, round_input);    \
        new.e = vsha1h_u32(vget_lane_u32(vget_low_u32(old.abcd), 0));   \
        return new;                                                     \
    }
SHA1_NEON_ROUND_FN(c)
SHA1_NEON_ROUND_FN(p)
SHA1_NEON_ROUND_FN(m)

static inline void sha1_neon_block(sha1_neon_core *core, const uint8_t *p)
{
    uint32x4_t constant, s0, s1, s2, s3;
    sha1_neon_core cr = *core;

    constant = vdupq_n_u32(SHA1_STAGE0_CONSTANT);
    s0 = sha1_neon_load_input(p);
    cr = sha1_neon_round4_c(cr, s0, constant);
    s1 = sha1_neon_load_input(p + 16);
    cr = sha1_neon_round4_c(cr, s1, constant);
    s2 = sha1_neon_load_input(p + 32);
    cr = sha1_neon_round4_c(cr, s2, constant);
    s3 = sha1_neon_load_input(p + 48);
    cr = sha1_neon_round4_c(cr, s3, constant);
    s0 = sha1_neon_schedule_update(s0, s1, s2, s3);
    cr = sha1_neon_round4_c(cr, s0, constant);

    constant = vdupq_n_u32(SHA1_STAGE1_CONSTANT);
    s1 = sha1_neon_schedule_update(s1, s2, s3, s0);
    cr = sha1_neon_round4_p(cr, s1, constant);
    s2 = sha1_neon_schedule_update(s2, s3, s0, s1);
    cr = sha1_neon_round4_p(cr, s2, constant);
    s3 = sha1_neon_schedule_update(s3, s0, s1, s2);
    cr = sha1_neon_round4_p(cr, s3, constant);
    s0 = sha1_neon_schedule_update(s0, s1, s2, s3);
    cr = sha1_neon_round4_p(cr, s0, constant);
    s1 = sha1_neon_schedule_update(s1, s2, s3, s0);
    cr = sha1_neon_round4_p(cr, s1, constant);

    constant = vdupq_n_u32(SHA1_STAGE2_CONSTANT);
    s2 = sha1_neon_schedule_update(s2, s3, s0, s1);
    cr = sha1_neon_round4_m(cr, s2, constant);
    s3 = sha1_neon_schedule_update(s3, s0, s1, s2);
    cr = sha1_neon_round4_m(cr, s3, constant);
    s0 = sha1_neon_schedule_update(s0, s1, s2, s3);
    cr = sha1_neon_round4_m(cr, s0, constant);
    s1 = sha1_neon_schedule_update(s1, s2, s3, s0);
    cr = sha1_neon_round4_m(cr, s1, constant);
    s2 = sha1_neon_schedule_update(s2, s3, s0, s1);
    cr = sha1_neon_round4_m(cr, s2, constant);

    constant = vdupq_n_u32(SHA1_STAGE3_CONSTANT);
    s3 = sha1_neon_schedule_update(s3, s0, s1, s2);
    cr = sha1_neon_round4_p(cr, s3, constant);
    s0 = sha1_neon_schedule_update(s0, s1, s2, s3);
    cr = sha1_neon_round4_p(cr, s0, constant);
    s1 = sha1_neon_schedule_update(s1, s2, s3, s0);
    cr = sha1_neon_round4_p(cr, s1, constant);
    s2 = sha1_neon_schedule_update(s2, s3, s0, s1);
    cr = sha1_neon_round4_p(cr, s2, constant);
    s3 = sha1_neon_schedule_update(s3, s0, s1, s2);
    cr = sha1_neon_round4_p(cr, s3, constant);

    core->abcd = vaddq_u32(core->abcd, cr.abcd);
    core->e += cr.e;
}

typedef struct sha1_neon {
    sha1_neon_core core;
    sha1_block blk;
    BinarySink_IMPLEMENTATION;
    ssh_hash hash;
} sha1_neon;

static void sha1_neon_write(BinarySink *bs, const void *vp, size_t len);

static ssh_hash *sha1_neon_new(const ssh_hashalg *alg)
{
    const struct sha1_extra *extra = (const struct sha1_extra *)alg->extra;
    if (!check_availability(extra))
        return NULL;

    sha1_neon *s = snew(sha1_neon);

    s->hash.vt = alg;
    BinarySink_INIT(s, sha1_neon_write);
    BinarySink_DELEGATE_INIT(&s->hash, s);
    return &s->hash;
}

static void sha1_neon_reset(ssh_hash *hash)
{
    sha1_neon *s = container_of(hash, sha1_neon, hash);

    s->core.abcd = vld1q_u32(sha1_initial_state);
    s->core.e = sha1_initial_state[4];

    sha1_block_setup(&s->blk);
}

static void sha1_neon_copyfrom(ssh_hash *hcopy, ssh_hash *horig)
{
    sha1_neon *copy = container_of(hcopy, sha1_neon, hash);
    sha1_neon *orig = container_of(horig, sha1_neon, hash);

    *copy = *orig; /* structure copy */

    BinarySink_COPIED(copy);
    BinarySink_DELEGATE_INIT(&copy->hash, copy);
}

static void sha1_neon_free(ssh_hash *hash)
{
    sha1_neon *s = container_of(hash, sha1_neon, hash);
    smemclr(s, sizeof(*s));
    sfree(s);
}

static void sha1_neon_write(BinarySink *bs, const void *vp, size_t len)
{
    sha1_neon *s = BinarySink_DOWNCAST(bs, sha1_neon);

    while (len > 0)
        if (sha1_block_write(&s->blk, &vp, &len))
            sha1_neon_block(&s->core, s->blk.block);
}

static void sha1_neon_digest(ssh_hash *hash, uint8_t *digest)
{
    sha1_neon *s = container_of(hash, sha1_neon, hash);

    sha1_block_pad(&s->blk, BinarySink_UPCAST(s));
    vst1q_u8(digest, vrev32q_u8(vreinterpretq_u8_u32(s->core.abcd)));
    PUT_32BIT_MSB_FIRST(digest + 16, s->core.e);
}

SHA1_VTABLE(neon, "NEON accelerated");
