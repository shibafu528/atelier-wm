#include <time.h>
#include <stdio.h>
#include "time.h"

void PrintTime(time_t *tm, char* output, int length) {
    struct tm *local;
    local = localtime(tm);

    snprintf(output, length, "%4d/%d/%d %d:%02d:%02d",
              local->tm_year + 1900,
              local->tm_mon + 1,
              local->tm_mday,
              local->tm_hour,
              local->tm_min,
              local->tm_sec);
}

void PrintCurrentTime(char* output, int length) {
    time_t tm = time(NULL);
    PrintTime(&tm, output, length);
}
