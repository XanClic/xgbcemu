#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gbc.h"
#include "os-time.h"

uint32_t determine_tsc_resolution(void)
{
    uint32_t resolutions[10];

    for (int i = 0; i < 10; i++)
    {
        uint64_t rdtsc_start, rdtsc_end;

        __asm__ __volatile__ ("" ::: "memory");

        rdtsc_start = rdtsc();
        nanosleep(&(struct timespec){ .tv_nsec = 1000000 }, NULL);
        rdtsc_end = rdtsc();

        __asm__ __volatile__ ("" ::: "memory");

        resolutions[i] = (rdtsc_end - rdtsc_start);
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
