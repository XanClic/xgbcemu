#include <stddef.h>
#include <time.h>

static struct tm *curtime = NULL;

void unlatch_time(void)
{
    curtime = NULL;
}

void latch_time(void)
{
    if (curtime == NULL)
    {
        time_t now = time(NULL);
        curtime = gmtime(&now);
    }
}

int current_seconds(void)
{
    return curtime->tm_sec;
}

int current_minutes(void)
{
    return curtime->tm_min;
}

int current_hour(void)
{
    return curtime->tm_hour;
}

int current_day_of_year(void)
{
    return curtime->tm_yday;
}

