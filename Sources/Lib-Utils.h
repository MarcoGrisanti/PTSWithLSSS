#include <fcntl.h>
#include <gmp.h>
#include <math.h>
#include <nettle/sha2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* getBinaryString(char* s);
void concatenate (mpz_t result, mpz_t a, mpz_t b);
void H(mpz_t digest, uint8_t* message, int messageLenght, mpz_t modulus);
void HF(mpz_t digest, uint8_t* message, int messageLength);
void Determinant(mpz_t determinant, mpz_t** A, int N, mpz_t modulus);
void VDeterminant(mpz_t det, mpz_t** M, int N, mpz_t mod);
int extractRandSeed(uint8_t* seed, size_t seedBits);
int gmpRandSeed(gmp_randstate_t state, size_t bits);