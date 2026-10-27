#include "decode.h"
#include "params.h"
#include <stdint.h>
#include <string.h>

#if KEM_MODE == 128
  #define F1_N (N / 2)
#elif KEM_MODE == 256 || KEM_MODE == 512
  #define F1_N (N / 4)
#endif

#define F1_NWORDS ((F1_N + 63) / 64)

typedef struct {
    uint64_t w[F1_NWORDS];
} poly_f1_u64;

/*************************************************
* Name:        f1u64_getbit
*
* Description: Extract one coefficient from a bit-packed polynomial over F2
*
* Arguments:   - const poly_f1_u64 *p: pointer to input polynomial
*              - int i: coefficient index in the range [0, F1_N)
*
* Returns:     coefficient i of p, either 0 or 1
**************************************************/
static inline int f1u64_getbit(const poly_f1_u64 *p, int i)
{
    return (int)((p->w[i >> 6] >> (i & 63)) & 1);
}

/*************************************************
* Name:        f1u64_eval_one
*
* Description: Evaluate a at X = 1 over F2 by computing the parity of all
*              coefficients. Since F1_N is a power of two,
*              X^F1_N + 1 = (X + 1)^F1_N over F2, so a is invertible in
*              F2[X]/(X^F1_N + 1) exactly when a(1) is 1.
*
* Arguments:   - const poly_f1_u64 *a: pointer to input polynomial
*
* Returns:     a(1), either 0 or 1
**************************************************/
static int f1u64_eval_one(const poly_f1_u64 *a)
{
    uint64_t v = 0;
    int i;
    for (i = 0; i < F1_NWORDS; i++)
        v ^= a->w[i];
    v ^= v >> 32; v ^= v >> 16; v ^= v >> 8;
    v ^= v >> 4;  v ^= v >> 2;  v ^= v >> 1;
    return (int)(v & 1);
}

/*************************************************
* Name:        f1u64_translate
*
* Description: Apply the binary Taylor shift f(X) -> f(X + 1) in place.
*              This involution changes variables between arithmetic modulo
*              X^F1_N + 1 = (X + 1)^F1_N and arithmetic modulo X^F1_N.
*
* Arguments:   - poly_f1_u64 *f: pointer to input/output polynomial
**************************************************/
static void f1u64_translate(poly_f1_u64 *f)
{
    int s, w, ws, i, j;
    for (s = 1; s < F1_N; s <<= 1) {
        if (s < 64) {
            uint64_t mask = ~0ULL / ((1ULL << s) + 1);
            for (w = 0; w < F1_NWORDS; w++)
                f->w[w] ^= (f->w[w] >> s) & mask;
        } else {
            ws = s >> 6;
            for (i = 0; i < F1_NWORDS; i += 2 * ws)
                for (j = 0; j < ws; j++)
                    f->w[i + j] ^= f->w[i + ws + j];
        }
    }
}

/*************************************************
* Name:        spread32
*
* Description: Insert one zero bit after every bit of x. This maps the
*              coefficient of X^i to X^(2i), thereby squaring a degree-<32
*              polynomial over F2.
*
* Arguments:   - uint32_t x: packed input coefficients
*
* Returns:     packed 64-bit polynomial with input bit i at output bit 2i
**************************************************/
static uint64_t spread32(uint32_t x)
{
    uint64_t v = x;
    v = (v | (v << 16)) & 0x0000FFFF0000FFFFULL;
    v = (v | (v <<  8)) & 0x00FF00FF00FF00FFULL;
    v = (v | (v <<  4)) & 0x0F0F0F0F0F0F0F0FULL;
    v = (v | (v <<  2)) & 0x3333333333333333ULL;
    v = (v | (v <<  1)) & 0x5555555555555555ULL;
    return v;
}

