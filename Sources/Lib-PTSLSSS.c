#include "Lib-PTSLSSS.h"

void keysInit(keys_t keys, int modulusBits, int N, int T) {
    keys->modulusBits = modulusBits;
    keys->N = N;
    keys->T = T;

    mpz_inits(keys->n, keys->m, keys->e, keys->d, keys->g, NULL);
    
    keys->A = (mpz_t**) malloc(sizeof(mpz_t*) * keys->N);
    for (int i = 0; i < keys->N; i++) {
        keys->A[i] = (mpz_t*) malloc(sizeof(mpz_t) * keys->T);
        for (int j = 0; j < keys->T; j++) {
            mpz_init(keys->A[i][j]);
        }
    }

    keys->x = (mpz_t*) malloc(sizeof(mpz_t) * keys->T);
    for (int i = 0; i < keys->T; i++) mpz_init(keys->x[i]);

    keys->y = (mpz_t*) malloc(sizeof(mpz_t) * keys->N);
    for (int i = 0; i < keys->N; i++) mpz_init(keys->y[i]);

    keys->v = (mpz_t*) malloc(sizeof(mpz_t) * keys->N);
    for (int i = 0; i < keys->N; i++) mpz_init(keys->v[i]);
}

void keysClear(keys_t keys) {
    mpz_clears(keys->n, keys->m, keys->e, keys->d, keys->g, NULL);

    for (int i = 0; i < keys->N; i++) {
        for (int j = 0; j < keys->T; j++) {
            mpz_clear(keys->A[i][j]);
        }
        free(keys->A[i]);
    }
    free(keys->A);

    for (int i = 0; i < keys->T; i++) mpz_clear(keys->x[i]);
    free(keys->x);
    
    for (int i = 0; i < keys->N; i++) mpz_clear(keys->y[i]);
    free(keys->y);
    
    for (int i = 0; i < keys->N; i++) mpz_clear(keys->v[i]);
    free(keys->v);
}

void signInit(sign_t ciphertext) {
    mpz_init(ciphertext->s);
}

void signClear(sign_t ciphertext) {
    mpz_clear(ciphertext->s);
}

