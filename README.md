# FLIT-NGCC

This repository contains the updated implementations and documentation
corrections for FLIT, a lattice-based key-encapsulation mechanism submitted to
the NGCC program.

## Contents

- `Algorithm specifications/` — the submitted algorithm specification.
- `Implementations/Reference_Implementation/` — portable C implementations
  for FLIT128, FLIT256, and FLIT512.
- `Implementations/Optimized_Implementation/` — AVX2 implementations for
  FLIT128, FLIT256, and FLIT512, including assembly NTT and reduction kernels.
- `Implementations/Additional_Implementation/` — FIPS 202/SHAKE variants at
  the 128-bit and 256-bit security levels, in reference and AVX2 forms.
- `Test_Vectors/` — deterministic known-answer test (KAT) vectors.

See [`Implementations/README.md`](Implementations/README.md) for the source
layout, build commands, and test programs.

## Quick check

Each implementation directory has its own `Makefile`. For example:

```sh
cd Implementations/Reference_Implementation/FLIT128
make clean
make test
./test/test_mul
./test/test_cpapke
./test/test_kem
make
./KAT_KEM
```

## Published KAT checksums

Reference and optimized implementations produce identical KAT output for each
listed parameter set. The canonical published vectors are in `Test_Vectors/`;
per-implementation `output/` directories are generated locally and are not
tracked.


## Specification corrections

The following corrections apply to
`Algorithm specifications/Algorithm specification.pdf`. They do not change
the parameter sets implemented by the source code.

### Complete KEM secret-key sizes

The complete KEM secret-key sizes in Table 11 should be:

| Parameter set | Correct size (bytes) | Implemented formula |
|:---:|:---:|:---:|
| FLIT128 | 1351 | `640 + 32 + 615 + 64` |
| FLIT256 | 2605 | `1280 + 32 + 1229 + 64` |
| FLIT512 | 6336 | `3072 + 64 + 3072 + 128` |

The implementation stores the NTT-domain secret polynomial using `Q_BITS`
bits per coefficient. The corresponding definitions are
`KEM_CPAPKE_SECRETKEYBYTES = N * Q_BITS / 8 + SEEDBYTES` and
`KEM_SECRETKEYBYTES = KEM_CPAPKE_SECRETKEYBYTES +
KEM_CPAPKE_PUBLICKEYBYTES + 2 * SEEDBYTES` in `params.h`.
