#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>
#include <stdint.h>

#define DEBUG 0

#define DEBUG_TAG "[DEBUG] "
#define INFO_TAG "[INFO] "
#define WARN_TAG "[WARN] "

#if DEBUG
#define pr_debug(fmt, ...) printf(DEBUG_TAG fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif

#define pr_info(fmt, ...) printf(INFO_TAG fmt, ##__VA_ARGS__)

#endif