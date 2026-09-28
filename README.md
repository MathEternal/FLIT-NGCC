# FLIT-NGCC

This repository contains the updated implementations and errata for FLIT, a
lattice-based key-encapsulation mechanism submitted to the NGCC program.

## Contents

- `Algorithm specifications/` — the submitted algorithm specification.
- `Implementations/Reference_Implementation/` — portable C implementations
  for FLIT128, FLIT256, and FLIT512.
- `Implementations/Optimized_Implementation/` — AVX2 implementations for
  FLIT128, FLIT256, and FLIT512, including assembly NTT and reduction kernels.
- `Implementations/Additional_Implementation/` — FIPS 202/SHAKE variants at
  the 128-bit and 256-bit security levels, in reference and AVX2 forms.
- `Test_Vectors/` — deterministic known-answer test (KAT) vectors.
- `ERRATA.md` — corrections to the submitted specification.

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

For an optimized implementation, also run `./test/test_ntt_avx`. Optimized
builds require an x86-64 processor with AVX2, BMI2, POPCNT, and PCLMULQDQ
support. The build defaults to GCC and a Linux-like environment.
The optimized assembly uses GNU/ELF directives and is not an Apple Silicon or
generic macOS target.

The release validation described here used GCC 11.4 on x86-64 Linux/WSL2.
Cross-compiler portability is not currently claimed; see the protected
rotate-macro note in [`ERRATA.md`](ERRATA.md).

## Published KAT checksums

Reference and optimized implementations produce identical KAT output for each
listed parameter set. The canonical published vectors are in `Test_Vectors/`;
per-implementation `output/` directories are generated locally and are not
tracked.

| Variant | SHA-256 |
|---|---|
| FLIT128 | `a772deebcc67af946d60d9ebab8014318879617de619692bd59650196b578e69` |
| FLIT256 | `0788e13e3a0c0ff0c8ada4defab0ea4213bf7980bcb6099dba1a2e35b05ea618` |
| FLIT512 | `e4019117230dc2ef5983e8c2c252f08189e8004e3b7db595a66eec3aa8d9820c` |
| FLIT-FIPS202-128 | `dbbe814ecbb2954adb09ef7d9bef30da281c6e7a18828d3cdba423f262dd` |
| FLIT-FIPS202-256 | `58792b5969c2e79ef899f91bb912e21d02c6b0b6aeaf1afefcb6948d6da39333` |

The specification PDF should be read together with [`ERRATA.md`](ERRATA.md).

## Licensing

No repository-wide license is currently provided. Files that carry their own
copyright or license notices remain subject to those notices.
