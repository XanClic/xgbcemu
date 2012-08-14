#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "gbc.h"
#include "os-time.h"

#define DURATION_NS 10000ULL

uint32_t determine_tsc_resolution(void)
{
    uint32_t resolutions[10];

    for (int i = 0; i < 10; i++)
    {
        uint64_t rdtsc_start, rdtsc_end, duration;
        struct timeval tv1, tv2;

        __asm__ __volatile__ ("" ::: "memory");

        gettimeofday(&tv1, NULL);
        rdtsc_start = rdtsc();
        nanosleep(&(struct timespec){ .tv_nsec = DURATION_NS * 1000 }, NULL);
        gettimeofday(&tv2, NULL);
        rdtsc_end = rdtsc();

        __asm__ __volatile__ ("" ::: "memory");

        duration = (uint64_t)(tv2.tv_usec - tv1.tv_usec) + (uint64_t)(tv2.tv_sec - tv1.tv_sec) * 1000000ULL;
        resolutions[i] = (rdtsc_end - rdtsc_start) * DURATION_NS / duration;
    }

    printf("Determined TSC resolutions (inc/ms):\n");
    uint64_t sum = 0;
    uint32_t max = 0, min = 0xFFFFFFFF;
    for (int i = 0; i < 10; i++)
    {
        if (resolutions[i] < min)
            min = resolutions[i];
        if (resolutions[i] > max)
            max = resolutions[i];
        sum += (uint64_t)resolutions[i];
        if (i < 9)
            printf("%i ", resolutions[i]);
        else
            printf("%i\n", resolutions[i]);
    }

    sum -= min + max;

    sum /= 8;

    printf("Total resolution: %i; removed %i and %i\n", (int)sum, min, max);

    return sum;
}
