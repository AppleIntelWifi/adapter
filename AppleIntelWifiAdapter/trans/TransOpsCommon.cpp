//
//  TransOpsCommon.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/5.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransOps.h"
#include "../fw/NotificationWait.hpp"


bool IWLTransOps::setHWRFKillState(bool state)
{
    bool rfkill_safe_init_done = trans->m_pDevice->rfkill_safe_init_done;
    bool unified = iwl_mvm_has_unified_ucode(trans->m_pDevice);
    if (state)
        set_bit(IWL_MVM_STATUS_HW_RFKILL, &trans->m_pDevice->status);
    else
        clear_bit(IWL_MVM_STATUS_HW_RFKILL, &trans->m_pDevice->status);
    
    //    iwl_mvm_set_rfkill_state(mvm);
    bool rfkill_state = iwl_mvm_is_radio_killed(trans->m_pDevice);
    if (rfkill_state) {
        IOLockWakeup(trans->m_pDevice->rx_sync_waitq, NULL, true);
    }
    
    
    /* iwl_run_init_mvm_ucode is waiting for results, abort it. */
    if (rfkill_safe_init_done)
                iwl_abort_notification_waits(&trans->m_pDevice->notif_wait);
        
    /*
     * Don't ask the transport to stop the firmware. We'll do it
     * after cfg80211 takes us down.
     */
        if (unified)
            return false;
    
    /*
     * Stop the device if we run OPERATIONAL firmware or if we are in the
     * middle of the calibrations.
     */
    return state && rfkill_safe_init_done;
}

void IWLTransOps::setRfKillState(bool state)
{
    IWL_WARN(0, "reporting RF_KILL (radio %s)\n",
             state ? "disabled" : "enabled");
    if (setHWRFKillState(state)) {
        stopDeviceDirectly();
    }
}

bool IWLTransOps::checkHWRFKill()
{
    bool hw_rfkill = trans->isRFKikkSet();
    bool prev = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    bool report;
    
    if (hw_rfkill) {
        set_bit(STATUS_RFKILL_HW, &trans->status);
        set_bit(STATUS_RFKILL_OPMODE, &trans->status);
    } else {
        clear_bit(STATUS_RFKILL_HW, &trans->status);
        if (trans->opmode_down)
            clear_bit(STATUS_RFKILL_OPMODE, &trans->status);
    }
    report = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    if (prev != report)
        setRfKillState(report);
    return hw_rfkill;
}

void IWLTransOps::irqRfKillHandle()
{
    struct isr_statistics *isr_stats = &trans->isr_stats;
    bool hw_rfkill, prev, report;
    
    IOLockLock(trans->mutex);
    prev = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    hw_rfkill = trans->isRFKikkSet();
    if (hw_rfkill) {
        set_bit(STATUS_RFKILL_OPMODE, &trans->status);
        set_bit(STATUS_RFKILL_HW, &trans->status);
    }
    if (trans->opmode_down)
        report = hw_rfkill;
    else
        report = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    
    IWL_WARN(0, "RF_KILL bit toggled to %s.\n",
             hw_rfkill ? "disable radio" : "enable radio");
    
    isr_stats->rfkill++;
    
    if (prev != report)
        setRfKillState(report);
    IOLockUnlock(trans->mutex);
    
    if (hw_rfkill) {
        if (test_and_clear_bit(STATUS_SYNC_HCMD_ACTIVE,
                               &trans->status))
            IWL_INFO(0,
                     "Rfkill while SYNC HCMD in flight\n");
        IOLockWakeup(trans->wait_command_queue, this, true);
    } else {
        clear_bit(STATUS_RFKILL_HW, &trans->status);
        if (trans->opmode_down)
            clear_bit(STATUS_RFKILL_OPMODE, &trans->status);
    }
}

void IWLTransOps::handleStopRFKill(bool was_in_rfkill)
{
    bool hw_rfkill;
    /*
     * Check again since the RF kill state may have changed while
     * all the interrupts were disabled, in this case we couldn't
     * receive the RF kill interrupt and update the state in the
     * op_mode.
     * Don't call the op_mode if the rkfill state hasn't changed.
     * This allows the op_mode to call stop_device from the rfkill
     * notification without endless recursion. Under very rare
     * circumstances, we might have a small recursion if the rfkill
     * state changed exactly now while we were called from stop_device.
     * This is very unlikely but can happen and is supported.
     */
    hw_rfkill = trans->isRFKikkSet();
    if (hw_rfkill) {
        set_bit(STATUS_RFKILL_HW, &trans->status);
        set_bit(STATUS_RFKILL_OPMODE, &trans->status);
    } else {
        clear_bit(STATUS_RFKILL_HW, &trans->status);
        clear_bit(STATUS_RFKILL_OPMODE, &trans->status);
    }
    if (hw_rfkill != was_in_rfkill)
        setRfKillState(hw_rfkill);
}

IWLTransOps::IWLTransOps(IWLTransport* trans) {
    this->trans = trans;
}

int IWLTransOps::startHW()
{
    int err;
    err = trans->prepareCardHW();
    if (err) {
        IWL_ERR(0, "Error while preparing HW: %d\n", err);
        return err;
    }
    err = trans->clearPersistenceBit();
    if (err) {
        return err;
    }
    trans->swReset();
    if ((err = forcePowerGating())) {
        return err;
    }
    err = apmInit();
    if (err) {
        return err;
    }
    trans->initMsix();
    
    
    /* From now on, the op_mode will be kept updated about RF kill state */
    trans->enableRFKillIntr();
    trans->opmode_down = false;
    /* Set is_down to false here so that...*/
    trans->is_down = false;
    checkHWRFKill();
    return 0;
}

bool IWLTransOps::fwRunning() {
    return test_bit(IWL_MVM_STATUS_FIRMWARE_RUNNING, &this->trans->status);
}
