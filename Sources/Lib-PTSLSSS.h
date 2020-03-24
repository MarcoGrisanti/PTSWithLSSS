#include "Lib-Mesg.h"
#include "Lib-Utils.h"

#define PRIME_NUMBER_TEST_ITERATIONS 12

struct keysStruct {
    unsigned int modulusBits;
    int N;
    int T;
    mpz_t n;
    mpz_t m;
    mpz_t e;
    mpz_t d;
    mpz_t g; // v
    mpz_t* x;
    mpz_t* y;
    mpz_t* v;
    mpz_t** A;
};
typedef struct keysStruct *keysPtr;
typedef struct keysStruct keys_t[1];

struct messageStruct {
    uint8_t* m;
    int lenght;
};
typedef struct messageStruct *messagePtr;
typedef struct messageStruct message_t[1];

struct signStruct {
    mpz_t s;
};
typedef struct signStruct *signPtr;
typedef struct signStruct sign_t[1];

void keysInit(keys_t keys, int modulusBits, int N, int T);
void keysClear(keys_t keys);
void signInit(sign_t ciphertext);
void signClear(sign_t ciphertext);
void generateKeys(keys_t keys, gmp_randstate_t prng);
void sign(sign_t signature, message_t message, keys_t keys, int* S, int SLen, gmp_randstate_t prng);
void verify(message_t message, sign_t signature, keys_t keys);