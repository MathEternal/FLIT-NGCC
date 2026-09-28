# FLIT specification errata

This file records known corrections to
`Algorithm specifications/Algorithm specification.pdf`. These corrections do
not change the parameter sets implemented by the source code. Open items are
identified explicitly below.

## Complete KEM secret-key sizes

The complete KEM secret-key sizes in Table 11 should be:

| Parameter set | Correct size (bytes) | Implemented formula |
|---|---:|---|
| FLIT128 | 1351 | `640 + 32 + 615 + 64` |
| FLIT256 | 2605 | `1280 + 32 + 1229 + 64` |
| FLIT512 | 6336 | `3072 + 64 + 3072 + 128` |

The implementation stores the NTT-domain secret polynomial using `Q_BITS`
bits per coefficient. The corresponding definitions are
`KEM_CPAPKE_SECRETKEYBYTES = N * Q_BITS / 8 + SEEDBYTES` and
`KEM_SECRETKEYBYTES = KEM_CPAPKE_SECRETKEYBYTES +
KEM_CPAPKE_PUBLICKEYBYTES + 2 * SEEDBYTES` in `params.h`.

## Recursive Karatsuba pseudocode

In Algorithm 1, the three recursive child products must not be reduced modulo
`X^(k/2) - zeta` before they are recombined. They are ordinary child
polynomial products used to form the low and high halves; reduction modulo the
parent relation is applied after recombination. The C implementations follow
this ordering, so this is a specification-pseudocode correction rather than a
change to the implemented computation.

## DFR independence assumption (open analysis item)

The decryption-failure-rate discussion treats repeated decoding coordinates as
independent when forming a product-style probability estimate. Those
coordinates are derived from shared sampled polynomials, so independence does
not follow automatically. The numerical estimate should therefore be treated
as requiring a dependence-aware justification; functional testing does not
close this proof gap.

## KEM wrapper length validation (open implementation item)

The NGCC harness functions in `KEM_AlgorithmInstance.c` expose input-length
arguments for public keys, secret keys, and ciphertexts, but the current
wrapper does not validate those arguments before calling the fixed-size core
KEM API. Callers must allocate buffers using the `kem_get_*_len_bytes()`
functions and pass exactly those lengths. Length validation remains open; the
interface file is retained in its submission-compatible form.

## Protected rotate macro and compiler portability (open)

The submission-controlled `drng.c` and `auxfunc.c` define a 32-bit rotate
macro whose `n = 0` case evaluates a right shift by 32. That operation is
undefined in C. GCC/UBSan reports it in `drng.c` at the two SM3 round-constant
branches (currently lines 90 and 94), and in `auxfunc.c` at the corresponding
two branches (currently lines 84 and 88). These files are listed as files that
must not be modified in the supplied `README.txt`, so they are preserved here
unchanged.

All release correctness tests passed with GCC 11.4 on x86-64 Linux/WSL2. A
previous Apple Clang 21 `-O2` run reported a FLIT512 KEM failure; Apple Clang
was not available in the release-test environment, so that behavior remains
unresolved. Do not assume cross-compiler equivalence until the protected
rotate implementation is replaced or corrected by the interface provider.
