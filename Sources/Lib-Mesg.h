#ifndef LIB_MESG_H
#define LIB_MESG_H

#include "Lib-Timing.h"
#include <assert.h>
#include <gmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#if defined(PBC_SUPPORT)
#include <pbc/pbc.h>
//#endif

typedef enum {
    msg_silence,
    msg_normal,
    msg_verbose,
    msg_very_verbose
} msg_level_t;

#define pmesg_mpz_shortening_threshold 22
#define pmesg_mpz_shortening_prefix_suffix_length 10
#define pmesg_mpz_default_base 10
#define pmesg_io_channel stdout

int gmp_vfprintf(FILE *, const char *, va_list);
int gmp_fprintf(FILE *, const char *, ...);
size_t mpz_out_str(FILE *, int, mpz_srcptr);

#if defined(NDEBUG) && defined(__GNUC__)
#define plain_pmesg(level, format, args...) ((void)0)
#define gmp_pmesg(level, format, args...) ((void)0)
#else
void plain_pmesg(msg_level_t level, const char *format, ...);
void gmp_pmesg(msg_level_t level, const char *format, ...);
#endif

#if defined(NDEBUG)
#define pmesg_mpz(level, name, number) ((void)0)
#define pmesg_mpz_in_base(level, name, number, base) ((void)0)
#define pmesg_mpf(level, name, number) ((void)0)
#define pmesg_hex(level, name, data_size, data) ((void)0)
#define pmesg_stats(level, name, stats) ((void)0)
#define pmesg_element(level, name, element) ((void)0)
#else

void __pmesg_mpz(msg_level_t level, const char *name, const char *var_name, const mpz_t number, int base);
void __pmesg_mpf(msg_level_t level, const char *name, const char *var_name, const mpf_t number);
void __pmesg_hex(msg_level_t level, const char *name, const char *var_name, size_t data_size, const void *data);
void __pmesg_stats(msg_level_t level, const char *name, const char *var_name, const stats_t stats);

#define pmesg_mpz(level, name, number)                                         \
    (__pmesg_mpz(level, name, #number, number, pmesg_mpz_default_base))

#define pmesg_mpz_in_base(level, name, number, base)                           \
    (__pmesg_mpz(level, name, #number, number, base))

#define pmesg_mpf(level, name, number)                                         \
    (__pmesg_mpf(level, name, #number, number))

#define pmesg_hex(level, name, data_size, data)                                \
    (__pmesg_hex(level, name, #data, data_size, data))

#define pmesg_stats(level, name, stats)                                        \
    (__pmesg_stats(level, name, #stats, stats))

#define pmesg_element(level, name, element)                                    \
    (__pmesg_element(level, name, #element, element))

//#if defined(PBC_SUPPORT)
void __pmesg_element(msg_level_t level, const char *name, const char *var_name, const element_t element);
//#endif

#endif

void set_messaging_level(msg_level_t l);
msg_level_t get_messaging_level();

#if defined(PMESG_DEFAULT_PLAIN_SUPPORT)
#define pmesg plain_pmesg
#else
#define pmesg gmp_pmesg
#endif

#endif