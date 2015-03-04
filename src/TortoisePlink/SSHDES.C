#include <assert.h>
#include "ssh.h"


/* des.c - implementation of DES
 */

/*
 * Description of DES
 * ------------------
 *
 * Unlike the description in FIPS 46, I'm going to use _sensible_ indices:
 * bits in an n-bit word are numbered from 0 at the LSB to n-1 at the MSB.
 * And S-boxes are indexed by six consecutive bits, not by the outer two
 * followed by the middle four.
 *
 * The DES encryption routine requires a 64-bit input, and a key schedule K
 * containing 16 48-bit elements.
 *
 *   First the input is permuted by the initial permutation IP.
 *   Then the input is split into 32-bit words L and R. (L is the MSW.)
 *   Next, 16 rounds. In each round:
 *     (L, R) <- (R, L xor f(R, K[i]))
 *   Then the pre-output words L and R are swapped.
 *   Then L and R are glued back together into a 64-bit word. (L is the MSW,
 *     again, but since we just swapped them, the MSW is the R that came out
 *     of the last round.)
 *   The 64-bit output block is permuted by the inverse of IP and returned.
 *
 * Decryption is identical except that the elements of K are used in the
 * opposite order. (This wouldn't work if that word swap didn't happen.)
 *
 * The function f, used in each round, accepts a 32-bit word R and a
 * 48-bit key block K. It produces a 32-bit output.
 *
 *   First R is expanded to 48 bits using the bit-selection function E.
 *   The resulting 48-bit block is XORed with the key block K to produce
 *     a 48-bit block X.
 *   This block X is split into eight groups of 6 bits. Each group of 6
 *     bits is then looked up in one of the eight S-boxes to convert
 *     it to 4 bits. These eight groups of 4 bits are glued back
 *     together to produce a 32-bit preoutput block.
 *   The preoutput block is permuted using the permutation P and returned.
 *
 * Key setup maps a 64-bit key word into a 16x48-bit key schedule. Although
 * the approved input format for the key is a 64-bit word, eight of the
 * bits are discarded, so the actual quantity of key used is 56 bits.
 *
 *   First the input key is converted to two 28-bit words C and D using
 *     the bit-selection function PC1.
 *   Then 16 rounds of key setup occur. In each round, C and D are each
 *     rotated left by either 1 or 2 bits (depending on which round), and
 *     then converted into a key schedule element using the bit-selection
 *     function PC2.
 *
 * That's the actual algorithm. Now for the tedious details: all those
 * painful permutations and lookup tables.
 *
 * IP is a 64-to-64 bit permutation. Its output contains the following
 * bits of its input (listed in order MSB to LSB of output).
 *
 *    6 14 22 30 38 46 54 62  4 12 20 28 36 44 52 60
 *    2 10 18 26 34 42 50 58  0  8 16 24 32 40 48 56
 *    7 15 23 31 39 47 55 63  5 13 21 29 37 45 53 61
 *    3 11 19 27 35 43 51 59  1  9 17 25 33 41 49 57
 *
 * E is a 32-to-48 bit selection function. Its output contains the following
 * bits of its input (listed in order MSB to LSB of output).
 *
 *    0 31 30 29 28 27 28 27 26 25 24 23 24 23 22 21 20 19 20 19 18 17 16 15
 *   16 15 14 13 12 11 12 11 10  9  8  7  8  7  6  5  4  3  4  3  2  1  0 31
 *
 * The S-boxes are arbitrary table-lookups each mapping a 6-bit input to a
 * 4-bit output. In other words, each S-box is an array[64] of 4-bit numbers.
 * The S-boxes are listed below. The first S-box listed is applied to the
 * most significant six bits of the block X; the last one is applied to the
 * least significant.
 *
 *   14  0  4 15 13  7  1  4  2 14 15  2 11 13  8  1
 *    3 10 10  6  6 12 12 11  5  9  9  5  0  3  7  8
 *    4 15  1 12 14  8  8  2 13  4  6  9  2  1 11  7
 *   15  5 12 11  9  3  7 14  3 10 10  0  5  6  0 13
 *
 *   15  3  1 13  8  4 14  7  6 15 11  2  3  8  4 14
 *    9 12  7  0  2  1 13 10 12  6  0  9  5 11 10  5
 *    0 13 14  8  7 10 11  1 10  3  4 15 13  4  1  2
 *    5 11  8  6 12  7  6 12  9  0  3  5  2 14 15  9
 *
 *   10 13  0  7  9  0 14  9  6  3  3  4 15  6  5 10
 *    1  2 13  8 12  5  7 14 11 12  4 11  2 15  8  1
 *   13  1  6 10  4 13  9  0  8  6 15  9  3  8  0  7
 *   11  4  1 15  2 14 12  3  5 11 10  5 14  2  7 12
 *
 *    7 13 13  8 14 11  3  5  0  6  6 15  9  0 10  3
 *    1  4  2  7  8  2  5 12 11  1 12 10  4 14 15  9
 *   10  3  6 15  9  0  0  6 12 10 11  1  7 13 13  8
 *   15  9  1  4  3  5 14 11  5 12  2  7  8  2  4 14
 *
 *    2 14 12 11  4  2  1 12  7  4 10  7 11 13  6  1
 *    8  5  5  0  3 15 15 10 13  3  0  9 14  8  9  6
 *    4 11  2  8  1 12 11  7 10  1 13 14  7  2  8 13
 *   15  6  9 15 12  0  5  9  6 10  3  4  0  5 14  3
 *
 *   12 10  1 15 10  4 15  2  9  7  2 12  6  9  8  5
 *    0  6 13  1  3 13  4 14 14  0  7 11  5  3 11  8
 *    9  4 14  3 15  2  5 12  2  9  8  5 12 15  3 10
 *    7 11  0 14  4  1 10  7  1  6 13  0 11  8  6 13
 *
 *    4 13 11  0  2 11 14  7 15  4  0  9  8  1 13 10
 *    3 14 12  3  9  5  7 12  5  2 10 15  6  8  1  6
 *    1  6  4 11 11 13 13  8 12  1  3  4  7 10 14  7
 *   10  9 15  5  6  0  8 15  0 14  5  2  9  3  2 12
 *
 *   13  1  2 15  8 13  4  8  6 10 15  3 11  7  1  4
 *   10 12  9  5  3  6 14 11  5  0  0 14 12  9  7  2
 *    7  2 11  1  4 14  1  7  9  4 12 10 14  8  2 13
 *    0 15  6 12 10  9 13  0 15  3  3  5  5  6  8 11
 *
 * P is a 32-to-32 bit permutation. Its output contains the following
 * bits of its input (listed in order MSB to LSB of output).
 *
 *   16 25 12 11  3 20  4 15 31 17  9  6 27 14  1 22
 *   30 24  8 18  0  5 29 23 13 19  2 26 10 21 28  7
 *
 * PC1 is a 64-to-56 bit selection function. Its output is in two words,
 * C and D. The word C contains the following bits of its input (listed
 * in order MSB to LSB of output).
 *
 *    7 15 23 31 39 47 55 63  6 14 22 30 38 46
 *   54 62  5 13 21 29 37 45 53 61  4 12 20 28
 *
 * And the word D contains these bits.
 *
 *    1  9 17 25 33 41 49 57  2 10 18 26 34 42
 *   50 58  3 11 19 27 35 43 51 59 36 44 52 60
 *
 * PC2 is a 56-to-48 bit selection function. Its input is in two words,
 * C and D. These are treated as one 56-bit word (with C more significant,
 * so that bits 55 to 28 of the word are bits 27 to 0 of C, and bits 27 to
 * 0 of the word are bits 27 to 0 of D). The output contains the following
 * bits of this 56-bit input word (listed in order MSB to LSB of output).
 *
 *   42 39 45 32 55 51 53 28 41 50 35 46 33 37 44 52 30 48 40 49 29 36 43 54
 *   15  4 25 19  9  1 26 16  5 11 23  8 12  7 17  0 22  3 10 14  6 20 27 24
 */

