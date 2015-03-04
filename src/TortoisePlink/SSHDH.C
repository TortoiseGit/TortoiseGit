/*
 * Diffie-Hellman implementation for PuTTY.
 */

#include "ssh.h"

/*
 * The primes used in the group1 and group14 key exchange.
 */
static const unsigned char P1[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
    0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
    0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
    0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
    0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
    0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
    0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
    0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static const unsigned char P14[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
    0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
    0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
    0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
    0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
    0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
    0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
    0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
    0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05, 0x98, 0xDA, 0x48, 0x36,
    0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
    0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56,
    0x20, 0x85, 0x52, 0xBB, 0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
    0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04, 0xF1, 0x74, 0x6C, 0x08,
    0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
    0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2,
    0xEC, 0x07, 0xA2, 0x8F, 0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,
    0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18, 0x39, 0x95, 0x49, 0x7C,
    0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
    0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAC, 0xAA, 0x68, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
};

/*
 * The generator g = 2 (used for both group1 and group14).
 */
static const unsigned char G[] = { 2 };

static const struct ssh_kex ssh_diffiehellman_group1_sha1 = {
    "diffie-hellman-group1-sha1", "group1",
    KEXTYPE_DH, P1, G, lenof(P1), lenof(G), &ssh_sha1
};

static const struct ssh_kex *const group1_list[] = {
    &ssh_diffiehellman_group1_sha1
};

const struct ssh_kexes ssh_diffiehellman_group1 = {
    sizeof(group1_list) / sizeof(*group1_list),
    group1_list
};

static const struct ssh_kex ssh_diffiehellman_group14_sha1 = {
    "diffie-hellman-group14-sha1", "group14",
    KEXTYPE_DH, P14, G, lenof(P14), lenof(G), &ssh_sha1
};

static const struct ssh_kex *const group14_list[] = {
    &ssh_diffiehellman_group14_sha1
};

const struct ssh_kexes ssh_diffiehellman_group14 = {
    sizeof(group14_list) / sizeof(*group14_list),
    group14_list
};

static const struct ssh_kex ssh_diffiehellman_gex_sha256 = {
    "diffie-hellman-group-exchange-sha256", NULL,
    KEXTYPE_DH, NULL, NULL, 0, 0, &ssh_sha256
};

static const struct ssh_kex ssh_diffiehellman_gex_sha1 = {
    "diffie-hellman-group-exchange-sha1", NULL,
    KEXTYPE_DH, NULL, NULL, 0, 0, &ssh_sha1
};

static const struct ssh_kex *const gex_list[] = {
    &ssh_diffiehellman_gex_sha256,
    &ssh_diffiehellman_gex_sha1
};

const struct ssh_kexes ssh_diffiehellman_gex = {
    sizeof(gex_list) / sizeof(*gex_list),
    gex_list
};

/*
 * Variables.
 */
struct dh_ctx {
    Bignum x, e, p, q, qmask, g;
};

/*
 * Common DH initialisation.
 */
static void dh_init(struct dh_ctx *ctx)
{
    ctx->q = bignum_rshift(ctx->p, 1);
    ctx->qmask = bignum_bitmask(ctx->q);
    ctx->x = ctx->e = NULL;
}

/*
 * Initialise DH for a standard group.
 */
void *dh_setup_group(const struct ssh_kex *kex)
{
    struct dh_ctx *ctx = snew(struct dh_ctx);
    ctx->p = bignum_from_bytes(kex->pdata, kex->plen);
    ctx->g = bignum_from_bytes(kex->gdata, kex->glen);
    dh_init(ctx);
    return ctx;
}

/*
 * Initialise DH for a server-supplied group.
 */
void *dh_setup_gex(Bignum pval, Bignum gval)
{
    struct dh_ctx *ctx = snew(struct dh_ctx);
    ctx->p = copybn(pval);
    ctx->g = copybn(gval);
    dh_init(ctx);
    return ctx;
}

/*
 * Clean up and free a context.
 */
void dh_cleanup(void *handle)
{
    struct dh_ctx *ctx = (struct dh_ctx *)handle;
    freebn(ctx->x);
    freebn(ctx->e);
    freebn(ctx->p);
    freebn(ctx->g);
    freebn(ctx->q);
    freebn(ctx->qmask);
    sfree(ctx);
}

/*
 * DH stage 1: invent a number x between 1 and q, and compute e =
 * g^x mod p. Return e.
 * 
 * If `nbits' is greater than zero, it is used as an upper limit
 * for the number of bits in x. This is safe provided that (a) you
 * use twice as many bits in x as the number of bits you expect to
 * use in your session key, and (b) the DH group is a safe prime
 * (which SSH demands that it must be).
 * 
 * P. C. van Oorschot, M. J. Wiener
 * "On Diffie-Hellman Key Agreement with Short Exponents".
 * Advances in Cryptology: Proceedings of Eurocrypt '96
 * Springer-Verlag, May 1996.
 */
Bignum dh_create_e(void *handle, int nbits)
{
    struct dh_ctx *ctx = (struct dh_ctx *)handle;
    int i;

    int nbytes;
    unsigned char *buf;

    nbytes = ssh1_bignum_length(ctx->qmask);
    buf = snewn(nbytes, unsigned char);

    do {
	/*
	 * Create a potential x, by ANDing a string of random bytes
	 * with qmask.
	 */
	if (ctx->x)
	    freebn(ctx->x);
	if (nbits == 0 || nbits > bignum_bitcount(ctx->qmask)) {
	    ssh1_write_bignum(buf, ctx->qmask);
	    for (i = 2; i < nbytes; i++)
		buf[i] &= random_byte();
	    ssh1_read_bignum(buf, nbytes, &ctx->x);   /* can't fail */
	} else {
	    int b, nb;
	    ctx->x = bn_power_2(nbits);
	    b = nb = 0;
	    for (i = 0; i < nbits; i++) {
		if (nb == 0) {
		    nb = 8;
		    b = random_byte();
		}
		bignum_set_bit(ctx->x, i, b & 1);
		b >>= 1;
		nb--;
	    }
	}
    } while (bignum_cmp(ctx->x, One) <= 0 || bignum_cmp(ctx->x, ctx->q) >= 0);

    sfree(buf);

    /*
     * Done. Now compute e = g^x mod p.
     */
    ctx->e = modpow(ctx->g, ctx->x, ctx->p);

    return ctx->e;
}

/*
 * DH stage 2-epsilon: given a number f, validate it to ensure it's in
 * range. (RFC 4253 section 8: "Values of 'e' or 'f' that are not in
 * the range [1, p-1] MUST NOT be sent or accepted by either side."
 * Also, we rule out 1 and p-1 too, since that's easy to do and since
 * they lead to obviously weak keys that even a passive eavesdropper
 * can figure out.)
 */
const char *dh_validate_f(void *handle, Bignum f)
{
    struct dh_ctx *ctx = (struct dh_ctx *)handle;
    if (bignum_cmp(f, One) <= 0) {
        return "f value received is too small";
    } else {
        Bignum pm1 = bigsub(ctx->p, One);
        int cmp = bignum_cmp(f, pm1);
        freebn(pm1);
        if (cmp >= 0)
            return "f value received is too large";
    }
    return NULL;
}

/*
 * DH stage 2: given a number f, compute K = f^x mod p.
 */
Bignum dh_find_K(void *handle, Bignum f)
{
    struct dh_ctx *ctx = (struct dh_ctx *)handle;
    Bignum ret;
    ret = modpow(f, ctx->x, ctx->p);
    return ret;
}