/*************************************************
* Name:        f1u64_square_trunc
*
* Description: Square a polynomial over F2 using its first prec valid
*              coefficients and retain terms of degree below trunc. The
*              truncation length must be even, as in all current callers.
*
* Arguments:   - poly_f1_u64 *r:       pointer to output polynomial
*              - const poly_f1_u64 *a: pointer to input polynomial
*              - int prec:             number of valid low coefficients in a
*              - int trunc:            even number of low output coefficients
*                                      to retain
**************************************************/
static void f1u64_square_trunc(poly_f1_u64 *r, const poly_f1_u64 *a,
                                int prec, int trunc)
{
    int k, nchunks, lim, rem;
    uint32_t bits;

    memset(r, 0, sizeof(*r));
    lim = prec < trunc / 2 ? prec : trunc / 2;
    nchunks = (lim + 31) / 32;

    for (k = 0; k < nchunks; k++) {
        bits = (k & 1)
            ? (uint32_t)(a->w[k >> 1] >> 32)
            : (uint32_t)(a->w[k >> 1]);
        rem = lim - k * 32;
        if (rem < 32)
            bits &= (1U << rem) - 1;
        r->w[k] = spread32(bits);
    }
}

/*************************************************
* Name:        f1u64_mul_trunc
*
* Description: Multiply a and b over F2 and retain terms of degree below
*              trunc, equivalently computing a*b modulo X^trunc
*
* Arguments:   - poly_f1_u64 *r:       pointer to output polynomial
*              - const poly_f1_u64 *a: pointer to first input polynomial
*              - const poly_f1_u64 *b: pointer to second input polynomial
*              - int trunc:            number of low output coefficients to
*                                      retain
**************************************************/
static void f1u64_mul_trunc(poly_f1_u64 *r, const poly_f1_u64 *a,
                             const poly_f1_u64 *b, int trunc)
{
    poly_f1_u64 tmp;
    int tw, i, w, ws, bs, lim;

    memset(&tmp, 0, sizeof(tmp));
    tw = (trunc + 63) >> 6;

    for (i = 0; i < trunc; i++) {
        if (!f1u64_getbit(b, i)) continue;
        ws = i >> 6;
        bs = i & 63;
        lim = tw - ws;
        if (lim > F1_NWORDS) lim = F1_NWORDS;

        if (bs == 0) {
            for (w = 0; w < lim; w++)
                tmp.w[w + ws] ^= a->w[w];
        } else {
            for (w = 0; w < lim; w++) {
                tmp.w[w + ws] ^= a->w[w] << bs;
                if (w + ws + 1 < tw)
                    tmp.w[w + ws + 1] ^= a->w[w] >> (64 - bs);
            }
        }
    }
    if (trunc & 63)
        tmp.w[tw - 1] &= (1ULL << (trunc & 63)) - 1;
    *r = tmp;
}

/*************************************************
* Name:        f1u64_newton_inv
*
* Description: Compute the inverse of fhat modulo X^F1_N by precision
*              doubling. The input must have constant coefficient 1.
*              In characteristic two, each iteration refines the inverse
*              with g <- fhat*g^2 modulo the new precision.
*
* Arguments:   - poly_f1_u64 *g:          pointer to output inverse
*              - const poly_f1_u64 *fhat: pointer to input polynomial
**************************************************/
static void f1u64_newton_inv(poly_f1_u64 *g, const poly_f1_u64 *fhat)
{
    poly_f1_u64 gsq;
    int prec, np;

    memset(g, 0, sizeof(*g));
    g->w[0] = 1;

    prec = 1;
    while (prec < F1_N) {
        np = prec << 1;
        if (np > F1_N) np = F1_N;

        f1u64_square_trunc(&gsq, g, prec, np);
        f1u64_mul_trunc(g, fhat, &gsq, np);

        prec = np;
    }
}

