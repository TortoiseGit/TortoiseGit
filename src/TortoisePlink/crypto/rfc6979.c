/*
 * Code to generate 'nonce' values for DSA signature algorithms, in a
 * deterministic way.
 */

#include "ssh.h"
#include "mpint.h"
#include "misc.h"

struct RFC6979 {
    /*
     * Size of the cyclic group over which we're doing DSA.
     * Equivalently, the multiplicative order of g (for integer DSA)
     * or the curve's base point (for ECDSA). For integer DSA this is
     * also the same thing as the small prime q from the key
     * parameters.
     *
     * This pointer is not owned. Freeing this structure will not free
     * it, and freeing the pointed-to integer before freeing this
     * structure will make this structure dangerous to use.
     */
    mp_int *q;

    /*
     * The private key integer, which is always the discrete log of
     * the public key with respect to the group generator.
     *
     * This pointer is not owned. Freeing this structure will not free
     * it, and freeing the pointed-to integer before freeing this
     * structure will make this structure dangerous to use.
     */
    mp_int *x;

    /*
     * Cached values derived from q: its length in bits, and in bytes.
     */
    size_t qbits, qbytes;

    /*
     * Reusable hash and MAC objects.
     */
    ssh_hash *hash;
    ssh2_mac *mac;

    /*
     * Cached value: the output length of the hash.
     */
    size_t hlen;

    /*
     * The byte string V used in the algorithm.
     */
    unsigned char V[MAX_HASH_LEN];

    /*
     * The string T to use during each attempt, and how many
     * hash-sized blocks to fill it with.
     */
    size_t T_nblocks;
    unsigned char *T;
};

static mp_int *bits2int(ptrlen b, RFC6979 *s)
{
    if (b.len > s->qbytes)
        b.len = s->qbytes;
    mp_int *x = mp_from_bytes_be(b);

    /*
     * Rationale for using mp_rshift_fixed_into and not
     * mp_rshift_safe_into: the shift count is derived from the
     * difference between the length of the modulus q, and the length
     * of the input bit string, i.e. between the _sizes_ of things
     * involved in the protocol. But the sizes aren't secret. Only the
     * actual values of integers and bit strings of those sizes are
     * secret. So it's OK for the shift count to be known to an
     * attacker - they'd know it anyway just from which DSA algorithm
     * we were using.
     */
    if (b.len * 8 > s->qbits)
        mp_rshift_fixed_into(x, x, b.len * 8 - s->qbits);

    return x;
}

static void BinarySink_put_int2octets(BinarySink *bs, mp_int *x, RFC6979 *s)
{
    mp_int *x_mod_q = mp_mod(x, s->q);
    for (size_t i = s->qbytes; i-- > 0 ;)
        put_byte(bs, mp_get_byte(x_mod_q, i));
    mp_free(x_mod_q);
}

static void BinarySink_put_bits2octets(BinarySink *bs, ptrlen b, RFC6979 *s)
{
    mp_int *x = bits2int(b, s);
    BinarySink_put_int2octets(bs, x, s);
    mp_free(x);
}

#define put_int2octets(bs, x, s) \
    BinarySink_put_int2octets(BinarySink_UPCAST(bs), x, s)
#define put_bits2octets(bs, b, s) \
    BinarySink_put_bits2octets(BinarySink_UPCAST(bs), b, s)

RFC6979 *rfc6979_new(const ssh_hashalg *hashalg, mp_int *q, mp_int *x)
{
    /* Make the state structure. */
    RFC6979 *s = snew(RFC6979);
    s->q = q;
    s->x = x;
    s->qbits = mp_get_nbits(q);
    s->qbytes = (s->qbits + 7) >> 3;
    s->hash = ssh_hash_new(hashalg);
    s->mac = hmac_new_from_hash(hashalg);
    s->hlen = hashalg->hlen;

    /* In each attempt, we concatenate enough hash blocks to be
     * greater than qbits in size. */
    size_t hbits = 8 * s->hlen;
    s->T_nblocks = (s->qbits + hbits - 1) / hbits;
    s->T = snewn(s->T_nblocks * s->hlen, unsigned char);

    return s;
}

