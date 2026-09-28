#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../params.h"
#include "../indcpa.h"
#include "../rng.h"
#include "../drng.h"

DRNG_ctx drng_algorithm;

#define MLEN KEM_CPAPKE_MSGBYTES
#ifndef NTESTS
#define NTESTS 1000
#endif

int main(void)
{
    unsigned int i, j;
    uint8_t m[MLEN];
    uint8_t m2[MLEN];
    uint8_t c[KEM_CPAPKE_CIPHERTEXTBYTES];
    uint8_t coins[SEEDBYTES];
    uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES];
    uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES];

    unsigned char init_seed[64];
    for (unsigned int k = 0; k < sizeof(init_seed); k++)
        init_seed[k] = (unsigned char)k;
    init_random_number(&drng_algorithm, init_seed, sizeof(init_seed));
    randombytes_init(init_seed, NULL, 256);

    for (i = 0; i < NTESTS; ++i) {
        // Generate random message
        randombytes(m, MLEN);
        randombytes(coins, SEEDBYTES);

        // Generate keypair
        indcpa_keypair(pk, sk);
        
        // Encrypt
        indcpa_enc(c, m, pk, coins);
        
        // Decrypt
        indcpa_dec(m2, c, sk);

        // Check if decrypted message matches original
        if (memcmp(m, m2, MLEN) != 0) {
            fprintf(stderr, "Decryption failed at test %d\n", i);
            fprintf(stderr, "Original:  ");
            for (j = 0; j < MLEN; j++) fprintf(stderr, "%02x", m[j]);
            fprintf(stderr, "\n");
            fprintf(stderr, "Decrypted: ");
            for (j = 0; j < MLEN; j++) fprintf(stderr, "%02x", m2[j]);
            fprintf(stderr, "\n");
            return -1;
        }
    }

    printf("All %d encryption/decryption tests passed!\n", NTESTS);
    
    printf("KEM_CPAPKE_PUBLICKEYBYTES = %d\n", KEM_CPAPKE_PUBLICKEYBYTES);
    printf("KEM_CPAPKE_SECRETKEYBYTES = %d\n", KEM_CPAPKE_SECRETKEYBYTES);
    printf("KEM_CPAPKE_CIPHERTEXTBYTES = %d\n", KEM_CPAPKE_CIPHERTEXTBYTES);
    printf("KEM_CPAPKE_MSGBYTES = %d\n", KEM_CPAPKE_MSGBYTES);

    return 0;
}