/*
 * Implementation details
 * ----------------------
 * 
 * If you look at the code in this module, you'll find it looks
 * nothing _like_ the above algorithm. Here I explain the
 * differences...
 *
 * Key setup has not been heavily optimised here. We are not
 * concerned with key agility: we aren't codebreakers. We don't
 * mind a little delay (and it really is a little one; it may be a
 * factor of five or so slower than it could be but it's still not
 * an appreciable length of time) while setting up. The only tweaks
 * in the key setup are ones which change the format of the key
 * schedule to speed up the actual encryption. I'll describe those
 * below.
 *
 * The first and most obvious optimisation is the S-boxes. Since
 * each S-box always targets the same four bits in the final 32-bit
 * word, so the output from (for example) S-box 0 must always be
 * shifted left 28 bits, we can store the already-shifted outputs
 * in the lookup tables. This reduces lookup-and-shift to lookup,
 * so the S-box step is now just a question of ORing together eight
 * table lookups.
 *
 * The permutation P is just a bit order change; it's invariant
 * with respect to OR, in that P(x)|P(y) = P(x|y). Therefore, we
 * can apply P to every entry of the S-box tables and then we don't
 * have to do it in the code of f(). This yields a set of tables
 * which might be called SP-boxes.
 *
 * The bit-selection function E is our next target. Note that E is
 * immediately followed by the operation of splitting into 6-bit
 * chunks. Examining the 6-bit chunks coming out of E we notice
 * they're all contiguous within the word (speaking cyclically -
 * the end two wrap round); so we can extract those bit strings
 * individually rather than explicitly running E. This would yield
 * code such as
 *
 *     y |= SPboxes[0][ (rotl(R, 5) ^  top6bitsofK) & 0x3F ];
 *     t |= SPboxes[1][ (rotl(R,11) ^ next6bitsofK) & 0x3F ];
 *
 * and so on; and the key schedule preparation would have to
 * provide each 6-bit chunk separately.
 *
 * Really we'd like to XOR in the key schedule element before
 * looking up bit strings in R. This we can't do, naively, because
 * the 6-bit strings we want overlap. But look at the strings:
 *
 *       3322222222221111111111
 * bit   10987654321098765432109876543210
 * 
 * box0  XXXXX                          X
 * box1     XXXXXX
 * box2         XXXXXX
 * box3             XXXXXX
 * box4                 XXXXXX
 * box5                     XXXXXX
 * box6                         XXXXXX
 * box7  X                          XXXXX
 *
 * The bit strings we need to XOR in for boxes 0, 2, 4 and 6 don't
 * overlap with each other. Neither do the ones for boxes 1, 3, 5
 * and 7. So we could provide the key schedule in the form of two
 * words that we can separately XOR into R, and then every S-box
 * index is available as a (cyclically) contiguous 6-bit substring
 * of one or the other of the results.
 *
 * The comments in Eric Young's libdes implementation point out
 * that two of these bit strings require a rotation (rather than a
 * simple shift) to extract. It's unavoidable that at least _one_
 * must do; but we can actually run the whole inner algorithm (all
 * 16 rounds) rotated one bit to the left, so that what the `real'
 * DES description sees as L=0x80000001 we see as L=0x00000003.
 * This requires rotating all our SP-box entries one bit to the
 * left, and rotating each word of the key schedule elements one to
 * the left, and rotating L and R one bit left just after IP and
 * one bit right again just before FP. And in each round we convert
 * a rotate into a shift, so we've saved a few per cent.
 *
 * That's about it for the inner loop; the SP-box tables as listed
 * below are what I've described here (the original S value,
 * shifted to its final place in the input to P, run through P, and
 * then rotated one bit left). All that remains is to optimise the
 * initial permutation IP.
 *
 * IP is not an arbitrary permutation. It has the nice property
 * that if you take any bit number, write it in binary (6 bits),
 * permute those 6 bits and invert some of them, you get the final
 * position of that bit. Specifically, the bit whose initial
 * position is given (in binary) as fedcba ends up in position
 * AcbFED (where a capital letter denotes the inverse of a bit).
 *
 * We have the 64-bit data in two 32-bit words L and R, where bits
 * in L are those with f=1 and bits in R are those with f=0. We
 * note that we can do a simple transformation: suppose we exchange
 * the bits with f=1,c=0 and the bits with f=0,c=1. This will cause
 * the bit fedcba to be in position cedfba - we've `swapped' bits c
 * and f in the position of each bit!
 * 
 * Better still, this transformation is easy. In the example above,
 * bits in L with c=0 are bits 0x0F0F0F0F, and those in R with c=1
 * are 0xF0F0F0F0. So we can do
 *
 *     difference = ((R >> 4) ^ L) & 0x0F0F0F0F
 *     R ^= (difference << 4)
 *     L ^= difference
 *
 * to perform the swap. Let's denote this by bitswap(4,0x0F0F0F0F).
 * Also, we can invert the bit at the top just by exchanging L and
 * R. So in a few swaps and a few of these bit operations we can
 * do:
 * 
 * Initially the position of bit fedcba is     fedcba
 * Swap L with R to make it                    Fedcba
 * Perform bitswap( 4,0x0F0F0F0F) to make it   cedFba
 * Perform bitswap(16,0x0000FFFF) to make it   ecdFba
 * Swap L with R to make it                    EcdFba
 * Perform bitswap( 2,0x33333333) to make it   bcdFEa
 * Perform bitswap( 8,0x00FF00FF) to make it   dcbFEa
 * Swap L with R to make it                    DcbFEa
 * Perform bitswap( 1,0x55555555) to make it   acbFED
 * Swap L with R to make it                    AcbFED
 *
 * (In the actual code the four swaps are implicit: R and L are
 * simply used the other way round in the first, second and last
 * bitswap operations.)
 *
 * The final permutation is just the inverse of IP, so it can be
 * performed by a similar set of operations.
 */