void rfc6979_setup(RFC6979 *s, ptrlen message)
{
    unsigned char h1[MAX_HASH_LEN];
    unsigned char K[MAX_HASH_LEN];

    /* 3.2 (a): hash the message to get h1. */
    ssh_hash_reset(s->hash);
    put_datapl(s->hash, message);
    ssh_hash_digest(s->hash, h1);

    /* 3.2 (b): set V to a sequence of 0x01 bytes the same size as the
     * hash function's output. */
    memset(s->V, 1, s->hlen);

    /* 3.2 (c): set the initial HMAC key K to all zeroes, again the
     * same size as the hash function's output. */
    memset(K, 0, s->hlen);
    ssh2_mac_setkey(s->mac, make_ptrlen(K, s->hlen));

    /* 3.2 (d): compute the MAC of V, the private key, and h1, with
     * key K, making a new key to replace K. */
    ssh2_mac_start(s->mac);
    put_data(s->mac, s->V, s->hlen);
    put_byte(s->mac, 0);
    put_int2octets(s->mac, s->x, s);
    put_bits2octets(s->mac, make_ptrlen(h1, s->hlen), s);
    ssh2_mac_genresult(s->mac, K);
    ssh2_mac_setkey(s->mac, make_ptrlen(K, s->hlen));

    /* 3.2 (e): replace V with its HMAC using the new K. */
    ssh2_mac_start(s->mac);
    put_data(s->mac, s->V, s->hlen);
    ssh2_mac_genresult(s->mac, s->V);

    /* 3.2 (f): repeat step (d), only using the new K in place of the
     * initial all-zeroes one, and with the extra byte in the middle
     * of the MAC preimage being 1 rather than 0. */
    ssh2_mac_start(s->mac);
    put_data(s->mac, s->V, s->hlen);
    put_byte(s->mac, 1);
    put_int2octets(s->mac, s->x, s);
    put_bits2octets(s->mac, make_ptrlen(h1, s->hlen), s);
    ssh2_mac_genresult(s->mac, K);
    ssh2_mac_setkey(s->mac, make_ptrlen(K, s->hlen));

    /* 3.2 (g): repeat step (e), using the again-replaced K. */
    ssh2_mac_start(s->mac);
    put_data(s->mac, s->V, s->hlen);
    ssh2_mac_genresult(s->mac, s->V);

    smemclr(h1, sizeof(h1));
    smemclr(K, sizeof(K));
}

RFC6979Result rfc6979_attempt(RFC6979 *s)
{
    RFC6979Result result;

    /* 3.2 (h) 1: set T to the empty string */
    /* 3.2 (h) 2: make lots of output by concatenating MACs of V */
    for (size_t i = 0; i < s->T_nblocks; i++) {
        ssh2_mac_start(s->mac);
        put_data(s->mac, s->V, s->hlen);
        ssh2_mac_genresult(s->mac, s->V);
        memcpy(s->T + i * s->hlen, s->V, s->hlen);
    }

    /* 3.2 (h) 3: if we have a number in [1, q-1], return it ... */
    result.k = bits2int(make_ptrlen(s->T, s->T_nblocks * s->hlen), s);
    result.ok = mp_hs_integer(result.k, 1) & ~mp_cmp_hs(result.k, s->q);

    /*
     * Perturb K and regenerate V ready for the next attempt.
     */
    ssh2_mac_start(s->mac);
    put_data(s->mac, s->V, s->hlen);
    put_byte(s->mac, 0);
    unsigned char K[MAX_HASH_LEN];
    ssh2_mac_genresult(s->mac, K);
    ssh2_mac_setkey(s->mac, make_ptrlen(K, s->hlen));
    smemclr(K, sizeof(K));

    ssh2_mac_start(s->mac);
    put_data(s->mac, s->V, s->hlen);
    ssh2_mac_genresult(s->mac, s->V);

    return result;
}

void rfc6979_free(RFC6979 *s)
{
    /* We don't free s->q or s->x: our caller still owns those. */

    ssh_hash_free(s->hash);
    ssh2_mac_free(s->mac);
    smemclr(s->T, s->T_nblocks * s->hlen);
    sfree(s->T);

    /* Clear the whole structure before freeing. Most fields aren't
     * sensitive (pointers or well-known length values), but V is, and
     * it's easier to clear the whole lot than fiddle about
     * identifying the sensitive fields. */
    smemclr(s, sizeof(*s));

    sfree(s);
}

mp_int *rfc6979(
    const ssh_hashalg *hashalg, mp_int *q, mp_int *x, ptrlen message)
{
    RFC6979 *s = rfc6979_new(hashalg, q, x);
    rfc6979_setup(s, message);
    RFC6979Result result;
    while (true) {
        result = rfc6979_attempt(s);
        if (result.ok)
            break;
        else
            mp_free(result.k);
    }
    rfc6979_free(s);
    return result.k;
}
