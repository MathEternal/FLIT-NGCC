#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

/*
 * Bit-packed polynomial over F2 with F1_N coefficients. Coefficient i is
 * stored in bit (i & 63) of word (i >> 6); storage is 32-byte aligned.
 */
typedef struct {
  uint64_t w[F1_N_WORDS] __attribute__((aligned(32)));
} poly_f1;

/* Internal alias used by the bit-packed arithmetic helpers. */
typedef poly_f1 poly_f1_u64;
#if KEM_MODE == 128

/*
 * Fold f modulo 2 and invert it in F2[X]/(X^F1_N + 1). Returns 1 and
 * writes f1 on success; returns 0 and leaves f1 unchanged.
 */
#define poly_inv_in_F2 KEM_NAMESPACE(poly_inv_in_F2)
int poly_inv_in_F2(poly_f1 *f1, const poly *f);

/*
 * Decode a coefficient-domain decryption polynomial using the precomputed
 * inverse f1. Writes KEM_MSGBYTES bytes to msg and modifies neither c nor f1.
 */
#define poly_decode_to_msg KEM_NAMESPACE(poly_decode)
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1);

#elif KEM_MODE == 256 || KEM_MODE == 512

/*
 * Fold f modulo 2 and invert it in F2[X]/(X^F1_N + 1). Returns 1 and
 * writes f1 on success; returns 0 and leaves f1 unchanged.
 */
#define poly_inv_in_F2 KEM_NAMESPACE(poly_inv_in_F2)
int poly_inv_in_F2(poly_f1 *f1, const poly *f);

/*
 * Decode a coefficient-domain decryption polynomial using the precomputed
 * inverse f1. Writes KEM_MSGBYTES bytes to msg and modifies neither c nor f1.
 */
#define poly_decode_to_msg KEM_NAMESPACE(poly_decode)
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1);

#endif

#endif
