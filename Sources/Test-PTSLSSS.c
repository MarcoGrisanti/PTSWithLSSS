#include <libgen.h>
#include "Lib-PTSLSSS.h"
#include "Lib-Timing.h"

int main(int argc, char* argv[]) {
    int doBench = 0;
    int modulusBits = 1024;
    int prngSecurityLevel = 128;
    int N = 10;
    int T = 2;
    int isVerified = 0;
    int benchSamplingTime = 5;
    int appliedSamplingTime = 0;
    int maxSamples = benchSamplingTime * 1000;
    char* messageText = "Hello World\0";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "help") == 0) {
            printf("Use: %s [verbose] [bench] [message <Value>] [mod-bits <Value>] [n <Value>] [t <Value>]\n", basename(argv[0]));
            exit(1);
        }
        else if (strcmp(argv[i], "verbose") == 0) {
            set_messaging_level(msg_verbose);
        }
        else if (strcmp(argv[i], "bench") == 0) {
            doBench = 1;
            appliedSamplingTime = benchSamplingTime;
        }
        else if (strcmp(argv[i], "message") == 0) {
            if (i + 1 >= argc) {
                printf("Missing Argument! Type \"help\".\n");
                exit(1);
            }
            messageText = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "mod-bits") == 0) {
            if (i + 1 >= argc) {
                printf("Missing Argument! Type \"help\".\n");
                exit(1);
            }
            modulusBits = atoi(argv[i + 1]);
            if (modulusBits < 128) {
                printf("Invalid \"mod-bits\"! < 128 \n");
                exit(1);
            }
            i++;
        }
        else if (strcmp(argv[i], "n") == 0) {
            if (i + 1 >= argc) {
                printf("Missing Argument! Type \"help\".\n");
                exit(1);
            }
            N = atoi(argv[i + 1]);
            if (N < 2) {
                printf("Invalid \"n\"! < 2\n");
                exit(1);
            }
            i++;
        }
        else if (strcmp(argv[i], "t") == 0) {
            if (i + 1 >= argc) {
                printf("Missing Argument! Type \"help\".\n");
                exit(1);
            }
            T = atoi(argv[i + 1]);
            if (T < 2 || T > N) {
                printf("Invalid \"t\"! < 2 || < N\n");
                exit(1);
            }
            i++;
        }
        else {
            printf("Use: %s [verbose] [bench] [message <Value>] [mod-bits <Value>] [n <Value>] [t <Value>]\n", basename(argv[0]));
            exit(1);
        }
    }

    if (doBench == 1) set_messaging_level(msg_silence);

    int* userID;
    userID = (int*) malloc(sizeof(int) * T);
    for (int i = 0; i < T; i++) userID[i] = i;

    stats_t timing;
    elapsed_time_t time;
    gmp_randstate_t prng;
    keys_t keys;
    sign_t signature;
    message_t message;

    printf("Calibration...\n");
    calibrate_clock_cycles_ratio();
    detect_clock_cycles_overhead();
    detect_timestamp_overhead();

    gmp_randinit_default(prng);
    gmpRandSeed(prng, prngSecurityLevel);

    keysInit(keys, modulusBits, N, T);
    signInit(signature);

    message->m = messageText;
    message->lenght = strlen(messageText);

    perform_oneshot_timestamp_sampling(time, tu_sec,
                                        {
                                            generateKeys(keys, prng);
                                        });
    if (doBench == 1) printf_et("generateKeys Time: ", time, tu_sec, "\n");

    perform_clock_cycles_sampling_period(timing, appliedSamplingTime, maxSamples, tu_millis,
                                        {
                                            sign(signature, message, keys, userID, T, prng);
                                        },
                                        {});
    if (doBench == 1) printf_short_stats("sign Time", timing, "");

    perform_clock_cycles_sampling_period(timing, appliedSamplingTime, maxSamples, tu_millis,
                                        {
                                            verify(message, signature, keys);
                                        },
                                        {});
    if (doBench == 1) printf_short_stats("verify Time", timing, "");

    keysClear(keys);
    signClear(signature);

    gmp_randclear(prng);

    return 0;

}