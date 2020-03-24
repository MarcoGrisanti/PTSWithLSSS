#include "Lib-Utils.h"

char* getBinaryString(char* s) {
    if(s == NULL) {
        printf("Error: getBinaryString\n");
        exit(1);
    }
    char* binaryString = malloc(strlen(s) * 8 + 1);
    binaryString[0] = '\0';
    for(int i = 0; i < strlen(s); i++) {
        char c = s[i];
        for(int j = 7; j >= 0; j--) {
            if (c & (1 << j)) strcat(binaryString, "1");
            else strcat(binaryString, "0");
        }
    }
    return binaryString;
}

void concatenate (mpz_t result, mpz_t a, mpz_t b) {
    int size;
    mpz_t tmp;

    mpz_init(tmp);

    size = mpz_sizeinbase(b, 10);
    mpz_ui_pow_ui(tmp, 10, size);
    mpz_mul(tmp, tmp, a);
    mpz_add(tmp, tmp, b);
    mpz_set(result, tmp);

    mpz_clear(tmp);
}

void H(mpz_t digest, uint8_t* message, int messageLenght, mpz_t modulus) {
    uint8_t hashMessage[messageLenght + 1];
    uint8_t hashDigest[SHA256_DIGEST_SIZE];
    uint8_t i;
    mpz_t tmp;

    mpz_init(tmp);

    i = 1;
    while (1 == 1) {
        strcpy(hashMessage, message);
        hashMessage[messageLenght] = i;
        struct sha256_ctx context;
        sha256_init(&context);
        sha256_update(&context, messageLenght + 1, hashMessage);
        sha256_digest(&context, SHA256_DIGEST_SIZE, hashDigest);
        
        mpz_import(digest, SHA256_DIGEST_SIZE, 1, sizeof(uint8_t), 0, 0, hashDigest);
        
        mpz_mod(digest, digest, modulus);
        mpz_gcd(tmp, digest, modulus);
        if (mpz_cmp_ui(tmp, 1) == 0) break;
        i++;
    }

    mpz_clear(tmp);
}

void HF(mpz_t digest, uint8_t* message, int messageLength) {
    uint8_t hashDigest[SHA256_DIGEST_SIZE];
    struct sha256_ctx context;
    sha256_init(&context);
    sha256_update(&context, messageLength, message);
    sha256_digest(&context, SHA256_DIGEST_SIZE, hashDigest);
    mpz_import(digest, SHA256_DIGEST_SIZE, 1, sizeof(uint8_t), 0, 0, hashDigest);
}

void Determinant(mpz_t determinant, mpz_t** A, int N, mpz_t modulus) {
    mpz_t** M;
    mpz_t* tmp;
    mpz_t c;

    M = (mpz_t**) malloc(sizeof(mpz_t*) * N);
    for (int i = 0; i < N; i++) {
        M[i] = (mpz_t*) malloc(sizeof(mpz_t) * N);
        for (int j = 0; j < N; j++) {
            mpz_init(M[i][j]);
            mpz_set(M[i][j], A[i][j]);
        }
    }

    tmp = (mpz_t*) malloc(sizeof(mpz_t) * N);
    for (int i = 0; i < N; i++) mpz_init(tmp[i]);

    mpz_init(c);

    mpz_set_ui(determinant, 1);
    
    for (int i = 0; i < N - 1; i++) {
        for (int j = i + 1; j < N; j++) {
            while (mpz_cmp_ui(M[j][i], 0) != 0) {
                int sign = 1;
                int nextStep = 1;
                int NonZeroRowIndex = j;
                while (mpz_cmp_ui(M[i][i], 0) == 0) {
                    sign = sign * -1;
                    for (int k = 0; k < N; k++) {
                        mpz_set(tmp[k], M[NonZeroRowIndex][k]);
                        mpz_set(M[NonZeroRowIndex][k], M[i][k]);
                        mpz_set(M[i][k], tmp[k]);
                    }
                    NonZeroRowIndex++;
                    if (mpz_cmp_ui(M[i][i], 0) != 0) {
                        nextStep = 0;
                        break;
                    }
                    if (NonZeroRowIndex == N && mpz_cmp(M[i][i], 0) == 0) {
                        sign = 0;
                        nextStep = 0;
                        break;
                    }
                }
                if (nextStep == 1) {
                    if (mpz_cmpabs(M[i][i], M[j][i]) > 0) {
                        sign = sign * -1;
                        for (int k = 0; k < N; k++) {
                            mpz_set(tmp[k], M[j][k]);
                            mpz_set(M[j][k], M[i][k]);
                            mpz_set(M[i][k], tmp[k]);
                        }
                    }
                    mpz_set_ui(c, 1);
                    if (mpz_cmp_ui(M[i][i], 0) != 0) {
                        mpz_set(tmp[0], M[j][i]);
                        mpz_mul_si(tmp[0], tmp[0], -1);
                        mpz_cdiv_q(c, tmp[0], M[i][i]);
                    }
                    for (int k = 0; k < N; k++) {
                        mpz_mul(tmp[0], c, M[i][k]);
                        mpz_add(M[j][k], M[j][k], tmp[0]);
                    }
                }
                mpz_mul_si(determinant, determinant, sign);
                if (mpz_cmp_ui(determinant, 0) == 0) {
                    return;
                }
            }
        }
    }

    for (int i = 0; i < N; i++) mpz_mul(determinant, determinant, M[i][i]);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            mpz_clear(M[i][j]);
        }
        free(M[i]);
    }
    free(M);

    for (int i = 0; i < N; i++) mpz_clear(tmp[i]);

    mpz_clear(c);
}

void VDeterminant(mpz_t det, mpz_t** M, int N, mpz_t mod) {
    if (N == 1) mpz_set(det, M[0][0]);
    else {
        mpz_t tmp;
        mpz_init(tmp);
        mpz_set_ui(det, 1);
        for (int i = N - 1; i > 0; i--) {
            for (int j = i - 1; j >= 0; j--) {
                mpz_sub(tmp, M[i][1], M[j][1]);
                mpz_mul(det, det, tmp);
            }
        }
        mpz_clear(tmp);
    }
}

int extractRandSeed(uint8_t* seed, size_t seedBits) {
    long seedBytes = (size_t)ceil(seedBits / 8.0);
    int fileDescriptor;
    if ((fileDescriptor = open("/dev/random", O_RDONLY)) == -1) {
        printf("Error! Function open()\n");
        return -1;
    }
    for (long i = 0; i < seedBytes; i++) {
        if (read(fileDescriptor, seed + i, sizeof(char)) == -1) {
            close(fileDescriptor);
            printf("Error! Function read()\n");
            return -1;
        }
    }
    close(fileDescriptor);
    return 0;
}

int gmpRandSeed(gmp_randstate_t state, size_t bits) {
    long seedBytes = (size_t)ceil(bits / 8.0);
    mpz_t seed;
    uint8_t buffer[seedBytes];
    if (extractRandSeed(buffer, bits) == -1) {
        printf("Error! Function extractRandSeed()");
        return -1;
    }
    mpz_init(seed);
    mpz_import(seed, seedBytes, -1, sizeof(char), 0, 0, buffer);
    gmp_randseed(state, seed);
    mpz_clear(seed);
    return 0;
}