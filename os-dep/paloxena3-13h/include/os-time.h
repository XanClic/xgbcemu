#ifndef OS_TIME_H
#define OS_TIME_H

void latch_time(void);
void unlatch_time(void);
int current_seconds(void);
int current_minutes(void);
int current_hour(void);
int current_day_of_year(void);

#endif