typedef struct {
    word32 k0246[16], k1357[16];
    word32 iv0, iv1;
} DESContext;

#define rotl(x, c) ( (x << c) | (x >> (32-c)) )
#define rotl28(x, c) ( ( (x << c) | (x >> (28-c)) ) & 0x0FFFFFFF)

static word32 bitsel(word32 * input, const int *bitnums, int size)
{
    word32 ret = 0;
    while (size--) {
	int bitpos = *bitnums++;
	ret <<= 1;
	if (bitpos >= 0)
	    ret |= 1 & (input[bitpos / 32] >> (bitpos % 32));
    }
    return ret;
}

static void des_key_setup(word32 key_msw, word32 key_lsw, DESContext * sched)
{

    static const int PC1_Cbits[] = {
	7, 15, 23, 31, 39, 47, 55, 63, 6, 14, 22, 30, 38, 46,
	54, 62, 5, 13, 21, 29, 37, 45, 53, 61, 4, 12, 20, 28
    };
    static const int PC1_Dbits[] = {
	1, 9, 17, 25, 33, 41, 49, 57, 2, 10, 18, 26, 34, 42,
	50, 58, 3, 11, 19, 27, 35, 43, 51, 59, 36, 44, 52, 60
    };
    /*
     * The bit numbers in the two lists below don't correspond to
     * the ones in the above description of PC2, because in the
     * above description C and D are concatenated so `bit 28' means
     * bit 0 of C. In this implementation we're using the standard
     * `bitsel' function above and C is in the second word, so bit
     * 0 of C is addressed by writing `32' here.
     */
    static const int PC2_0246[] = {
	49, 36, 59, 55, -1, -1, 37, 41, 48, 56, 34, 52, -1, -1, 15, 4,
	25, 19, 9, 1, -1, -1, 12, 7, 17, 0, 22, 3, -1, -1, 46, 43
    };
    static const int PC2_1357[] = {
	-1, -1, 57, 32, 45, 54, 39, 50, -1, -1, 44, 53, 33, 40, 47, 58,
	-1, -1, 26, 16, 5, 11, 23, 8, -1, -1, 10, 14, 6, 20, 27, 24
    };
    static const int leftshifts[] =
	{ 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

    word32 C, D;
    word32 buf[2];
    int i;

    buf[0] = key_lsw;
    buf[1] = key_msw;

    C = bitsel(buf, PC1_Cbits, 28);
    D = bitsel(buf, PC1_Dbits, 28);

    for (i = 0; i < 16; i++) {
	C = rotl28(C, leftshifts[i]);
	D = rotl28(D, leftshifts[i]);
	buf[0] = D;
	buf[1] = C;
	sched->k0246[i] = bitsel(buf, PC2_0246, 32);
	sched->k1357[i] = bitsel(buf, PC2_1357, 32);
    }

    sched->iv0 = sched->iv1 = 0;
}

static const word32 SPboxes[8][64] = {
    {0x01010400, 0x00000000, 0x00010000, 0x01010404,
     0x01010004, 0x00010404, 0x00000004, 0x00010000,
     0x00000400, 0x01010400, 0x01010404, 0x00000400,
     0x01000404, 0x01010004, 0x01000000, 0x00000004,
     0x00000404, 0x01000400, 0x01000400, 0x00010400,
     0x00010400, 0x01010000, 0x01010000, 0x01000404,
     0x00010004, 0x01000004, 0x01000004, 0x00010004,
     0x00000000, 0x00000404, 0x00010404, 0x01000000,
     0x00010000, 0x01010404, 0x00000004, 0x01010000,
     0x01010400, 0x01000000, 0x01000000, 0x00000400,
     0x01010004, 0x00010000, 0x00010400, 0x01000004,
     0x00000400, 0x00000004, 0x01000404, 0x00010404,
     0x01010404, 0x00010004, 0x01010000, 0x01000404,
     0x01000004, 0x00000404, 0x00010404, 0x01010400,
     0x00000404, 0x01000400, 0x01000400, 0x00000000,
     0x00010004, 0x00010400, 0x00000000, 0x01010004L},

    {0x80108020, 0x80008000, 0x00008000, 0x00108020,
     0x00100000, 0x00000020, 0x80100020, 0x80008020,
     0x80000020, 0x80108020, 0x80108000, 0x80000000,
     0x80008000, 0x00100000, 0x00000020, 0x80100020,
     0x00108000, 0x00100020, 0x80008020, 0x00000000,
     0x80000000, 0x00008000, 0x00108020, 0x80100000,
     0x00100020, 0x80000020, 0x00000000, 0x00108000,
     0x00008020, 0x80108000, 0x80100000, 0x00008020,
     0x00000000, 0x00108020, 0x80100020, 0x00100000,
     0x80008020, 0x80100000, 0x80108000, 0x00008000,
     0x80100000, 0x80008000, 0x00000020, 0x80108020,
     0x00108020, 0x00000020, 0x00008000, 0x80000000,
     0x00008020, 0x80108000, 0x00100000, 0x80000020,
     0x00100020, 0x80008020, 0x80000020, 0x00100020,
     0x00108000, 0x00000000, 0x80008000, 0x00008020,
     0x80000000, 0x80100020, 0x80108020, 0x00108000L},

    {0x00000208, 0x08020200, 0x00000000, 0x08020008,
     0x08000200, 0x00000000, 0x00020208, 0x08000200,
     0x00020008, 0x08000008, 0x08000008, 0x00020000,
     0x08020208, 0x00020008, 0x08020000, 0x00000208,
     0x08000000, 0x00000008, 0x08020200, 0x00000200,
     0x00020200, 0x08020000, 0x08020008, 0x00020208,
     0x08000208, 0x00020200, 0x00020000, 0x08000208,
     0x00000008, 0x08020208, 0x00000200, 0x08000000,
     0x08020200, 0x08000000, 0x00020008, 0x00000208,
     0x00020000, 0x08020200, 0x08000200, 0x00000000,
     0x00000200, 0x00020008, 0x08020208, 0x08000200,
     0x08000008, 0x00000200, 0x00000000, 0x08020008,
     0x08000208, 0x00020000, 0x08000000, 0x08020208,
     0x00000008, 0x00020208, 0x00020200, 0x08000008,
     0x08020000, 0x08000208, 0x00000208, 0x08020000,
     0x00020208, 0x00000008, 0x08020008, 0x00020200L},

    {0x00802001, 0x00002081, 0x00002081, 0x00000080,
     0x00802080, 0x00800081, 0x00800001, 0x00002001,
     0x00000000, 0x00802000, 0x00802000, 0x00802081,
     0x00000081, 0x00000000, 0x00800080, 0x00800001,
     0x00000001, 0x00002000, 0x00800000, 0x00802001,
     0x00000080, 0x00800000, 0x00002001, 0x00002080,
     0x00800081, 0x00000001, 0x00002080, 0x00800080,
     0x00002000, 0x00802080, 0x00802081, 0x00000081,
     0x00800080, 0x00800001, 0x00802000, 0x00802081,
     0x00000081, 0x00000000, 0x00000000, 0x00802000,
     0x00002080, 0x00800080, 0x00800081, 0x00000001,
     0x00802001, 0x00002081, 0x00002081, 0x00000080,
     0x00802081, 0x00000081, 0x00000001, 0x00002000,
     0x00800001, 0x00002001, 0x00802080, 0x00800081,
     0x00002001, 0x00002080, 0x00800000, 0x00802001,
     0x00000080, 0x00800000, 0x00002000, 0x00802080L},

    {0x00000100, 0x02080100, 0x02080000, 0x42000100,
     0x00080000, 0x00000100, 0x40000000, 0x02080000,
     0x40080100, 0x00080000, 0x02000100, 0x40080100,
     0x42000100, 0x42080000, 0x00080100, 0x40000000,
     0x02000000, 0x40080000, 0x40080000, 0x00000000,
     0x40000100, 0x42080100, 0x42080100, 0x02000100,
     0x42080000, 0x40000100, 0x00000000, 0x42000000,
     0x02080100, 0x02000000, 0x42000000, 0x00080100,
     0x00080000, 0x42000100, 0x00000100, 0x02000000,
     0x40000000, 0x02080000, 0x42000100, 0x40080100,
     0x02000100, 0x40000000, 0x42080000, 0x02080100,
     0x40080100, 0x00000100, 0x02000000, 0x42080000,
     0x42080100, 0x00080100, 0x42000000, 0x42080100,
     0x02080000, 0x00000000, 0x40080000, 0x42000000,
     0x00080100, 0x02000100, 0x40000100, 0x00080000,
     0x00000000, 0x40080000, 0x02080100, 0x40000100L},

    {0x20000010, 0x20400000, 0x00004000, 0x20404010,
     0x20400000, 0x00000010, 0x20404010, 0x00400000,
     0x20004000, 0x00404010, 0x00400000, 0x20000010,
     0x00400010, 0x20004000, 0x20000000, 0x00004010,
     0x00000000, 0x00400010, 0x20004010, 0x00004000,
     0x00404000, 0x20004010, 0x00000010, 0x20400010,
     0x20400010, 0x00000000, 0x00404010, 0x20404000,
     0x00004010, 0x00404000, 0x20404000, 0x20000000,
     0x20004000, 0x00000010, 0x20400010, 0x00404000,
     0x20404010, 0x00400000, 0x00004010, 0x20000010,
     0x00400000, 0x20004000, 0x20000000, 0x00004010,
     0x20000010, 0x20404010, 0x00404000, 0x20400000,
     0x00404010, 0x20404000, 0x00000000, 0x20400010,
     0x00000010, 0x00004000, 0x20400000, 0x00404010,
     0x00004000, 0x00400010, 0x20004010, 0x00000000,
     0x20404000, 0x20000000, 0x00400010, 0x20004010L},

    {0x00200000, 0x04200002, 0x04000802, 0x00000000,
     0x00000800, 0x04000802, 0x00200802, 0x04200800,
     0x04200802, 0x00200000, 0x00000000, 0x04000002,
     0x00000002, 0x04000000, 0x04200002, 0x00000802,
     0x04000800, 0x00200802, 0x00200002, 0x04000800,
     0x04000002, 0x04200000, 0x04200800, 0x00200002,
     0x04200000, 0x00000800, 0x00000802, 0x04200802,
     0x00200800, 0x00000002, 0x04000000, 0x00200800,
     0x04000000, 0x00200800, 0x00200000, 0x04000802,
     0x04000802, 0x04200002, 0x04200002, 0x00000002,
     0x00200002, 0x04000000, 0x04000800, 0x00200000,
     0x04200800, 0x00000802, 0x00200802, 0x04200800,
     0x00000802, 0x04000002, 0x04200802, 0x04200000,
     0x00200800, 0x00000000, 0x00000002, 0x04200802,
     0x00000000, 0x00200802, 0x04200000, 0x00000800,
     0x04000002, 0x04000800, 0x00000800, 0x00200002L},

    {0x10001040, 0x00001000, 0x00040000, 0x10041040,
     0x10000000, 0x10001040, 0x00000040, 0x10000000,
     0x00040040, 0x10040000, 0x10041040, 0x00041000,
     0x10041000, 0x00041040, 0x00001000, 0x00000040,
     0x10040000, 0x10000040, 0x10001000, 0x00001040,
     0x00041000, 0x00040040, 0x10040040, 0x10041000,
     0x00001040, 0x00000000, 0x00000000, 0x10040040,
     0x10000040, 0x10001000, 0x00041040, 0x00040000,
     0x00041040, 0x00040000, 0x10041000, 0x00001000,
     0x00000040, 0x10040040, 0x00001000, 0x00041040,
     0x10001000, 0x00000040, 0x10000040, 0x10040000,
     0x10040040, 0x10000000, 0x00040000, 0x10001040,
     0x00000000, 0x10041040, 0x00040040, 0x10000040,
     0x10040000, 0x10001000, 0x10001040, 0x00000000,
     0x10041040, 0x00041000, 0x00041000, 0x00001040,
     0x00001040, 0x00040040, 0x10000000, 0x10041000L}
};

#define f(R, K0246, K1357) (\
    s0246 = R ^ K0246, \
    s1357 = R ^ K1357, \
    s0246 = rotl(s0246, 28), \
    SPboxes[0] [(s0246 >> 24) & 0x3F] | \
    SPboxes[1] [(s1357 >> 24) & 0x3F] | \
    SPboxes[2] [(s0246 >> 16) & 0x3F] | \
    SPboxes[3] [(s1357 >> 16) & 0x3F] | \
    SPboxes[4] [(s0246 >>  8) & 0x3F] | \
    SPboxes[5] [(s1357 >>  8) & 0x3F] | \
    SPboxes[6] [(s0246      ) & 0x3F] | \
    SPboxes[7] [(s1357      ) & 0x3F])

