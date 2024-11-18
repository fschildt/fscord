#ifndef BASIC_TIME_H
#define BASIC_TIME_H

#include <os/os.h>

typedef OSTime Time;


typedef struct {
    Time t0;
    Time t1;
} Timer;

#define TIMER_START(name) \
    static Timer timer_##name; \
    timer_##name.t0 = os_time_get_now();

// Todo: On windows this %ld should be %lld
#define TIMER_END(name) \
    timer_##name.t1 = os_time_get_now(); \
    i64 sec_diff = timer_##name.t1.seconds - timer_##name.t0.seconds; \
    i64 nsec0 = timer_##name.t0.nanoseconds; \
    i64 nsec1 = timer_##name.t1.nanoseconds; \
    i64 nsec_total = sec_diff*1000*1000*1000; \
    if (nsec1 >= nsec0) nsec_total += nsec1 - nsec0; \
    else                nsec_total -= nsec1 + nsec0; \
    i64 ms_part = nsec_total / (1000*1000); \
    i64 ns_part = nsec_total % (1000*1000); \
    printf("%-24s: %3ld,%06ld ms\n", #name, ms_part, ns_part);


#define CYCLER_START(name) \
    u64 cycler_##name_t0 = __rdtsc();

// Todo: On windows this %lu should be %llu
#define CYCLER_END(name) \
    u64 cycler_##name_t1 = __rdtsc(); \
    u64 cycles_counted_##name = cycler_##name_t1 - cycler_##name_t0; \
    printf("cycles elapsed (%s): %lu\n", #name, cycles_counted_##name);


#endif // BASIC_TIME_H