void generateKeys(keys_t keys, gmp_randstate_t prng) {
    mpz_t p0;
    mpz_t q0;
    mpz_t p;
    mpz_t q;
    mpz_t tmp1;
    mpz_t tmp2;

    pmesg(msg_normal, "\nKeys Generation");

    /* Start Inits */

    mpz_inits(p0, q0, p, q, tmp1, tmp2, NULL);

    int pBits = keys->modulusBits >> 1;
    int qBits = keys->modulusBits - pBits;

    /* End Inits */

    /* Start p And p0 Generation */
    
    do {
        do mpz_urandomb(p0, prng, pBits - 1);
        while ((mpz_sizeinbase(p0, 2) < pBits - 1) || !mpz_probab_prime_p(p0, PRIME_NUMBER_TEST_ITERATIONS));
        mpz_mul_ui(tmp1, p0, 2);
        mpz_add_ui(p, tmp1, 1);
    } while (!mpz_probab_prime_p(p, PRIME_NUMBER_TEST_ITERATIONS));

    /* End p And p0 Generation */
    
    /* Start q And q0 Generation */
    
    do {
        do mpz_urandomb(q0, prng, qBits - 1);
        while ((mpz_sizeinbase(q0, 2) < qBits - 1) || !mpz_probab_prime_p(q0, PRIME_NUMBER_TEST_ITERATIONS));
        mpz_mul_ui(tmp1, q0, 2);
        mpz_add_ui(q, tmp1, 1);
    } while ((mpz_cmp(p, q) == 0) || !mpz_probab_prime_p(q, PRIME_NUMBER_TEST_ITERATIONS));

    /* End q And q0 Generation */

    mpz_mul(keys->m, p0, q0);
    mpz_mul(keys->n, p, q);
    
    /* Start e And d Generation */

    do {
        mpz_urandomm(keys->e, prng, keys->m);
        mpz_gcd(tmp1, keys->e, keys->m);
    } while ((mpz_cmp_ui(keys->e, 0) == 0) || (mpz_cmp_ui(tmp1, 1) != 0) || mpz_cmp_ui(keys->e, keys->N) <= 0 || !mpz_probab_prime_p(keys->e, PRIME_NUMBER_TEST_ITERATIONS));

    mpz_invert(keys->d, keys->e, keys->m);
    
    /* End e And d Generation */

    /* Start A** Generation */
    
    for (int i = 0; i < keys->N; i++) {
        do {
            mpz_urandomm(tmp1, prng, keys->m);
            mpz_gcd(tmp2, tmp1, keys->m);
        } while ((mpz_cmp_ui(tmp1, 0) == 0) || (mpz_cmp_ui(tmp2, 1) != 0));
        for (int j = 0; j < keys->T; j++) {
            mpz_powm_ui(keys->A[i][j], tmp1, j, keys->m);
        }
    }

    /* End A** Generation */

    /* Start x Generation */

    mpz_set(keys->x[0], keys->d);

    for (int i = 1; i < keys->T; i++) {
        do {
            mpz_urandomm(keys->x[i], prng, keys->m);
            mpz_gcd(tmp1, keys->x[i], keys->m);
        } while ((mpz_cmp_ui(keys->x[i], 0) == 0) || (mpz_cmp_ui(tmp1, 1) != 0));
    }
    
    /* End x* Generation*/

    /* Start y* Generation */

    for (int i = 0; i < keys->N; i++) {
        mpz_set_ui(tmp1, 0);
        for (int j = 0; j < keys->T; j++) {
            mpz_mul(tmp2, keys->A[i][j], keys->x[j]);
            mpz_mod(tmp2, tmp2, keys->m);
            mpz_add(tmp2, tmp2, tmp1);
            mpz_mod(tmp2, tmp2, keys->m);
            mpz_set(tmp1, tmp2);
        }
        mpz_set(keys->y[i], tmp1);
    }
    
    /* End y* Generation*/

    /* Start g Generation */

    do {
        mpz_urandomm(tmp1, prng, keys->n);
        mpz_gcd(tmp2, tmp1, keys->n);
    } while ((mpz_cmp_ui(tmp1, 0) == 0) || (mpz_cmp_ui(tmp1, 1) == 0) || (mpz_cmp_ui(tmp2, 1) != 0));

    mpz_powm_ui(keys->g, tmp1, 2, keys->n);

    /* End g Generation */

    /* Start v* Generation */
    
    for (int i = 0; i < keys->N; i++) mpz_powm(keys->v[i], keys->g, keys->y[i], keys->n);
    
    /* End v* Generation*/

    /* Start Print */

    pmesg_mpz(msg_verbose, "p0", p0);
    pmesg_mpz(msg_verbose, "q0", q0);
    pmesg_mpz(msg_verbose, "p", p);
    pmesg_mpz(msg_verbose, "q", q);
    pmesg_mpz(msg_verbose, "n", keys->n);
    pmesg_mpz(msg_verbose, "m", keys->m);
    pmesg_mpz(msg_verbose, "e", keys->e);
    pmesg_mpz(msg_verbose, "d", keys->d);
    pmesg_mpz(msg_verbose, "v", keys->g);

    for (int i = 0; i < keys->N; i++) {
        for (int j = 0; j < keys->T; j++) {
            char name[64];
            sprintf(name, "A[%d][%d]", i, j);
            pmesg_mpz(msg_verbose, name, keys->A[i][j]);
        }
    }

    for (int i = 0; i < keys->T; i++) {
        char name[64];
        sprintf(name, "x[%d]", i);
        pmesg_mpz(msg_verbose, name, keys->x[i]);
    }

    for (int i = 0; i < keys->N; i++) {
        char name[64];
        sprintf(name, "y[%d]", i);
        pmesg_mpz(msg_verbose, name, keys->y[i]);
    }

    for (int i = 0; i < keys->N; i++) {
        char name[64];
        sprintf(name, "v[%d]", i);
        pmesg_mpz(msg_verbose, name, keys->v[i]);
    }

    /* End Print */

    /* Start Clear */

    mpz_inits(p0, q0, p, q, tmp1, tmp2, NULL);

    /* End Clear */

}