#define bitswap(L, R, n, mask) (\
    swap = mask & ( (R >> n) ^ L ), \
    R ^= swap << n, \
    L ^= swap)

/* Initial permutation */
#define IP(L, R) (\
    bitswap(R, L,  4, 0x0F0F0F0F), \
    bitswap(R, L, 16, 0x0000FFFF), \
    bitswap(L, R,  2, 0x33333333), \
    bitswap(L, R,  8, 0x00FF00FF), \
    bitswap(R, L,  1, 0x55555555))

/* Final permutation */
#define FP(L, R) (\
    bitswap(R, L,  1, 0x55555555), \
    bitswap(L, R,  8, 0x00FF00FF), \
    bitswap(L, R,  2, 0x33333333), \
    bitswap(R, L, 16, 0x0000FFFF), \
    bitswap(R, L,  4, 0x0F0F0F0F))

static void des_encipher(word32 * output, word32 L, word32 R,
			 DESContext * sched)
{
    word32 swap, s0246, s1357;

    IP(L, R);

    L = rotl(L, 1);
    R = rotl(R, 1);

    L ^= f(R, sched->k0246[0], sched->k1357[0]);
    R ^= f(L, sched->k0246[1], sched->k1357[1]);
    L ^= f(R, sched->k0246[2], sched->k1357[2]);
    R ^= f(L, sched->k0246[3], sched->k1357[3]);
    L ^= f(R, sched->k0246[4], sched->k1357[4]);
    R ^= f(L, sched->k0246[5], sched->k1357[5]);
    L ^= f(R, sched->k0246[6], sched->k1357[6]);
    R ^= f(L, sched->k0246[7], sched->k1357[7]);
    L ^= f(R, sched->k0246[8], sched->k1357[8]);
    R ^= f(L, sched->k0246[9], sched->k1357[9]);
    L ^= f(R, sched->k0246[10], sched->k1357[10]);
    R ^= f(L, sched->k0246[11], sched->k1357[11]);
    L ^= f(R, sched->k0246[12], sched->k1357[12]);
    R ^= f(L, sched->k0246[13], sched->k1357[13]);
    L ^= f(R, sched->k0246[14], sched->k1357[14]);
    R ^= f(L, sched->k0246[15], sched->k1357[15]);

    L = rotl(L, 31);
    R = rotl(R, 31);

    swap = L;
    L = R;
    R = swap;

    FP(L, R);

    output[0] = L;
    output[1] = R;
}

