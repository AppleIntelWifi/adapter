//
//  IWLDebug.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLDebug_h
#define IWLDebug_h

#include <IOKit/IOLib.h>

#ifdef DEBUG
#define DebugLog(args...) IOLog(args)
#else
#define DebugLog(args...)
#endif

#define TraceLog(args...) IOLog(args)

#define __iwl_warn(args...) \
do { TraceLog("AppleIntelWifiAdapter WARN: " args); } while (0)

#define __iwl_info(args...) \
do { TraceLog("AppleIntelWifiAdapter INFO: " args); } while (0)

#define __iwl_crit(args...) \
do { TraceLog("AppleIntelWifiAdapter CRIT: " args); } while (0)

#define __iwl_err(rfkill_prefix, trace_only, args...) \
do { if (!trace_only) TraceLog("AppleIntelWifiAdapter ERR: " args); } while (0)

#define __iwl_dbg(level, limit, args...) \
do { ((!limit)) DebugLog("AppleIntelWifiAdapter DEBUG: " args); } while (0);

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

#define __IWL_DEBUG_DEV(level, limit, fmt, args...)     \
        do {                                            \
                __iwl_dbg(level, limit, fmt, ##args);   \
        } while (0)
#define IWL_DEBUG(m, level, fmt, args...)               \
        __IWL_DEBUG_DEV(level, false, fmt, ##args)
#define IWL_DEBUG_DEV(m, level, fmt, args...)           \
        __IWL_DEBUG_DEV(level, false, fmt, ##args)
#define IWL_DEBUG_LIMIT(m, level, fmt, args...)         \
        __IWL_DEBUG_DEV(level, true, fmt, ##args)

#endif /* IWLDebug_h */
