#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#if KEM_MODE == 128
/*
 * Unpacked polynomial over F2 used by the decoder. Each coefficient is
 * represented as an int16_t with value 0 or 1; there are N/2 coefficients.
 */
typedef struct{
  int16_t coeffs[N/2];
} poly_f1;

/*
 * Fold f modulo 2 and invert it in the binary quotient ring. Returns 1 on
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
 * Unpacked polynomial over F2 used by the decoder. Each coefficient is
 * represented as an int16_t with value 0 or 1; there are N/4 coefficients.
 */
typedef struct{
  int16_t coeffs[N/4];
} poly_f1;

/*
 * Fold f modulo 2 and invert it in the binary quotient ring. Returns 1 on
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