static void des_decipher(word32 * output, word32 L, word32 R,
			 DESContext * sched)
{
    word32 swap, s0246, s1357;

    IP(L, R);

    L = rotl(L, 1);
    R = rotl(R, 1);

    L ^= f(R, sched->k0246[15], sched->k1357[15]);
    R ^= f(L, sched->k0246[14], sched->k1357[14]);
    L ^= f(R, sched->k0246[13], sched->k1357[13]);
    R ^= f(L, sched->k0246[12], sched->k1357[12]);
    L ^= f(R, sched->k0246[11], sched->k1357[11]);
    R ^= f(L, sched->k0246[10], sched->k1357[10]);
    L ^= f(R, sched->k0246[9], sched->k1357[9]);
    R ^= f(L, sched->k0246[8], sched->k1357[8]);
    L ^= f(R, sched->k0246[7], sched->k1357[7]);
    R ^= f(L, sched->k0246[6], sched->k1357[6]);
    L ^= f(R, sched->k0246[5], sched->k1357[5]);
    R ^= f(L, sched->k0246[4], sched->k1357[4]);
    L ^= f(R, sched->k0246[3], sched->k1357[3]);
    R ^= f(L, sched->k0246[2], sched->k1357[2]);
    L ^= f(R, sched->k0246[1], sched->k1357[1]);
    R ^= f(L, sched->k0246[0], sched->k1357[0]);

    L = rotl(L, 31);
    R = rotl(R, 31);

    swap = L;
    L = R;
    R = swap;

    FP(L, R);

    output[0] = L;
    output[1] = R;
}

