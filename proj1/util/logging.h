#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <time.h>

#define CUR_LOGGING_LEVEL 2

#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4

#define LOG(level, message, ...) \
	if (level >= CUR_LOGGING_LEVEL) { \
		time_t t = time(NULL); \
		struct tm *tm = localtime(&t); \
		char date[20]; \
		strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm); \
		char* level_str = "NULL"; \
		switch (level) { \
			case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break; \
			case LOG_LEVEL_INFO:  level_str = "INFO"; break; \
			case LOG_LEVEL_WARN:  level_str = "WARN"; break; \
            case LOG_LEVEL_ERROR: level_str = "ERROR"; break; \
            default: level_str = "UNKNOWN"; \
        } \
        fprintf(stderr, "[%s] %s: ", date, level_str); \
        fprintf(stderr, message, ##__VA_ARGS__); \
        fprintf (stderr, "\n"); \
	}

#endif // LOGGING_H
