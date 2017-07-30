#ifndef OS_TIME_H
#define OS_TIME_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return (uint64_t)lo | ((uint64_t)hi << 32);
}

uint32_t determine_tsc_resolution(void);
void latch_time(void);
void unlatch_time(void);
int current_seconds(void);
int current_minutes(void);
int current_hour(void);
int current_day_of_year(void);

#define sleep_us(microseconds) Sleep(microseconds / 1000)

#endif
