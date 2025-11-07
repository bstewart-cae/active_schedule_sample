#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <stdio.h>

// -----------------------------------------------------------------
// Define your logging categories as unique bit values.
// -----------------------------------------------------------------
#define DBG_LOG_CAT_NONE        (0)
#define DBG_LOG_CAT_SYSTEM      (1 << 0)
#define DBG_LOG_CAT_APP         (1 << 1)
#define DBG_LOG_CAT_PERIPHERAL  (1 << 2)
#define DBG_LOG_CAT_RF          (1 << 3)
#define DBG_LOG_CAT_ALL         (0xFFFFFFFF)

// -----------------------------------------------------------------
// The master logging switch. Defined in your build system.
// For example, #define LOG_MASK_CONFIG (LOG_CAT_RF | LOG_DEBUG_APP)
// -----------------------------------------------------------------
#ifndef DBG_LOG_MASK_CONFIG
#define DBG_LOG_MASK_CONFIG  (DBG_LOG_CAT_APP)  // Default only to enable application logs
#endif


// -----------------------------------------------------------------
// Define a specific macro for each log category.
// The #if check is done by the preprocessor at compile-time.
// -----------------------------------------------------------------

#if (DBG_LOG_MASK_CONFIG & DBG_LOG_CAT_SYSTEM)
#define LOG_DEBUG_SYSTEM(fmt, ...) printf("[SYS] " fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG_SYSTEM(fmt, ...) ((void)0)
#endif

#if (DBG_LOG_MASK_CONFIG & DBG_LOG_CAT_APP)
#define LOG_DEBUG_APP(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG_APP(fmt, ...) ((void)0)
#endif

#if (DBG_LOG_MASK_CONFIG & DBG_LOG_CAT_PERIPHERAL)
#define LOG_DEBUG_PERI(fmt, ...) printf("[PERI] " fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG_PERI(fmt, ...) ((void)0)
#endif

#if (DBG_LOG_MASK_CONFIG & DBG_LOG_CAT_RF)
#define LOG_DEBUG_RF(fmt, ...) printf("[RF] " fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG_RF(fmt, ...) ((void)0)
#endif

#endif // DEBUG_LOG_H