static void des_cbc_encrypt(unsigned char *blk,
			    unsigned int len, DESContext * sched)
{
    word32 out[2], iv0, iv1;
    unsigned int i;

    assert((len & 7) == 0);

    iv0 = sched->iv0;
    iv1 = sched->iv1;
    for (i = 0; i < len; i += 8) {
	iv0 ^= GET_32BIT_MSB_FIRST(blk);
	iv1 ^= GET_32BIT_MSB_FIRST(blk + 4);
	des_encipher(out, iv0, iv1, sched);
	iv0 = out[0];
	iv1 = out[1];
	PUT_32BIT_MSB_FIRST(blk, iv0);
	PUT_32BIT_MSB_FIRST(blk + 4, iv1);
	blk += 8;
    }
    sched->iv0 = iv0;
    sched->iv1 = iv1;
}

static void des_cbc_decrypt(unsigned char *blk,
			    unsigned int len, DESContext * sched)
{
    word32 out[2], iv0, iv1, xL, xR;
    unsigned int i;

    assert((len & 7) == 0);

    iv0 = sched->iv0;
    iv1 = sched->iv1;
    for (i = 0; i < len; i += 8) {
	xL = GET_32BIT_MSB_FIRST(blk);
	xR = GET_32BIT_MSB_FIRST(blk + 4);
	des_decipher(out, xL, xR, sched);
	iv0 ^= out[0];
	iv1 ^= out[1];
	PUT_32BIT_MSB_FIRST(blk, iv0);
	PUT_32BIT_MSB_FIRST(blk + 4, iv1);
	blk += 8;
	iv0 = xL;
	iv1 = xR;
    }
    sched->iv0 = iv0;
    sched->iv1 = iv1;
}

static void des_3cbc_encrypt(unsigned char *blk,
			     unsigned int len, DESContext * scheds)
{
    des_cbc_encrypt(blk, len, &scheds[0]);
    des_cbc_decrypt(blk, len, &scheds[1]);
    des_cbc_encrypt(blk, len, &scheds[2]);
}

static void des_cbc3_encrypt(unsigned char *blk,
			     unsigned int len, DESContext * scheds)
{
    word32 out[2], iv0, iv1;
    unsigned int i;

    assert((len & 7) == 0);

    iv0 = scheds->iv0;
    iv1 = scheds->iv1;
    for (i = 0; i < len; i += 8) {
	iv0 ^= GET_32BIT_MSB_FIRST(blk);
	iv1 ^= GET_32BIT_MSB_FIRST(blk + 4);
	des_encipher(out, iv0, iv1, &scheds[0]);
	des_decipher(out, out[0], out[1], &scheds[1]);
	des_encipher(out, out[0], out[1], &scheds[2]);
	iv0 = out[0];
	iv1 = out[1];
	PUT_32BIT_MSB_FIRST(blk, iv0);
	PUT_32BIT_MSB_FIRST(blk + 4, iv1);
	blk += 8;
    }
    scheds->iv0 = iv0;
    scheds->iv1 = iv1;
}

static void des_3cbc_decrypt(unsigned char *blk,
			     unsigned int len, DESContext * scheds)
{
    des_cbc_decrypt(blk, len, &scheds[2]);
    des_cbc_encrypt(blk, len, &scheds[1]);
    des_cbc_decrypt(blk, len, &scheds[0]);
}

static void des_cbc3_decrypt(unsigned char *blk,
			     unsigned int len, DESContext * scheds)
{
    word32 out[2], iv0, iv1, xL, xR;
    unsigned int i;

    assert((len & 7) == 0);

    iv0 = scheds->iv0;
    iv1 = scheds->iv1;
    for (i = 0; i < len; i += 8) {
	xL = GET_32BIT_MSB_FIRST(blk);
	xR = GET_32BIT_MSB_FIRST(blk + 4);
	des_decipher(out, xL, xR, &scheds[2]);
	des_encipher(out, out[0], out[1], &scheds[1]);
	des_decipher(out, out[0], out[1], &scheds[0]);
	iv0 ^= out[0];
	iv1 ^= out[1];
	PUT_32BIT_MSB_FIRST(blk, iv0);
	PUT_32BIT_MSB_FIRST(blk + 4, iv1);
	blk += 8;
	iv0 = xL;
	iv1 = xR;
    }
    scheds->iv0 = iv0;
    scheds->iv1 = iv1;
}

static void des_sdctr3(unsigned char *blk,
			     unsigned int len, DESContext * scheds)
{
    word32 b[2], iv0, iv1, tmp;
    unsigned int i;

    assert((len & 7) == 0);

    iv0 = scheds->iv0;
    iv1 = scheds->iv1;
    for (i = 0; i < len; i += 8) {
	des_encipher(b, iv0, iv1, &scheds[0]);
	des_decipher(b, b[0], b[1], &scheds[1]);
	des_encipher(b, b[0], b[1], &scheds[2]);
	tmp = GET_32BIT_MSB_FIRST(blk);
	PUT_32BIT_MSB_FIRST(blk, tmp ^ b[0]);
	blk += 4;
	tmp = GET_32BIT_MSB_FIRST(blk);
	PUT_32BIT_MSB_FIRST(blk, tmp ^ b[1]);
	blk += 4;
	if ((iv1 = (iv1 + 1) & 0xffffffff) == 0)
	    iv0 = (iv0 + 1) & 0xffffffff;
    }
    scheds->iv0 = iv0;
    scheds->iv1 = iv1;
}