/*************************************************
* Name:        f1_pack
*
* Description: Pack the low bit of each int16_t coefficient into a
*              bit-packed polynomial over F2
*
* Arguments:   - poly_f1_u64 *dst:  pointer to output packed polynomial
*              - const poly_f1 *src: pointer to input coefficient polynomial
**************************************************/
static void f1_pack(poly_f1_u64 *dst, const poly_f1 *src)
{
    unsigned int i;
    memset(dst, 0, sizeof(*dst));
    for (i = 0; i < F1_N; i++) {
        if (src->coeffs[i] & 1)
            dst->w[i >> 6] |= 1ULL << (i & 63);
    }
}

/*************************************************
* Name:        f1_unpack
*
* Description: Unpack a bit-packed polynomial into int16_t coefficients,
*              writing each coefficient as either 0 or 1
*
* Arguments:   - poly_f1 *dst:       pointer to output coefficient polynomial
*              - const poly_f1_u64 *src: pointer to input packed polynomial
**************************************************/
static void f1_unpack(poly_f1 *dst, const poly_f1_u64 *src)
{
    unsigned int i;
    for (i = 0; i < F1_N; i++)
        dst->coeffs[i] = (int16_t)((src->w[i >> 6] >> (i & 63)) & 1);
}

/*************************************************
* Name:        poly_inv_in_F2
*
* Description: Fold f modulo 2 into F2[X]/(X^F1_N + 1) and compute the
*              inverse of the folded polynomial. On failure, f1 contains
*              the folded polynomial rather than a valid inverse.
*
* Arguments:   - poly_f1 *f1: pointer to output/scratch polynomial
*              - const poly *f: pointer to input coefficient polynomial
*
* Returns:     1 if the folded polynomial is invertible, otherwise 0
**************************************************/
int poly_inv_in_F2(poly_f1 *f1, const poly *f)
{
    unsigned int i;
    poly_f1_u64 bp, g;

#if KEM_MODE == 128
    for (i = 0; i < N / 2; i++)
        f1->coeffs[i] = (f->coeffs[i] + f->coeffs[i + N/2]) & 1;
#elif KEM_MODE == 256 || KEM_MODE == 512
    for (i = 0; i < N / 4; i++)
        f1->coeffs[i] = (f->coeffs[i] + f->coeffs[i + N/4] + f->coeffs[i + N/2] + f->coeffs[i + 3*N/4]) & 1;
#endif

    f1_pack(&bp, f1);

    if (!f1u64_eval_one(&bp))
        return 0;

    f1u64_translate(&bp);
    f1u64_newton_inv(&g, &bp);
    f1u64_translate(&g);
    f1_unpack(f1, &g);

    return 1;
}

/*************************************************
* Name:        f1u64_mul_ring
*
* Description: Multiply two bit-packed polynomials in
*              F2[X]/(X^F1_N + 1). The upper half of the carry-less product
*              is folded into the lower half with XOR.
*
* Arguments:   - poly_f1_u64 *r:       pointer to output polynomial
*              - const poly_f1_u64 *a: pointer to first input polynomial
*              - const poly_f1_u64 *b: pointer to second input polynomial
**************************************************/
static void f1u64_mul_ring(poly_f1_u64 *r,
                           const poly_f1_u64 *a,
                           const poly_f1_u64 *b)
{
    uint64_t prod[2 * F1_NWORDS + 1];
    uint64_t mask;
    int i, w, ws, bs;

    memset(prod, 0, sizeof(prod));

    for (i = 0; i < F1_N; i++) {
        mask = -(uint64_t)f1u64_getbit(b, i);
        ws = i >> 6;
        bs = i & 63;

        if (bs == 0) {
            for (w = 0; w < F1_NWORDS; w++)
                prod[w + ws] ^= a->w[w] & mask;
        } else {
            for (w = 0; w < F1_NWORDS; w++) {
                prod[w + ws]     ^= (a->w[w] << bs) & mask;
                prod[w + ws + 1] ^= (a->w[w] >> (64 - bs)) & mask;
            }
        }
    }

    if ((F1_N & 63) == 0) {
        int hw = F1_N >> 6;
        for (w = 0; w < F1_NWORDS; w++)
            r->w[w] = prod[w] ^ prod[w + hw];
    } else {
        int hw = F1_N >> 6;
        int hb = F1_N & 63;
        for (w = 0; w < F1_NWORDS; w++) {
            uint64_t upper = prod[w + hw] >> hb;
            if (w + hw + 1 < 2 * F1_NWORDS + 1)
                upper |= prod[w + hw + 1] << (64 - hb);
            r->w[w] = prod[w] ^ upper;
        }
        r->w[F1_NWORDS - 1] &= (1ULL << hb) - 1;
    }
}