void sign(sign_t signature, message_t message, keys_t keys, int* S, int SLen, gmp_randstate_t prng) {
    mpz_t w;
    mpz_t** AS;
    mpz_t*** partialAS;
    mpz_t* partialASDet;
    mpz_t* cofactorAS;
    mpz_t* s;
    mpz_t* r;
    mpz_t* st;
    mpz_t* vf;
    mpz_t* sf;
    mpz_t* D;
    mpz_t** DArgs;
    mpz_t* sigma;
    mpz_t* DVer;
    mpz_t** DVerArgs;
    mpz_t sm;
    mpz_t smv;
    mpz_t Delta;
    mpz_t a;
    mpz_t b;
    mpz_t tmp1;
    mpz_t tmp2;
    mpz_t tmp3;
    mpz_t tmp4;

    pmesg(msg_normal, "\nSign");

    /* Start Inits */

    mpz_inits(w, sm, smv, Delta, a, b, tmp1, tmp2, tmp3, tmp4, NULL);

    AS = (mpz_t**) malloc(sizeof(mpz_t*) * keys->T);
    for (int i = 0; i < keys->T; i++) {
        AS[i] = (mpz_t*) malloc(sizeof(mpz_t) * keys->T);
        for (int j = 0; j < keys->T; j++) {
            mpz_init(AS[i][j]);
        }
    }

    partialAS = (mpz_t***) malloc(sizeof(mpz_t**) * SLen);
    for (int k = 0; k < SLen; k++) {
        partialAS[k] = (mpz_t**) malloc(sizeof(mpz_t*) * keys->T - 1);
        for (int i = 0; i < keys->T - 1; i++) {
            partialAS[k][i] = (mpz_t*) malloc(sizeof(mpz_t) * keys->T - 1);
            for (int j = 0; j < keys->T - 1; j++) {
                mpz_init(partialAS[k][i][j]);
            }
        }
    }

    partialASDet = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(partialASDet[i]);

    cofactorAS = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(cofactorAS[i]);

    s = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(s[i]);

    r = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(r[i]);

    st = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(st[i]);

    vf = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(vf[i]);

    sf = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(sf[i]);

    D = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(D[i]);

    DArgs = (mpz_t**) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) {
        DArgs[i] = (mpz_t*) malloc(sizeof(mpz_t) * 6);
        for (int j = 0; j < 6; j++) {
            mpz_init(DArgs[i][j]);
        }
    }

    sigma = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(sigma[i]);

    DVer = (mpz_t*) malloc(sizeof(mpz_t) * SLen);
    for (int i = 0; i < SLen; i++) mpz_init(DVer[i]);

    DVerArgs = (mpz_t**) malloc(sizeof(mpz_t*) * SLen);
    for (int i = 0; i < SLen; i++) {
        DVerArgs[i] = (mpz_t*) malloc(sizeof(mpz_t) * 6);
        for (int j = 0; j < 6; j++) {
            mpz_init(DVerArgs[i][j]);
        }
    }

    /* End Inits */

    /* Start Message Hash */

    H(w, message->m, message->lenght, keys->n);

    /* End Message Hash */

    /* Start AS** Computation */

    for (int i = 0; i < keys->T; i++) {
        for (int j = 0; j < keys->T; j++) {
            mpz_set(AS[i][j], keys->A[S[i]][j]);
        }
    }
    
    /* End AS** Matrix Computation */

    /* Start partialAS*** Computation */

    for (int k = 0; k < SLen; k++) {
        for (int i = 0, a = 0; i < keys->T; i++, a++) {
            if (i == k) {
                i++;
                if (i == keys->T) break;
            }
            for (int j = 1, b = 0; j < keys->T; j++, b++) {
                mpz_set(partialAS[k][a][b], AS[i][j]);
            }
        }
    }

    /* End partialAS*** Computation */

    /* Start partialASDet* Computation */

    for (int i = 0; i < SLen; i++) Determinant(partialASDet[i], partialAS[i], keys->T - 1, keys->m);

    /* End partialASDet* Computation */

    /* Start cofactorAS Computation */

    for (int i = 0; i < SLen; i++) {
        if (i % 2 == 0) mpz_set(cofactorAS[i], partialASDet[i]);
        else mpz_mul_si(cofactorAS[i], partialASDet[i], -1);
    }

    /* End cofactorAS Computation */

    /* Start s* Computation */

    for (int i = 0; i < SLen; i++) {
        mpz_mul(tmp1, cofactorAS[i], keys->y[S[i]]);
        mpz_mul_ui(tmp1, tmp1, 2);
        mpz_powm(s[i], w, tmp1, keys->n);
    }

    /* End s* Computation */

    /* Start r* Computation */

    for (int i = 0; i < SLen; i++) {
        int upperBound = mpz_sizeinbase(keys->n, 2) + SHA256_DIGEST_SIZE * 2;
        mpz_set_ui(tmp1, 2);
        mpz_pow_ui(tmp1, tmp1, upperBound);
        mpz_urandomm(r[i], prng, tmp1);
    }

    /* End r* Computation */

    /* Start st* Computation */

    for (int i = 0; i < SLen; i++) {
        mpz_mul_ui(tmp1, cofactorAS[i], 4);
        mpz_powm(st[i], w, tmp1, keys->n);
    }

    /* End st* Computation */

    /* Start vf* Computation */

    for (int i = 0; i < SLen; i++) mpz_powm(vf[i], keys->g, r[i], keys->n);

    /* End vf* Computation */

    /* Start sf* Computation */

    for (int i = 0; i < SLen; i++) mpz_powm(sf[i], st[i], r[i], keys->n);

    /* End sf* Computation */

    /* Start D* Computation */

    for (int i = 0; i < SLen; i++) {
        mpz_powm_ui(tmp1, s[i], 2, keys->n);
        mpz_set(DArgs[i][0], keys->g);
        mpz_set(DArgs[i][1], st[i]);
        mpz_set(DArgs[i][2], keys->v[S[i]]);
        mpz_set(DArgs[i][3], tmp1);
        mpz_set(DArgs[i][4], vf[i]);
        mpz_set(DArgs[i][5], sf[i]);
        mpz_set(tmp1, DArgs[i][0]);
        for (int j = 1; j < 6; j++) concatenate(tmp1, tmp1, DArgs[i][j]);
        
        uint8_t hashMessage[mpz_sizeinbase(tmp1, 256)];
        mpz_export(hashMessage, NULL, 1, sizeof(uint8_t), 0, 0, tmp1);
        HF(D[i], hashMessage, sizeof(hashMessage) / sizeof(uint8_t));
    }

    /* End D* Computation */

    /* Start Sigma* Computation */

    for (int i = 0; i < SLen; i++) {
        mpz_mul(tmp1, keys->y[S[i]], D[i]);
        mpz_add(sigma[i], tmp1, r[i]);
    }

    /* End Sigma* Computation */

    /* Start Partial Verify */

    for (int i = 0; i < SLen; i++) {
        mpz_set(DVerArgs[i][0], keys->g);
        mpz_set(DVerArgs[i][1], st[i]);
        mpz_set(DVerArgs[i][2], keys->v[S[i]]);

        mpz_powm_ui(tmp1, s[i], 2, keys->n);
        mpz_set(DVerArgs[i][3], tmp1);

        mpz_mul_si(tmp4, D[i], -1);
        mpz_powm(tmp2, keys->g, sigma[i], keys->n);
        mpz_powm(tmp3, keys->v[S[i]], tmp4, keys->n);
        mpz_mul(tmp1, tmp2, tmp3);
        mpz_mod(tmp1, tmp1, keys->n);
        mpz_set(DVerArgs[i][4], tmp1);

        mpz_mul_si(tmp4, D[i], -2);
        mpz_powm(tmp2, st[i], sigma[i], keys->n);
        mpz_powm(tmp3, s[i], tmp4, keys->n);
        mpz_mul(tmp1, tmp2, tmp3);
        mpz_mod(tmp1, tmp1, keys->n);
        mpz_set(DVerArgs[i][5], tmp1);

        mpz_set(tmp1, DVerArgs[i][0]);
        for (int j = 1; j < 6; j++) concatenate(tmp1, tmp1, DVerArgs[i][j]);
        
        uint8_t hashMessage[mpz_sizeinbase(tmp1, 256)];
        mpz_export(hashMessage, NULL, 1, sizeof(uint8_t), 0, 0, tmp1);
        HF(DVer[i], hashMessage, sizeof(hashMessage) / sizeof(uint8_t));
    }

    /* End Partial Verify */

    /* Start Sign Computation */

    mpz_set_ui(sm, 1);
    for (int i = 0; i < SLen; i++) mpz_mul(sm, sm, s[i]);
    mpz_mod(sm, sm, keys->n);

    VDeterminant(tmp1, AS, keys->T, keys->m);
    mpz_mul_ui(Delta, tmp1, 2);
    mpz_mul(tmp1, keys->d, Delta);
    mpz_powm(smv, w, tmp1, keys->n);

    mpz_gcdext(tmp1, a, b, Delta, keys->e);
    mpz_powm(tmp2, sm, a, keys->n);
    mpz_powm(tmp3, w, b, keys->n);
    mpz_mul(signature->s, tmp2, tmp3);
    mpz_mod(signature->s, signature->s, keys->n);

    /* End Sign Computation */

    /* Start Print */

    pmesg(msg_normal, "Message: %s", message->m);
    pmesg_mpz(msg_normal, "Hash(Message)", w);

    for (int i = 0; i < keys->T; i++) {
        for (int j = 0; j < keys->T; j++) {
            char name[64];
            sprintf(name, "AS[%d][%d]", i, j);
            pmesg_mpz(msg_verbose, name, AS[i][j]);
        }
    }

    for (int k = 0; k < SLen; k++) {
        for (int i = 0; i < keys->T - 1; i++) {
            for (int j = 0; j < keys->T - 1; j++) {
                char name[64];
                sprintf(name, "plAS[%d][%d][%d]", k, i, j);
                pmesg_mpz(msg_verbose, name, partialAS[k][i][j]);
            }
        }
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "pASDet[%d]", i);
        pmesg_mpz(msg_verbose, name, partialASDet[i]);
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "cAS[%d]", i);
        pmesg_mpz(msg_verbose, name, cofactorAS[i]);
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "s[%d]", i);
        pmesg_mpz(msg_verbose, name, s[i]);
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "r[%d]", i);
        pmesg_mpz(msg_verbose, name, r[i]);
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "st[%d]", i);
        pmesg_mpz(msg_verbose, name, st[i]);
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "vf[%d]", i);
        pmesg_mpz(msg_verbose, name, vf[i]);
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "sf[%d]", i);
        pmesg_mpz(msg_verbose, name, sf[i]);
    }
    
    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "D[%d]", i);
        pmesg_mpz(msg_verbose, name, D[i]);
    }

    for (int i = 0; i < SLen; i++) {
        for (int j = 0; j < 6; j++) {
            char name[64];
            sprintf(name, "DArgs[%d][%d]", i, j);
            pmesg_mpz(msg_verbose, name, DArgs[i][j]);
        }
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "sigma[%d]", i);
        pmesg_mpz(msg_verbose, name, sigma[i]);
    }

    for (int i = 0; i < SLen; i++) {
        char name[64];
        sprintf(name, "DVer[%d]", i);
        pmesg_mpz(msg_verbose, name, DVer[i]);
    }

    for (int i = 0; i < SLen; i++) {
        for (int j = 0; j < 6; j++) {
            char name[64];
            sprintf(name, "DVerArgs[%d][%d]", i, j);
            pmesg_mpz(msg_verbose, name, DVerArgs[i][j]);
        }
    }

    pmesg_mpz(msg_verbose, "sm", sm);
    pmesg_mpz(msg_verbose, "smv", smv);
    pmesg_mpz(msg_verbose, "Delta", Delta);
    pmesg_mpz(msg_verbose, "a", a);
    pmesg_mpz(msg_verbose, "b", b);
    pmesg_mpz(msg_normal, "Sign", signature->s);

    /* End Print */

    /* Start Clear */

    mpz_clears(w, sm, smv, Delta, a, b, tmp1, tmp2, tmp3, tmp4, NULL);

    for (int i = 0; i < keys->T; i++) {
        for (int j = 0; j < keys->T; j++) {
            mpz_clear(AS[i][j]);
        }
        free(AS[i]);
    }
    free(AS);

    for (int k = 0; k < SLen; k++) {
        for (int i = 0; i < keys->T - 1; i++) {
            for (int j = 0; j < keys->T - 1; j++) {
                mpz_clear(partialAS[k][i][j]);
            }
            free(partialAS[k][i]);
        }
        free(partialAS[k]);
    }
    free(partialAS);

    for (int i = 0; i < SLen; i++) mpz_clear(partialASDet[i]);
    free(partialASDet);

    for (int i = 0; i < SLen; i++) mpz_clear(cofactorAS[i]);
    free(cofactorAS);

    for (int i = 0; i < SLen; i++) mpz_clear(s[i]);
    free(s);

    for (int i = 0; i < SLen; i++) mpz_clear(r[i]);
    free(r);
    
    for (int i = 0; i < SLen; i++) mpz_clear(st[i]);
    free(st);
    
    for (int i = 0; i < SLen; i++) mpz_clear(vf[i]);
    free(vf);
    
    for (int i = 0; i < SLen; i++) mpz_clear(sf[i]);
    free(sf);
    
    for (int i = 0; i < SLen; i++) mpz_clear(D[i]);
    free(D);

    for (int i = 0; i < SLen; i++) {
        for (int j = 0; j < 6; j++) {
            mpz_clear(DArgs[i][j]);
        }
        free(DArgs[i]);
    }
    free(DArgs);

    for (int i = 0; i < SLen; i++) mpz_clear(sigma[i]);
    free(sigma);
    
    for (int i = 0; i < SLen; i++) mpz_clear(DVer[i]);
    free(DVer);

    for (int i = 0; i < SLen; i++) {
        for (int j = 0; j < 6; j++) {
            mpz_clear(DVerArgs[i][j]);
        }
        free(DVerArgs[i]);
    }
    free(DVerArgs);

    /* End Clear */

}

void verify(message_t message, sign_t signature, keys_t keys) {
    mpz_t w;
    mpz_t v;

    pmesg(msg_normal, "\nVerify");

    /* Start Inits */

    mpz_inits(w, v, NULL);

    /* End Inits */

    /* Start Hash */

    H(w, message->m, message->lenght, keys->n);
    
    /* End Hash */

    /* Start v Computation */

    mpz_powm(v, signature->s, keys->e, keys->n);
    
    /* End v Computation */

    /* Start Print */

    pmesg(msg_normal, "Message: %s", message->m);
    pmesg_mpz(msg_normal, "Hash(Message)", w);
    pmesg_mpz(msg_normal, "Sign", signature->s);
    pmesg_mpz(msg_normal, "v", v);

    if (mpz_cmp(v, w) == 0) pmesg(msg_normal, "Verified");
    else pmesg(msg_normal, "Not Verified");

    /* End Print */

    /* Start Clear */

    mpz_clears(w, v, NULL);

    /* End Clear */

}