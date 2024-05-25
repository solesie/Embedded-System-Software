#ifndef LOGGING_H
#define LOGGING_H

#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/rtc.h>

#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_ERROR 4

#define LOG(level, fmt, ...) \
	do { \
		struct timespec ts; \
		struct tm tm; \
		ktime_get_real_ts(&ts); \
		time_to_tm(ts.tv_sec, 0, &tm); \
		if (level == LOG_LEVEL_DEBUG) printk(KERN_DEBUG "[%04ld-%02d-%02d %02d:%02d:%02d] DEBUG: " fmt "\n", \
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ##__VA_ARGS__); \
		else if (level == LOG_LEVEL_WARN) printk(KERN_WARNING "[%04ld-%02d-%02d %02d:%02d:%02d] WARN: " fmt "\n", \
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ##__VA_ARGS__); \
		else if (level == LOG_LEVEL_INFO) printk(KERN_INFO "[%04ld-%02d-%02d %02d:%02d:%02d] INFO: " fmt "\n", \
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ##__VA_ARGS__); \
		else if (level == LOG_LEVEL_ERROR) printk(KERN_ERR "[%04ld-%02d-%02d %02d:%02d:%02d] ERROR: " fmt "\n", \
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ##__VA_ARGS__); \
		else printk(KERN_INFO "[%04ld-%02d-%02d %02d:%02d:%02d] UNKNOWN: " fmt "\n", \
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ##__VA_ARGS__); \
	} while (0)

#endif // LOGGING_H