/*************************************************
* Name:        poly_decode_to_msg
*
* Description: Decode a coefficient-domain decryption polynomial into a
*              message. The routine subtracts HALF_Q and reduces c to
*              centered representatives in place, applies the D2 threshold
*              for KEM_MODE 128 or the D4 threshold for KEM_MODE 256/512,
*              and multiplies the recovered binary polynomial by the
*              precomputed inverse f1.
*
* Arguments:   - uint8_t msg[]: output message byte array of
*                               KEM_MSGBYTES bytes
*              - poly *c: pointer to input/scratch decryption polynomial;
*                         each coefficient is replaced by the centered
*                         representative of c[i] - HALF_Q modulo Q
*              - poly_f1 *f1: pointer to the precomputed inverse of the
*                             folded secret polynomial over F2
**************************************************/
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1) {
#if KEM_MODE == 128
    unsigned int i;
    poly_f1 mf1;
    poly_f1_u64 mf1_packed, f1_packed, m_packed;

    poly_sub_halfq(c);

    for (i = 0; i < N / 2; i++) {
        uint16_t u0 = (uint16_t)c->coeffs[i];
        uint16_t u1 = (uint16_t)c->coeffs[i + N / 2];

        uint16_t a0 = (u0 ^ -(u0 >> 15)) + (u0 >> 15);
        uint16_t a1 = (u1 ^ -(u1 >> 15)) + (u1 >> 15);

        mf1.coeffs[i] = (int16_t)((a0 + a1 - (uint16_t)((Q - 1) / 2)) >> 15);
    }

    f1_pack(&mf1_packed, &mf1);
    f1_pack(&f1_packed, f1);
    f1u64_mul_ring(&m_packed, &mf1_packed, &f1_packed);

    for (i = 0; i < KEM_MSGBYTES; i++)
        msg[i] = (uint8_t)(m_packed.w[i >> 3] >> ((i & 7) << 3));

#elif KEM_MODE == 256 || KEM_MODE == 512
    unsigned int i;
    poly_f1 mf1;
    poly_f1_u64 mf1_packed, f1_packed, m_packed;

    poly_sub_halfq(c);

    for (i = 0; i < N / 4; i++) {
        uint16_t u0 = (uint16_t)c->coeffs[i];
        uint16_t u1 = (uint16_t)c->coeffs[i + N / 4];
        uint16_t u2 = (uint16_t)c->coeffs[i + N / 2];
        uint16_t u3 = (uint16_t)c->coeffs[i + 3 * N / 4];

        uint16_t a0 = (u0 ^ -(u0 >> 15)) + (u0 >> 15);
        uint16_t a1 = (u1 ^ -(u1 >> 15)) + (u1 >> 15);
        uint16_t a2 = (u2 ^ -(u2 >> 15)) + (u2 >> 15);
        uint16_t a3 = (u3 ^ -(u3 >> 15)) + (u3 >> 15);

        mf1.coeffs[i] = (int16_t)((a0 + a1 + a2 + a3 - (uint16_t)(Q - 1)) >> 15);
    }

    f1_pack(&mf1_packed, &mf1);
    f1_pack(&f1_packed, f1);
    f1u64_mul_ring(&m_packed, &mf1_packed, &f1_packed);

    for (i = 0; i < KEM_MSGBYTES; i++)
        msg[i] = (uint8_t)(m_packed.w[i >> 3] >> ((i & 7) << 3));
#endif
}