static void *des3_make_context(void)
{
    return snewn(3, DESContext);
}

static void *des3_ssh1_make_context(void)
{
    /* Need 3 keys for each direction, in SSH-1 */
    return snewn(6, DESContext);
}

static void *des_make_context(void)
{
    return snew(DESContext);
}

static void *des_ssh1_make_context(void)
{
    /* Need one key for each direction, in SSH-1 */
    return snewn(2, DESContext);
}

static void des3_free_context(void *handle)   /* used for both 3DES and DES */
{
    sfree(handle);
}

static void des3_key(void *handle, unsigned char *key)
{
    DESContext *keys = (DESContext *) handle;
    des_key_setup(GET_32BIT_MSB_FIRST(key),
		  GET_32BIT_MSB_FIRST(key + 4), &keys[0]);
    des_key_setup(GET_32BIT_MSB_FIRST(key + 8),
		  GET_32BIT_MSB_FIRST(key + 12), &keys[1]);
    des_key_setup(GET_32BIT_MSB_FIRST(key + 16),
		  GET_32BIT_MSB_FIRST(key + 20), &keys[2]);
}

static void des3_iv(void *handle, unsigned char *key)
{
    DESContext *keys = (DESContext *) handle;
    keys[0].iv0 = GET_32BIT_MSB_FIRST(key);
    keys[0].iv1 = GET_32BIT_MSB_FIRST(key + 4);
}

static void des_key(void *handle, unsigned char *key)
{
    DESContext *keys = (DESContext *) handle;
    des_key_setup(GET_32BIT_MSB_FIRST(key),
		  GET_32BIT_MSB_FIRST(key + 4), &keys[0]);
}

static void des3_sesskey(void *handle, unsigned char *key)
{
    DESContext *keys = (DESContext *) handle;
    des3_key(keys, key);
    des3_key(keys+3, key);
}

static void des3_encrypt_blk(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_3cbc_encrypt(blk, len, keys);
}

static void des3_decrypt_blk(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_3cbc_decrypt(blk, len, keys+3);
}

static void des3_ssh2_encrypt_blk(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_cbc3_encrypt(blk, len, keys);
}

static void des3_ssh2_decrypt_blk(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_cbc3_decrypt(blk, len, keys);
}

static void des3_ssh2_sdctr(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_sdctr3(blk, len, keys);
}

static void des_ssh2_encrypt_blk(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_cbc_encrypt(blk, len, keys);
}

static void des_ssh2_decrypt_blk(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_cbc_decrypt(blk, len, keys);
}

void des3_decrypt_pubkey(unsigned char *key, unsigned char *blk, int len)
{
    DESContext ourkeys[3];
    des_key_setup(GET_32BIT_MSB_FIRST(key),
		  GET_32BIT_MSB_FIRST(key + 4), &ourkeys[0]);
    des_key_setup(GET_32BIT_MSB_FIRST(key + 8),
		  GET_32BIT_MSB_FIRST(key + 12), &ourkeys[1]);
    des_key_setup(GET_32BIT_MSB_FIRST(key),
		  GET_32BIT_MSB_FIRST(key + 4), &ourkeys[2]);
    des_3cbc_decrypt(blk, len, ourkeys);
    smemclr(ourkeys, sizeof(ourkeys));
}

void des3_encrypt_pubkey(unsigned char *key, unsigned char *blk, int len)
{
    DESContext ourkeys[3];
    des_key_setup(GET_32BIT_MSB_FIRST(key),
		  GET_32BIT_MSB_FIRST(key + 4), &ourkeys[0]);
    des_key_setup(GET_32BIT_MSB_FIRST(key + 8),
		  GET_32BIT_MSB_FIRST(key + 12), &ourkeys[1]);
    des_key_setup(GET_32BIT_MSB_FIRST(key),
		  GET_32BIT_MSB_FIRST(key + 4), &ourkeys[2]);
    des_3cbc_encrypt(blk, len, ourkeys);
    smemclr(ourkeys, sizeof(ourkeys));
}

void des3_decrypt_pubkey_ossh(unsigned char *key, unsigned char *iv,
			      unsigned char *blk, int len)
{
    DESContext ourkeys[3];
    des_key_setup(GET_32BIT_MSB_FIRST(key),
		  GET_32BIT_MSB_FIRST(key + 4), &ourkeys[0]);
    des_key_setup(GET_32BIT_MSB_FIRST(key + 8),
		  GET_32BIT_MSB_FIRST(key + 12), &ourkeys[1]);
    des_key_setup(GET_32BIT_MSB_FIRST(key + 16),
		  GET_32BIT_MSB_FIRST(key + 20), &ourkeys[2]);
    ourkeys[0].iv0 = GET_32BIT_MSB_FIRST(iv);
    ourkeys[0].iv1 = GET_32BIT_MSB_FIRST(iv+4);
    des_cbc3_decrypt(blk, len, ourkeys);
    smemclr(ourkeys, sizeof(ourkeys));
}

void des3_encrypt_pubkey_ossh(unsigned char *key, unsigned char *iv,
			      unsigned char *blk, int len)
{
    DESContext ourkeys[3];
    des_key_setup(GET_32BIT_MSB_FIRST(key),
		  GET_32BIT_MSB_FIRST(key + 4), &ourkeys[0]);
    des_key_setup(GET_32BIT_MSB_FIRST(key + 8),
		  GET_32BIT_MSB_FIRST(key + 12), &ourkeys[1]);
    des_key_setup(GET_32BIT_MSB_FIRST(key + 16),
		  GET_32BIT_MSB_FIRST(key + 20), &ourkeys[2]);
    ourkeys[0].iv0 = GET_32BIT_MSB_FIRST(iv);
    ourkeys[0].iv1 = GET_32BIT_MSB_FIRST(iv+4);
    des_cbc3_encrypt(blk, len, ourkeys);
    smemclr(ourkeys, sizeof(ourkeys));
}

