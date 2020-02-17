//
//  NotificationWait.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/8.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef NotificationWait_hpp
#define NotificationWait_hpp

#include "../trans/TransHdr.h"

#include <sys/queue.h>
#include <linux/types.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IOLib.h>

#define MAX_NOTIF_CMDS    5

/**
 * struct iwl_notification_wait - notification wait entry
 * @list: list head for global list
 * @fn: Function called with the notification. If the function
 *    returns true, the wait is over, if it returns false then
 *    the waiter stays blocked. If no function is given, any
 *    of the listed commands will unblock the waiter.
 * @cmds: command IDs
 * @n_cmds: number of command IDs
 * @triggered: waiter should be woken up
 * @aborted: wait was aborted
 *
 * This structure is not used directly, to wait for a
 * notification declare it on the stack, and call
 * iwl_init_notification_wait() with appropriate
 * parameters. Then do whatever will cause the ucode
 * to notify the driver, and to wait for that then
 * call iwl_wait_notification().
 *
 * Each notification is one-shot. If at some point we
 * need to support multi-shot notifications (which
 * can't be allocated on the stack) we need to modify
 * the code for them.
 */
struct iwl_notification_wait {
    STAILQ_ENTRY(iwl_notification_wait) list;

    bool (*fn)(struct iwl_notif_wait_data *notif_data,
           struct iwl_rx_packet *pkt, void *data);
    void *fn_data;

    u16 cmds[MAX_NOTIF_CMDS];
    u8 n_cmds;
    bool triggered, aborted;
};

struct iwl_notif_wait_data {
    STAILQ_HEAD(, iwl_notification_wait) notif_waits;
    IOSimpleLock *notif_wait_lock;
    IOLock *notif_waitq;
};

/* caller functions */
void iwl_notification_wait_init(struct iwl_notif_wait_data *notif_data);
bool iwl_notification_wait(struct iwl_notif_wait_data *notif_data, struct iwl_rx_packet *pkt);
void iwl_abort_notification_waits(struct iwl_notif_wait_data *notif_data);

static inline void
iwl_notification_notify(struct iwl_notif_wait_data *notif_data)
{
    struct iwl_notification_wait *wait_entry;
    IOLockLock(notif_data->notif_waitq);
    STAILQ_FOREACH(wait_entry, &notif_data->notif_waits, list)
        IOLockWakeup(notif_data->notif_waitq, wait_entry, true);
    IOLockUnlock(notif_data->notif_waitq);
}

static inline void
iwl_notification_wait_notify(struct iwl_notif_wait_data *notif_data,
                 struct iwl_rx_packet *pkt)
{
    if (iwl_notification_wait(notif_data, pkt)) {
        iwl_notification_notify(notif_data);
    }
        
}

/* user functions */
void __acquires(wait_entry)
iwl_init_notification_wait(struct iwl_notif_wait_data *notif_data, struct iwl_notification_wait *wait_entry,
                           const u16 *cmds, int n_cmds,
                           bool (*fn)(struct iwl_notif_wait_data *notif_data, struct iwl_rx_packet *pkt, void *data),
                           void *fn_data);

int __must_check __releases(wait_entry)
iwl_wait_notification(struct iwl_notif_wait_data *notif_data, struct iwl_notification_wait *wait_entry,
                      unsigned long timeout);

void __releases(wait_entry)
iwl_remove_notification(struct iwl_notif_wait_data *notif_data, struct iwl_notification_wait *wait_entry);

#endif /* NotificationWait_hpp */
