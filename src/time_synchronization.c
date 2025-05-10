#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void  print_date() {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", local);
    printf(buf);
}

void sleep_until_next_min_interval(int delay_in_minutes) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int seconds_past_hour = t->tm_min * 60 + t->tm_sec;
    int seconds_to_wait = (delay_in_minutes * 60) - (seconds_past_hour % (delay_in_minutes * 60));
    sleep(seconds_to_wait);
}