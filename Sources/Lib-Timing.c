#include "Lib-Timing.h"

elapsed_time_t clock_cycles_timing_overhead = 0.0;
elapsed_time_t timestamp_timing_overhead = 0.0;
float stats_kernel_lower_cut = 0.005;
float stats_kernel_upper_cut = 0.05;
double clock_cycles_per_ns = 1.0;
const char *time_unit_str[] = {"ns", "μs", "ms", "s"};
#define calibration_loop 1000000

#if defined(__MACH__)
clockid_t clock_to_use = CLOCK_GETRUSAGE_ID;

#elif defined(__linux__)
#if defined(_POSIX_CPUTIME)
clockid_t clock_to_use = CLOCK_PROCESS_CPUTIME_ID;
#else
clockid_t clock_to_use = CLOCK_MONOTONIC;
#endif

#elif defined(_WIN32)
clockid_t clock_to_use = CLOCK_PROCESS_TIME_WIN_ID;
#else
#warning "No Suitable Clock Timer for this System: Timing Will Not Work!"
clockid_t clock_to_use = CLOCK_NONE;

#endif

#ifdef USE_RDTSCP
clock_cycles_t (*get_clock_cycles_before)() = &cpuid_rdtsc;
clock_cycles_t (*get_clock_cycles_after)() = &rdtscp_cpuid;
#else
clock_cycles_t (*get_clock_cycles_before)() = &cpuid_rdtsc;
clock_cycles_t (*get_clock_cycles_after)() = &cpuid_rdtsc;
#endif

