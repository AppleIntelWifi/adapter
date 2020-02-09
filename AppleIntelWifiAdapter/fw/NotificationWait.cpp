//
//  NotificationWait.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/8.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "NotificationWait.hpp"
#include <sys/errno.h>

void iwl_notification_wait_init(struct iwl_notif_wait_data *notif_wait)
{
    notif_wait->notif_wait_lock = IOSimpleLockAlloc();
    STAILQ_INIT(&notif_wait->notif_waits);
    notif_wait->notif_waitq = IOLockAlloc();
}

bool iwl_notification_wait(struct iwl_notif_wait_data *notif_wait, struct iwl_rx_packet *pkt)
{
    bool triggered = false;

    if (!STAILQ_EMPTY(&notif_wait->notif_waits)) {
        struct iwl_notification_wait *w;

        //IOSimpleLockLock(notif_wait->notif_wait_lock);
        STAILQ_FOREACH(w, &notif_wait->notif_waits, list) {
            int i;
            bool found = false;

            /*
             * If it already finished (triggered) or has been
             * aborted then don't evaluate it again to avoid races,
             * Otherwise the function could be called again even
             * though it returned true before
             */
            if (w->triggered || w->aborted)
                continue;

            for (i = 0; i < w->n_cmds; i++) {
                u16 rec_id = WIDE_ID(pkt->hdr.group_id, pkt->hdr.cmd);

                if (w->cmds[i] == rec_id || (!iwl_cmd_groupid(w->cmds[i]) && DEF_ID(w->cmds[i]) == rec_id)) {
                    found = true;
                    break;
                }
            }
            if (!found)
                continue;

            if (!w->fn || w->fn(notif_wait, pkt, w->fn_data)) {
                w->triggered = true;
                triggered = true;
            }
        }
        //IOSimpleLockUnlock(notif_wait->notif_wait_lock);
    }

    return triggered;
}

void iwl_abort_notification_waits(struct iwl_notif_wait_data *notif_wait)
{
    struct iwl_notification_wait *wait_entry;

    //IOSimpleLockLock(notif_wait->notif_wait_lock);
    STAILQ_FOREACH(wait_entry, &notif_wait->notif_waits, list)
        wait_entry->aborted = true;
    //IOSimpleLockUnlock(notif_wait->notif_wait_lock);
    
    // TODO: Implement
    //wake_up_all(&notif_wait->notif_waitq);
    IOLockLock(notif_wait->notif_waitq);
    STAILQ_FOREACH(wait_entry, &notif_wait->notif_waits, list)
        IOLockWakeup(notif_wait->notif_waitq, wait_entry, true);
    IOLockUnlock(notif_wait->notif_waitq);
}

void
iwl_init_notification_wait(struct iwl_notif_wait_data *notif_wait, struct iwl_notification_wait *wait_entry,
                           const u16 *cmds, int n_cmds,
                           bool (*fn)(struct iwl_notif_wait_data *notif_wait, struct iwl_rx_packet *pkt, void *data),
                           void *fn_data)
{
    if (WARN_ON(n_cmds > MAX_NOTIF_CMDS))
        n_cmds = MAX_NOTIF_CMDS;

    wait_entry->fn = fn;
    wait_entry->fn_data = fn_data;
    wait_entry->n_cmds = n_cmds;
    memcpy(wait_entry->cmds, cmds, n_cmds * sizeof(u16));
    wait_entry->triggered = false;
    wait_entry->aborted = false;

    //IOSimpleLockLock(notif_wait->notif_wait_lock);
    STAILQ_INSERT_HEAD(&notif_wait->notif_waits, wait_entry, list);
    //IOSimpleLockUnlock(notif_wait->notif_wait_lock);
}

void iwl_remove_notification(struct iwl_notif_wait_data *notif_wait,
                 struct iwl_notification_wait *wait_entry)
{
    //IOSimpleLockLock(notif_wait->notif_wait_lock);
    STAILQ_REMOVE_HEAD(&notif_wait->notif_waits, list);
    //IOSimpleLockUnlock(notif_wait->notif_wait_lock);
}

int iwl_wait_notification(struct iwl_notif_wait_data *notif_wait, struct iwl_notification_wait *wait_entry,
                          unsigned long timeout)
{
    int ret = 0;
    
    IOLockLock(notif_wait->notif_waitq);
    AbsoluteTime deadline;
    clock_interval_to_deadline((u32)timeout, kMillisecondScale, (UInt64 *) &deadline);
    
    ret = IOLockSleepDeadline(notif_wait->notif_waitq, wait_entry, deadline, THREAD_INTERRUPTIBLE);
    iwl_remove_notification(notif_wait, wait_entry);
    IOLockUnlock(notif_wait->notif_waitq);

    if (wait_entry->aborted)
        return -EIO;

    /* return value is always >= 0 */
    if (ret != THREAD_AWAKENED)
        return -ETIMEDOUT;
    return 0;
}