static void des_keysetup_xdmauth(const unsigned char *keydata, DESContext *dc)
{
    unsigned char key[8];
    int i, nbits, j;
    unsigned int bits;

    bits = 0;
    nbits = 0;
    j = 0;
    for (i = 0; i < 8; i++) {
	if (nbits < 7) {
	    bits = (bits << 8) | keydata[j];
	    nbits += 8;
	    j++;
	}
	key[i] = (bits >> (nbits - 7)) << 1;
	bits &= ~(0x7F << (nbits - 7));
	nbits -= 7;
    }

    des_key_setup(GET_32BIT_MSB_FIRST(key), GET_32BIT_MSB_FIRST(key + 4), dc);
}

void des_encrypt_xdmauth(const unsigned char *keydata,
                         unsigned char *blk, int len)
{
    DESContext dc;
    des_keysetup_xdmauth(keydata, &dc);
    des_cbc_encrypt(blk, len, &dc);
}

void des_decrypt_xdmauth(const unsigned char *keydata,
                         unsigned char *blk, int len)
{
    DESContext dc;
    des_keysetup_xdmauth(keydata, &dc);
    des_cbc_decrypt(blk, len, &dc);
}

static const struct ssh2_cipher ssh_3des_ssh2 = {
    des3_make_context, des3_free_context, des3_iv, des3_key,
    des3_ssh2_encrypt_blk, des3_ssh2_decrypt_blk,
    "3des-cbc",
    8, 168, SSH_CIPHER_IS_CBC, "triple-DES CBC"
};

static const struct ssh2_cipher ssh_3des_ssh2_ctr = {
    des3_make_context, des3_free_context, des3_iv, des3_key,
    des3_ssh2_sdctr, des3_ssh2_sdctr,
    "3des-ctr",
    8, 168, 0, "triple-DES SDCTR"
};

/*
 * Single DES in SSH-2. "des-cbc" is marked as HISTORIC in
 * RFC 4250, referring to
 * FIPS-46-3.  ("Single DES (i.e., DES) will be permitted 
 * for legacy systems only.") , but ssh.com support it and 
 * apparently aren't the only people to do so, so we sigh 
 * and implement it anyway.
 */
static const struct ssh2_cipher ssh_des_ssh2 = {
    des_make_context, des3_free_context, des3_iv, des_key,
    des_ssh2_encrypt_blk, des_ssh2_decrypt_blk,
    "des-cbc",
    8, 56, SSH_CIPHER_IS_CBC, "single-DES CBC"
};

static const struct ssh2_cipher ssh_des_sshcom_ssh2 = {
    des_make_context, des3_free_context, des3_iv, des_key,
    des_ssh2_encrypt_blk, des_ssh2_decrypt_blk,
    "des-cbc@ssh.com",
    8, 56, SSH_CIPHER_IS_CBC, "single-DES CBC"
};

static const struct ssh2_cipher *const des3_list[] = {
    &ssh_3des_ssh2_ctr,
    &ssh_3des_ssh2
};

const struct ssh2_ciphers ssh2_3des = {
    sizeof(des3_list) / sizeof(*des3_list),
    des3_list
};

static const struct ssh2_cipher *const des_list[] = {
    &ssh_des_ssh2,
    &ssh_des_sshcom_ssh2
};

const struct ssh2_ciphers ssh2_des = {
    sizeof(des_list) / sizeof(*des_list),
    des_list
};

const struct ssh_cipher ssh_3des = {
    des3_ssh1_make_context, des3_free_context, des3_sesskey,
    des3_encrypt_blk, des3_decrypt_blk,
    8, "triple-DES inner-CBC"
};

static void des_sesskey(void *handle, unsigned char *key)
{
    DESContext *keys = (DESContext *) handle;
    des_key(keys, key);
    des_key(keys+1, key);
}

static void des_encrypt_blk(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_cbc_encrypt(blk, len, keys);
}

static void des_decrypt_blk(void *handle, unsigned char *blk, int len)
{
    DESContext *keys = (DESContext *) handle;
    des_cbc_decrypt(blk, len, keys+1);
}

const struct ssh_cipher ssh_des = {
    des_ssh1_make_context, des3_free_context, des_sesskey,
    des_encrypt_blk, des_decrypt_blk,
    8, "single-DES CBC"
};

#ifdef TEST_XDM_AUTH

/*
 * Small standalone utility which allows encryption and decryption of
 * single cipher blocks in the XDM-AUTHORIZATION-1 style. Written
 * during the rework of X authorisation for connection sharing, to
 * check the corner case when xa1_firstblock matches but the rest of
 * the authorisation is bogus.
 *
 * Just compile this file on its own with the above ifdef symbol
 * predefined:

gcc -DTEST_XDM_AUTH -o sshdes sshdes.c

 */

#include <stdlib.h>
void *safemalloc(size_t n, size_t size) { return calloc(n, size); }
void safefree(void *p) { return free(p); }
void smemclr(void *p, size_t size) { memset(p, 0, size); }
int main(int argc, char **argv)
{
    unsigned char words[2][8];
    unsigned char out[8];
    int i, j;

    memset(words, 0, sizeof(words));

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 8 && argv[i+1][2*j]; j++) {
            char x[3];
            unsigned u;
            x[0] = argv[i+1][2*j];
            x[1] = argv[i+1][2*j+1];
            x[2] = 0;
            sscanf(x, "%02x", &u);
            words[i][j] = u;
        }
    }

    memcpy(out, words[0], 8);
    des_decrypt_xdmauth(words[1], out, 8);
    printf("decrypt(%s,%s) = ", argv[1], argv[2]);
    for (i = 0; i < 8; i++) printf("%02x", out[i]);
    printf("\n");

    memcpy(out, words[0], 8);
    des_encrypt_xdmauth(words[1], out, 8);
    printf("encrypt(%s,%s) = ", argv[1], argv[2]);
    for (i = 0; i < 8; i++) printf("%02x", out[i]);
    printf("\n");
}

#endif
