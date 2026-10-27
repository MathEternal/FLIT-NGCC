#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

/*
 * Bit-packed polynomial over F2 with F1_N coefficients. Coefficient i is
 * stored in bit (i & 63) of word (i >> 6).
 */
typedef struct {
    uint64_t w[F1_N_WORDS];
} poly_f1_u64;

/*
 * Unpacked polynomial over F2 used at the API boundary. Each coefficient is
 * represented as a uint8_t with value 0 or 1.
 */
typedef struct{
  uint8_t coeffs[F1_N];
} poly_f1;

#if KEM_MODE == 128

/*
 * Fold f modulo 2 and invert it in F2[X]/(X^F1_N + 1). Returns 1 on
 * success and 0 on failure. On failure, f1 contains the folded polynomial,
 * not a valid inverse.
 */
#define poly_inv_in_F2 KEM_NAMESPACE(poly_inv_in_F2)
int poly_inv_in_F2(poly_f1 *f1, const poly *f);

/*
 * Decode a coefficient-domain decryption polynomial without modifying f1.
 * Each c[i] is replaced by the centered representative of
 * c[i] - HALF_Q modulo Q.
 */
#define poly_decode_to_msg KEM_NAMESPACE(poly_decode)
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1);

#elif KEM_MODE == 256 || KEM_MODE == 512

/*
 * Fold f modulo 2 and invert it in F2[X]/(X^F1_N + 1). Returns 1 on
 * success and 0 on failure. On failure, f1 contains the folded polynomial,
 * not a valid inverse.
 */
#define poly_inv_in_F2 KEM_NAMESPACE(poly_inv_in_F2)
int poly_inv_in_F2(poly_f1 *f1, const poly *f);

/*
 * Decode a coefficient-domain decryption polynomial without modifying f1.
 * Each c[i] is replaced by the centered representative of
 * c[i] - HALF_Q modulo Q.
 */
#define poly_decode_to_msg KEM_NAMESPACE(poly_decode)
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1);

#endif

#endif
