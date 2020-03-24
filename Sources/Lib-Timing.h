#ifndef LIB_TIMING_H
#define LIB_TIMING_H

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined(__unix)
#include <sys/resource.h>
#endif

#if defined(__MACH__)
#include <CoreServices/CoreServices.h>
#include <mach/clock.h>
#include <mach/clock_types.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#if defined(HIGHRES_CLOCK)
#define MAC_CLOCK_SERVICE_TO_USE HIGHRES_CLOCK

#else
#define MAC_CLOCK_SERVICE_TO_USE SYSTEM_CLOCK

#endif

#define CLOCK_ABSTIME_MAC_ID 98
#define CLOCK_SERVICE_MAC_ID 99

#endif

#if defined(_WIN32)
#include <Processthreadsapi.h>
#include <windows.h>

#define CLOCK_QPC_WIN_ID 90
#define CLOCK_PROCESS_TIME_WIN_ID 91

#endif

#define CLOCK_GETRUSAGE_ID 100
#define CLOCK_NONE 1000

enum time_unit {
    tu_nanos = 0,
    tu_micros,
    tu_millis,
    tu_sec
};

struct stats_struct {
    enum time_unit unit;
    size_t size, ksize;
    double max, min;
    double mean;
    double median;
    double stddev;
};

extern clockid_t clock_to_use;

typedef double elapsed_time_t;
typedef unsigned long long clock_cycles_t;
typedef struct stats_struct *stats_ptr;
typedef struct stats_struct stats_t[1];
typedef struct timespec timestamp_t[1];

#define perform_oneshot_clock_cycles_sampling(ELAPSED_TIME, UNIT, CODE) {                   \
        clock_cycles_t cc_before, cc_after;                                                 \
        cc_before = get_clock_cycles_before();                                              \
        {CODE};                                                                             \
        cc_after = get_clock_cycles_after();                                                \
        ELAPSED_TIME = et_to(get_elapsed_time_from_cpu_cycles(cc_before, cc_after), UNIT);  \
    }

#define perform_oneshot_timestamp_sampling(ELAPSED_TIME, UNIT, CODE) {                      \
        timestamp_t ts_before, ts_after;                                                    \
        get_timestamp(ts_before);                                                           \
        {CODE};                                                                             \
        get_timestamp(ts_after);                                                            \
        ELAPSED_TIME = et_to(get_elapsed_time_from_timestamp(ts_before, ts_after), UNIT);   \
    }

#define perform_timestamp_sampling(STATS, VECTOR, NUM_SAMPLES, UNIT, CODE, CLEAN) {                                                 \
        assert(STATS != NULL);                                                                                                      \
        assert(NUM_SAMPLES > 0);                                                                                                    \
        timestamp_t ts_before, ts_after;                                                                                            \
        elapsed_time_t *vector_samples = (VECTOR != NULL ? VECTOR : (elapsed_time_t *) calloc(NUM_SAMPLES, sizeof(elapsed_time_t)));\
        assert(vector_samples);                                                                                                     \
        for (size_t vector_index = 0; vector_index < NUM_SAMPLES; vector_index++) {                                                 \
            get_timestamp(ts_before);                                                                                               \
            {CODE};                                                                                                                 \
            get_timestamp(ts_after);                                                                                                \
            vector_samples[vector_index] = et_to(get_elapsed_time_from_timestamp(ts_before, ts_after), UNIT);                       \
            if ((vector_index + 1) < NUM_SAMPLES) {                                                                                 \
                CLEAN                                                                                                               \
            }                                                                                                                       \
        }                                                                                                                           \
        extract_stats(STATS, vector_samples, NUM_SAMPLES, UNIT);                                                                    \
        if (VECTOR == NULL) free(vector_samples);                                                                                   \
    }

#define perform_clock_cycles_sampling(STATS, VECTOR, NUM_SAMPLES, UNIT, CODE, CLEAN) {                                              \
        assert(STATS != NULL);                                                                                                      \
        assert(NUM_SAMPLES > 0);                                                                                                    \
        clock_cycles_t cc_before, cc_after;                                                                                         \
        elapsed_time_t *vector_samples = (VECTOR != NULL ? VECTOR : (elapsed_time_t *) calloc(NUM_SAMPLES, sizeof(elapsed_time_t)));\
        assert(vector_samples);                                                                                                     \
        for (size_t vector_index = 0; vector_index < NUM_SAMPLES; vector_index++) {                                                 \
            cc_before = get_clock_cycles_before();                                                                                  \
            {CODE};                                                                                                                 \
            cc_after = get_clock_cycles_after();                                                                                    \
            vector_samples[vector_index] = et_to(get_elapsed_time_from_cpu_cycles(cc_before, cc_after), UNIT);                      \
            if ((vector_index + 1) < NUM_SAMPLES) {                                                                                 \
                CLEAN                                                                                                               \
            }                                                                                                                       \
        }                                                                                                                           \
        extract_stats(STATS, vector_samples, NUM_SAMPLES, UNIT);                                                                    \
        if (VECTOR == NULL) free(vector_samples);                                                                                   \
    }

