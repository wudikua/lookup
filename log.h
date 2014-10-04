#ifndef LOG_H
#define LOG_H

#define LOG_DEBUG 0
#define LOG_ROW   1

int logLevel;

void lookup_log(int level, const char *fmt,...);

#endif
