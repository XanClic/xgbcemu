#include <string.h>

#include "gbc.h"

void enter_shell(void)
{
    char input[200];

    os_print("\n");

    for (;;)
    {
        os_print_flush("xgbcemu$ ");
        os_get_line(input, 200);

        if (!strcmp(input, "quit") || !strcmp(input, "exit"))
            break;
        else if (!strcmp(input, "change_time"))
        {
            latch_time();
            int doy = current_day_of_year(), hour = current_hour(), min = current_minutes(), sec = current_seconds();
            os_print("Now:     Day: %i; Time: %02i:%02i:%02i\n", doy, hour, min, sec);
            os_print("Setting: Day: %i; Time: %02i:%02i:%02i\n", doy + doy_diff, hour + hour_diff, min + min_diff, sec + sec_diff);
            os_print("Enter new values, -1 for no change. \n");
            os_print("(Differences will be calculated relative to the time given above)\n");
            os_print_flush("New day of year? ");
            int new_doy = os_get_integer();
            if (new_doy >= 0)
                doy_diff = new_doy - doy;
            os_print_flush("New hour? ");
            int new_hour = os_get_integer();
            if (new_hour >= 0)
                hour_diff = new_hour - hour;
            os_print_flush("New minute? ");
            int new_min = os_get_integer();
            if (new_min >= 0)
                min_diff = new_min - min;
            os_print_flush("New second? ");
            int new_sec = os_get_integer();
            if (new_sec >= 0)
                sec_diff = new_sec - sec;
            unlatch_time();
        }
    }
}