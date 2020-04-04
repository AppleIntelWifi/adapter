//
//  IWLDebug.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#ifndef IWLDebug_h
#define IWLDebug_h

#include <IOKit/IOLib.h>

#ifdef DEBUG
#define DebugLog(args...) TraceLog(args)
#else
#define DebugLog(args...)
#endif

#ifdef KERNEL_LOG
#define TraceLog(args...) \
do { kprintf(args); } while (0)
#else
#define TraceLog(args...) IOLog(args)
#endif

#define __iwl_warn(args...) \
do { TraceLog("AppleIntelWifiAdapter WARN: " args);} while (0)

#define __iwl_info(args...) \
do { TraceLog("AppleIntelWifiAdapter INFO: " args);} while (0)

#define __iwl_crit(args...) \
do { TraceLog("AppleIntelWifiAdapter CRIT: " args); } while (0)

#define __iwl_err(rfkill_prefix, trace_only, args...) \
do { if (!trace_only) TraceLog("AppleIntelWifiAdapter ERR: " args); } while (0)

#define __iwl_dbg(args...) \
do { DebugLog("AppleIntelWifiAdapter DEBUG: " args); } while (0)

#define __iwl_dbg_buf(in_buf, size) \
do { size_t out_len = 0; unsigned char* buf = base64_encode((const unsigned char*)in_buf, size, &out_len); \
    if (buf != NULL) { \
        DebugLog("AppleIntelWifiAdapter DEBUG: buffer is %s\n", buf); \
        _FREE(buf, out_len + 1); \
    } \
} while(0)

/* No matter what is m (priv, bus, trans), this will work */
#define IWL_ERR_DEV(m, f, a...)                        \
        do {                                        \
                __iwl_err(false, false, f, ## a);   \
        } while (0)
#define IWL_ERR(m, f, a...)                         \
        IWL_ERR_DEV(m, f, ## a)
#define IWL_WARN(m, f, a...)                        \
        do {                                        \
                __iwl_warn(f, ## a);                \
        } while (0)
#define IWL_INFO(m, f, a...)                        \
        do {                                        \
                __iwl_info(f, ## a);                \
        } while (0)
#define IWL_CRIT(m, f, a...)                        \
        do {                                        \
                __iwl_crit(f, ## a);                \
        } while (0)

#define __IWL_DEBUG_DEV(m, f, args...)     \
        do {                                            \
                __iwl_dbg(f, ##args);   \
        } while (0)
#define IWL_DEBUG(m, f, args...)               \
        __IWL_DEBUG_DEV(level, f, ##args)

#define IWL_DEBUG_BUF(b, s) \
        do { \
            __iwl_dbg_buf(b, s); \
        } while(0)

unsigned char * base64_encode(unsigned char *src, size_t len,
                              size_t *out_len);

unsigned char * base64_decode(const unsigned char *src, size_t len,
                              size_t *out_len);

#endif /* IWLDebug_h */
