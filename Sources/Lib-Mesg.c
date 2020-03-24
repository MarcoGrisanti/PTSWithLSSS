#include "Lib-Mesg.h"

msg_level_t msg_level_threshold = msg_normal;

void set_messaging_level(msg_level_t l) { msg_level_threshold = l; }
msg_level_t get_messaging_level() { return msg_level_threshold; }

#if defined(NDEBUG) && defined(__GNUC__)
#else
void plain_pmesg(msg_level_t level, const char *format, ...) {
#if defined(NDEBUG)
#else
    va_list args;
    if (level > msg_level_threshold) return;
    for (char i = msg_silence; i < level; i++) fputc(' ', pmesg_io_channel);
    va_start(args, format);
    vfprintf(pmesg_io_channel, format, args);
    va_end(args);
    fputc('\n', pmesg_io_channel);
#endif
}

void gmp_pmesg(msg_level_t level, const char *format, ...) {
#if defined(NDEBUG)
#else
    va_list args;
    if (level > msg_level_threshold) return;
    for (char i = msg_silence; i < level; i++)fputc(' ', pmesg_io_channel);
    va_start(args, format);
    gmp_vfprintf(pmesg_io_channel, format, args);
    va_end(args);
    fputc('\n', pmesg_io_channel);
#endif
}

#endif

#if !defined(NDEBUG)
void __pmesg_mpz(msg_level_t level, const char *name, const char *var_name, const mpz_t number, int base) {
    if (level > msg_level_threshold) return;
    for (char i = msg_silence; i < level; i++) fputc(' ', pmesg_io_channel);
    int number_base_len = mpz_sizeinbase(number, base);
    if (strlen(name) > 0) fprintf(pmesg_io_channel, "%s '%s' (%zu bit): ", name, var_name, mpz_sizeinbase(number, 2));
    else fprintf(pmesg_io_channel, "'%s' (%zu bit): ", var_name, mpz_sizeinbase(number, 2));
    if (number_base_len <= pmesg_mpz_shortening_threshold) {
        mpz_out_str(pmesg_io_channel, base, number);
    }
    else {
        char buffer[number_base_len + 2];
        mpz_get_str(buffer, base, number);
        assert((pmesg_mpz_shortening_prefix_suffix_length * 2) < pmesg_mpz_shortening_threshold);
        buffer[pmesg_mpz_shortening_prefix_suffix_length] = '\0';
        fprintf(pmesg_io_channel, "%s.....%s", buffer, buffer + number_base_len - pmesg_mpz_shortening_prefix_suffix_length);
    }
    fputc('\n', pmesg_io_channel);
}

void __pmesg_mpf(msg_level_t level, const char *name, const char *var_name, const mpf_t number) {
    if (level > msg_level_threshold) return;
    for (char i = msg_silence; i < level; i++) fputc(' ', pmesg_io_channel);
    if (strlen(name) > 0) gmp_fprintf(pmesg_io_channel, "%s '%s': %Ff\n", name, var_name, number);
    else gmp_fprintf(pmesg_io_channel, "'%s': %Ff\n", var_name, number);
}

void __pmesg_hex(msg_level_t level, const char *name, const char *var_name, size_t data_size, const void *data) {
    if (level > msg_level_threshold) return;
    for (char i = msg_silence; i < level; i++) fputc(' ', pmesg_io_channel);
    if (strlen(name) > 0) fprintf(pmesg_io_channel, "%s '%s': ", name, var_name);
    else fprintf(pmesg_io_channel, "'%s': ", var_name);
    for (size_t i = 0; i < data_size; i++) fprintf(pmesg_io_channel, "%02X ", ((uint8_t *)data)[i]);
    fprintf(pmesg_io_channel, "\n");
}

void __pmesg_stats(msg_level_t level, const char *name, const char *var_name, const stats_t stats) {
    if (level > msg_level_threshold) return;
    for (char i = msg_silence; i < level; i++) fputc(' ', pmesg_io_channel);
    if (strlen(name) > 0) fprintf(pmesg_io_channel, "%s '%s': ", name, var_name);
    else fprintf(pmesg_io_channel, "'%s': ", var_name);
    fprintf_short_stats(pmesg_io_channel, "", stats, "");
}

//#if defined(PBC_SUPPORT)
void __pmesg_element(msg_level_t level, const char *name, const char *var_name, const element_t element) {
    if (level > msg_level_threshold) return;
    for (char i = msg_silence; i < level; i++) fputc(' ', pmesg_io_channel);
    if (strlen(name) > 0) element_fprintf(pmesg_io_channel, "%s '%s': %B\n", name, var_name, element);
    else element_fprintf(pmesg_io_channel, "'%s': %B\n", var_name, element);
}

//#endif

#endif