#define perform_timestamp_sampling_period(STATS, PERIOD, MAX_SAMPLES, UNIT, CODE, CLEAN) {                      \
        assert(STATS != NULL);                                                                                  \
        assert(PERIOD >= 0);                                                                                    \
        assert(MAX_SAMPLES > 0);                                                                                \
        timestamp_t ts_before, ts_after;                                                                        \
        timestamp_t ts_begin;                                                                                   \
        elapsed_time_t *vector_samples = (elapsed_time_t *) calloc(MAX_SAMPLES, sizeof(elapsed_time_t));        \
        assert(vector_samples);                                                                                 \
        size_t vector_index;                                                                                    \
        get_timestamp(ts_begin);                                                                                \
        for (vector_index = 0; vector_index < MAX_SAMPLES; vector_index++) {                                    \
            get_timestamp(ts_before);                                                                           \
            {CODE};                                                                                             \
            get_timestamp(ts_after);                                                                            \
            vector_samples[vector_index] = et_to(get_elapsed_time_from_timestamp(ts_before, ts_after), UNIT);   \
            if (et_to(get_elapsed_time_from_timestamp(ts_begin, ts_after), tu_sec) > PERIOD) {                  \
                vector_index++;                                                                                 \
                break;                                                                                          \
            }                                                                                                   \
            if ((vector_index + 1) < MAX_SAMPLES) {                                                             \
                CLEAN                                                                                           \
            }                                                                                                   \
        }                                                                                                       \
        extract_stats(STATS, vector_samples, vector_index, UNIT);                                               \
        free(vector_samples);                                                                                   \
    }

#define perform_clock_cycles_sampling_period(STATS, PERIOD, MAX_SAMPLES, UNIT, CODE, CLEAN) {                   \
        assert(STATS != NULL);                                                                                  \
        assert(PERIOD >= 0);                                                                                    \
        assert(MAX_SAMPLES > 0);                                                                                \
        clock_cycles_t cc_before, cc_after;                                                                     \
        clock_cycles_t cc_begin;                                                                                \
        elapsed_time_t *vector_samples = (elapsed_time_t *) calloc(MAX_SAMPLES, sizeof(elapsed_time_t));        \
        assert(vector_samples);                                                                                 \
        size_t vector_index;                                                                                    \
        cc_begin = get_clock_cycles_before();                                                                   \
        for (vector_index = 0; vector_index < MAX_SAMPLES; vector_index++) {                                    \
            cc_before = get_clock_cycles_before();                                                              \
            {CODE} cc_after = get_clock_cycles_after();                                                         \
            vector_samples[vector_index] = et_to(get_elapsed_time_from_cpu_cycles(cc_before, cc_after), UNIT);  \
            if (et_to(get_elapsed_time_from_cpu_cycles(cc_begin, cc_after), tu_sec) > PERIOD) {                 \
                vector_index++;                                                                                 \
                break;                                                                                          \
            }                                                                                                   \
            if ((vector_index + 1) < MAX_SAMPLES) {                                                             \
                CLEAN                                                                                           \
            }                                                                                                   \
        }                                                                                                       \
        extract_stats(STATS, vector_samples, vector_index, UNIT);                                               \
        free(vector_samples);                                                                                   \
    }

#define printf_et(PREFIX, NUMBER, UNIT, SUFFIX) \
    fprintf_et(stdout, PREFIX, NUMBER, UNIT, SUFFIX)
#define printf_stats(NAME, STATS, SUFFIX) \
    fprintf_stats(stdout, NAME, STATS, SUFFIX)
#define printf_short_stats(NAME, STATS, SUFFIX) \
    fprintf_short_stats(stdout, NAME, STATS, SUFFIX)

extern clock_cycles_t (*get_clock_cycles_before)();
extern clock_cycles_t (*get_clock_cycles_after)();
void get_timestamp(timestamp_t ts);
void set_stats_kernel_cuts(float lower, float upper);
void set_clock_cycles_per_ns(double ratio);
void calibrate_clock_cycles_ratio();
void detect_clock_cycles_overhead();
void detect_timestamp_overhead();
void extract_stats(stats_t stats, elapsed_time_t vector[], size_t size, enum time_unit unit);
void fprintf_et(FILE *stream, const char *prefix, const elapsed_time_t number, enum time_unit unit, const char *suffix);
void fprintf_stats(FILE *stream, const char *name, const stats_t stats, const char *suffix);
void fprintf_short_stats(FILE *stream, const char *name, const stats_t stats, const char *suffix);
elapsed_time_t get_clock_cycles_per_ns();
elapsed_time_t get_clock_cycles_overhead();
elapsed_time_t get_timestamp_overhead();
elapsed_time_t get_elapsed_time_from_cpu_cycles(clock_cycles_t before, clock_cycles_t after);
elapsed_time_t et_to(const elapsed_time_t ns, enum time_unit unit);
elapsed_time_t get_timestamp_resolution();
elapsed_time_t get_elapsed_time_from_timestamp(timestamp_t before,timestamp_t after);
clock_cycles_t rdtsc();
clock_cycles_t cpuid_rdtsc();
clock_cycles_t rdtscp();
clock_cycles_t rdtscp_cpuid();

#endif