#ifdef __x86_64__
inline clock_cycles_t rdtsc() {
    unsigned int lo, hi;
    asm volatile("rdtsc ;"
                 "mov %%edx, %0 ;"
                 "mov %%eax, %1 ;"
                 : "=r"(hi), "=r"(lo)
                 :
                 : "%rax", "%rdx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

inline clock_cycles_t cpuid_rdtsc() {
    unsigned int lo, hi;
    asm volatile("cpuid ;"
                 "rdtsc ;"
                 "mov %%edx, %0 ;"
                 "mov %%eax, %1 ;"
                 : "=r"(hi), "=r"(lo)
                 :
                 : "%rax", "%rbx", "%rcx", "%rdx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

inline clock_cycles_t rdtscp() {
    unsigned int lo, hi;
    asm volatile("rdtscp ;"
                 "mov %%edx, %0 ;"
                 "mov %%eax, %1 ;"
                 : "=r"(hi), "=r"(lo)
                 :
                 : "%rax", "%rcx", "%rdx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

inline clock_cycles_t rdtscp_cpuid() {
    unsigned int lo, hi;
    asm volatile("rdtscp ;"
                 "mov %%edx, %0 ;"
                 "mov %%eax, %1 ;"
                 "cpuid ;"
                 : "=r"(hi), "=r"(lo)
                 :
                 : "%rax", "%rbx", "%rcx", "%rdx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#else

inline clock_cycles_t rdtsc() {
    unsigned int lo, hi;
    asm volatile("rdtsc ;"
                 "mov %%edx, %0 ;"
                 "mov %%eax, %1 ;"
                 : "=r"(hi), "=r"(lo)
                 :
                 : "%eax", "%edx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

inline clock_cycles_t cpuid_rdtsc() {
    unsigned int lo, hi;
    asm volatile("cpuid ;"
                 "rdtsc ;"
                 "mov %%edx, %0 ;"
                 "mov %%eax, %1 ;"
                 : "=r"(hi), "=r"(lo)
                 :
                 : "%eax", "%ebx", "%ecx", "%edx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

inline clock_cycles_t rdtscp() {
    unsigned int lo, hi;
    asm volatile("rdtscp ;"
                 "mov %%edx, %0 ;"
                 "mov %%eax, %1 ;"
                 : "=r"(hi), "=r"(lo)
                 :
                 : "%eax", "%ecx", "%edx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

inline clock_cycles_t rdtscp_cpuid() {
    unsigned int lo, hi;
    asm volatile("rdtscp ;"
                 "mov %%edx, %0 ;"
                 "mov %%eax, %1 ;"
                 "cpuid ;"
                 : "=r"(hi), "=r"(lo)
                 :
                 : "%eax", "%ebx", "%ecx", "%edx");
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#endif

inline void get_timestamp(timestamp_t ts) {
#if defined(__MACH__)
    
    static bool mac_services_initialized = false;
    static clock_serv_t cclock;
    static mach_timebase_info_data_t timebase_info;
    if (!mac_services_initialized) {
        host_get_clock_service(mach_host_self(), MAC_CLOCK_SERVICE_TO_USE, &cclock);
        mach_timebase_info(&timebase_info);
        mac_services_initialized = true;
    }
    if (clock_to_use == (clockid_t)CLOCK_ABSTIME_MAC_ID) {
        assert(timebase_info.denom);
        double mts = (mach_absolute_time() * timebase_info.numer) / (double) timebase_info.denom;
        ts->tv_sec = mts * 1e-9;
        ts->tv_nsec = mts - (ts->tv_sec * 1e+9);
        return;
    }
    else if (clock_to_use == (clockid_t)CLOCK_SERVICE_MAC_ID) {
        mach_timespec_t mts;
        clock_get_time(cclock, &mts);
        ts->tv_sec = mts.tv_sec;
        ts->tv_nsec = mts.tv_nsec;
        return;
    }

#endif

#if defined(_WIN32)
    
    static bool win_clock_initialized = false;
    static LARGE_INTEGER counter_frequency;
    static HANDLE handle;
    if (!win_clock_initialized) {
        QueryPerformanceFrequency(&counter_frequency);
        handle = GetCurrentProcess();
        win_clock_initialized = true;
    }
    if (clock_to_use == (clockid_t)CLOCK_QPC_WIN_ID) {
        assert(counter_frequency.QuadPart > 0);
        LARGE_INTEGER pc;
        QueryPerformanceCounter(&pc);
        double wts = pc.QuadPart * 1e+9 / (double)counter_frequency.QuadPart;
        ts->tv_sec = wts * 1e-9;
        ts->tv_nsec = wts - (ts->tv_sec * 1e+9);
        return;
    }
    else if (clock_to_use == (clockid_t)CLOCK_PROCESS_TIME_WIN_ID) {
        LARGE_INTEGER dummy_time, user_time;
        GetProcessTimes(handle, (FILETIME *) &dummy_time, (FILETIME *) &dummy_time, (FILETIME *) &dummy_time, (FILETIME *) &user_time);
        ts->tv_sec = user_time.QuadPart * 1e+2 * 1e-9;
        ts->tv_nsec = user_time.QuadPart * 1e+2 - (ts->tv_sec * 1e+9);
        return;
    }

#endif

#if defined(__unix__)

    if (clock_to_use == (clockid_t)CLOCK_GETRUSAGE_ID) {
        struct rusage res;
        getrusage(RUSAGE_SELF, &res);
        ts->tv_sec = res.ru_utime.tv_sec;
        ts->tv_nsec = res.ru_utime.tv_usec * 1e+3;
        return;
    }

#endif

    if (clock_to_use != (clockid_t)CLOCK_NONE) {
        clock_gettime(clock_to_use, ts);
    }
    else {
        ts->tv_sec = 0;
        ts->tv_nsec = 0;
    }
}

elapsed_time_t get_timestamp_resolution() {

#if defined(__MACH__)
    
    if (clock_to_use == (clockid_t)CLOCK_ABSTIME_MAC_ID) {
        return 1.0;
    }
    else if (clock_to_use == (clockid_t)CLOCK_SERVICE_MAC_ID) {
        clock_serv_t cclock;
        natural_t attribute[4];
        mach_msg_type_number_t count = sizeof(attribute) / sizeof(natural_t);
        host_get_clock_service(mach_host_self(), MAC_CLOCK_SERVICE_TO_USE, &cclock);
        clock_get_attributes(cclock, CLOCK_GET_TIME_RES, (clock_attr_t)&attribute, &count);
        mach_port_deallocate(mach_task_self(), cclock);
        return (elapsed_time_t)attribute[0];
    }

#endif

#if defined(_WIN32)

    if (clock_to_use == (clockid_t)CLOCK_QPC_WIN_ID) {
        LARGE_INTEGER counter_frequency;
        QueryPerformanceFrequency(&counter_frequency);
        return (1e+9 / (double)counter_frequency.QuadPart);
    }
    else if (clock_to_use == (clockid_t)CLOCK_PROCESS_TIME_WIN_ID) {
        return 1e+2;
    }

#endif

    if (clock_to_use == (clockid_t)CLOCK_GETRUSAGE_ID) {

#if defined(__MACH__)

        return 1e+3;

#endif

#if defined(__linux__)

        return 1e+6;

#endif
        return 0.0;
    
    }
    
    if (clock_to_use != (clockid_t)CLOCK_NONE) {
        timestamp_t res;
        clock_getres(clock_to_use, res);
        return (res->tv_sec * 1e+9) + (res->tv_nsec);
    }
    else return 0.0;
}

elapsed_time_t get_elapsed_time_from_timestamp(timestamp_t before, timestamp_t after) {
    long long deltat_s = after->tv_sec - before->tv_sec;
    long long deltat_ns = after->tv_nsec - before->tv_nsec;
    return deltat_s * 1e+9 + deltat_ns - timestamp_timing_overhead;
}

void set_stats_kernel_cuts(float lower, float upper) {
    stats_kernel_lower_cut = lower;
    stats_kernel_upper_cut = upper;
}

void set_clock_cycles_per_ns(double ratio) {
    clock_cycles_per_ns = ratio;
}

elapsed_time_t get_clock_cycles_per_ns() {
    return clock_cycles_per_ns;
}

void calibrate_clock_cycles_ratio() {
    timestamp_t before_ts, after_ts;
    clock_cycles_t before = 0, after = 0;
    clockid_t old_clock_to_use = clock_to_use;

#if defined(__linux__)

    clock_to_use = CLOCK_MONOTONIC;

#elif defined(__MACH__)

    clock_to_use = CLOCK_ABSTIME_MAC_ID;

#else

    clock_to_use = CLOCK_QPC_WIN_ID;

#endif

    get_timestamp(before_ts);
    before = get_clock_cycles_before();
    for (volatile unsigned long long i = 0; i < calibration_loop; i++) {}
    after = get_clock_cycles_after();
    get_timestamp(after_ts);
    clock_cycles_per_ns = (double) (after - before) / (double) get_elapsed_time_from_timestamp(before_ts, after_ts);
    clock_to_use = old_clock_to_use;
}

elapsed_time_t get_clock_cycles_overhead() {
    return clock_cycles_timing_overhead;
}

void detect_clock_cycles_overhead() { 
    stats_t stats;
    perform_clock_cycles_sampling(stats, NULL, calibration_loop, tu_nanos, {}, {});
    clock_cycles_timing_overhead = stats->median;
}

elapsed_time_t get_timestamp_overhead() {
    return timestamp_timing_overhead;
}

void detect_timestamp_overhead() {
    stats_t stats;
    perform_timestamp_sampling(stats, NULL, calibration_loop, tu_nanos, {}, {});
    timestamp_timing_overhead = stats->median;
}

inline elapsed_time_t get_elapsed_time_from_cpu_cycles(clock_cycles_t before, clock_cycles_t after) {
    return rintl(((after - before) / clock_cycles_per_ns) - clock_cycles_timing_overhead);
}

inline elapsed_time_t et_to(const elapsed_time_t ns, enum time_unit unit) {
    return ns / pow(10.0, unit * 3);
}

static int __et_compare(const void *n1, const void *n2) {
    if (*(elapsed_time_t *) n1 > *(elapsed_time_t *) n2) return 1;
    else if (*(elapsed_time_t *) n1 < *(elapsed_time_t *) n2) return -1;
    else return 0;
}

void extract_stats(stats_t stats, elapsed_time_t vector[], size_t size, enum time_unit unit) {
    elapsed_time_t sum = 0.0;
    size_t i, first;
    assert(stats);
    assert(size >= 1);
    if (size == 1) {
        stats->unit = unit;
        stats->size = stats->ksize = size;
        stats->min = stats->max = stats->median = stats->mean = vector[0];
        stats->stddev = 0.0;
        return;
    }
    stats->unit = unit;
    stats->size = size;
    stats->ksize = (size_t) ceilf(size * (1.0 - stats_kernel_lower_cut - stats_kernel_upper_cut));
    qsort(vector, size, sizeof(elapsed_time_t), __et_compare);
    first = (size_t) ceilf(size * stats_kernel_lower_cut);
    stats->max = vector[stats->ksize - 1];
    stats->min = vector[first];
    if (stats->ksize % 2) stats->median = vector[first + (stats->ksize / 2)];
    else stats->median = (vector[first + (stats->ksize / 2 - 1)] + vector[first + (stats->ksize / 2)]) / 2.0;
    for (i = first; i < stats->ksize + first; i++) sum += vector[i];
    stats->mean = sum / stats->ksize;
    stats->stddev = 0.0;
    for (i = first; i < stats->ksize + first; i++) stats->stddev += pow(vector[i] - stats->mean, 2);
    stats->stddev = sqrt(stats->stddev / stats->ksize);
}

inline void fprintf_et(FILE *stream, const char *prefix, const elapsed_time_t number, enum time_unit unit, const char *suffix) {
    fprintf(stream, "%s%.*lf %s%s", prefix, unit * 3, number, time_unit_str[unit], suffix);
}

void fprintf_stats(FILE *stream, const char *name, const stats_t stats, const char *suffix) {
    fprintf(stream, "%s:", name);
    fprintf_et(stream, " Media = ", stats->mean, stats->unit, "");
    fprintf_et(stream, ", Mediana = ", stats->median, stats->unit, "");
    fprintf(stream, ", Dev. St. = %.*lf %s", (stats->unit <= tu_micros ? tu_micros : stats->unit) * 3, stats->stddev, time_unit_str[stats->unit]);
    fprintf_et(stream, ", Min = ", stats->min, stats->unit, "");
    fprintf_et(stream, ", Max = ", stats->max, stats->unit, "");
    fprintf(stream, ", Kernel = %zd/%zd%s\n", stats->ksize, stats->size, suffix);
}

void fprintf_short_stats(FILE *stream, const char *name, const stats_t stats, const char *suffix) {
    if (strlen(name) > 0) fprintf(stream, "%s: ", name);
    fprintf_et(stream, "", stats->median, stats->unit, "");
    if (stats->stddev > 0.0) fprintf(stream, " (±%.*lf %s)%s\n", (stats->unit <= tu_micros ? tu_micros : stats->unit) * 3, stats->stddev, time_unit_str[stats->unit], suffix);
    else fprintf(stream, "%s\n", suffix);
}