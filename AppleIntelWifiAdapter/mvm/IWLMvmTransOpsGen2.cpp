//
//  IWLMvmTransOpsGen2.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmTransOpsGen2.hpp"
#include "IWLDebug.h"
#include "../device/IWLCSR.h"
#include "../fw/api/txq.h"

#define super IWLTransOps

int IWLMvmTransOpsGen2::nicInit()
{
    int queue_size = max_t(u32, IWL_CMD_QUEUE_SIZE,
                           trans->m_pDevice->cfg->min_txq_size);
    
    /* TODO: most of the logic can be removed in A0 - but not in Z0 */
    IOSimpleLockLock(trans->irq_lock);
    apmInit();
    IOSimpleLockUnlock(trans->irq_lock);
    
    nicConfig();
    
    /* Allocate the RX queue, or reset if it is already allocated */
    if (rxInit())
        return -ENOMEM;
    
    /* Allocate or reset and init all Tx and Command queues */
    if (txInit(trans->cmd_queue, queue_size))
        return -ENOMEM;
    
    /* enable shadow regs in HW */
    trans->setBit(CSR_MAC_SHADOW_REG_CTRL, 0x800FFFFF);
    IWL_INFO(trans, "Enabling shadow registers in device\n");
    return 0;
}

int IWLMvmTransOpsGen2::rxInit()
{
    /* Set interrupt coalescing timer to default (2048 usecs) */
    trans->iwlWrite8(CSR_INT_COALESCING, IWL_HOST_INT_TIMEOUT_DEF);
    return trans->rxInit();
}

int IWLMvmTransOpsGen2::txInit(int txq_id, int queue_size)
{
    
    return 0;
}

int IWLMvmTransOpsGen2::apmInit()
{
    int ret = 0;
    IWL_INFO(trans, "Init card's basic functions\n");
    /*
     * Use "set_bit" below rather than "write", to preserve any hardware
     * bits already set by default after reset.
     */
    /*
     * Disable L0s without affecting L1;
     * don't wait for ICH L0s (ICH bug W/A)
     */
    trans->setBit(CSR_GIO_CHICKEN_BITS,
                  CSR_GIO_CHICKEN_BITS_REG_BIT_L1A_NO_L0S_RX);
    /* Set FH wait threshold to maximum (HW error during stress W/A) */
    trans->setBit(CSR_DBG_HPET_MEM_REG, CSR_DBG_HPET_MEM_REG_VAL);
    /*
     * Enable HAP INTA (interrupt from management bus) to
     * wake device's PCI Express link L1a -> L0s
     */
    trans->setBit(CSR_HW_IF_CONFIG_REG,
                  CSR_HW_IF_CONFIG_REG_BIT_HAP_WAKE_L1A);
    trans->apmConfig();
    ret = trans->finishNicInit();
    if (ret)
        return ret;
    set_bit(STATUS_DEVICE_ENABLED, &trans->status);
    return 0;
}

void IWLMvmTransOpsGen2::apmStop(bool op_mode_leave)
{
    IWL_INFO(trans, "Stop card, put in low power state\n");
    if (op_mode_leave) {
        if (!test_bit(STATUS_DEVICE_ENABLED, &trans->status))
            apmInit();
        /* inform ME that we are leaving */
        trans->setBit(CSR_DBG_LINK_PWR_MGMT_REG,
                      CSR_RESET_LINK_PWR_MGMT_DISABLED);
        trans->setBit(CSR_HW_IF_CONFIG_REG,
                      CSR_HW_IF_CONFIG_REG_PREPARE |
                      CSR_HW_IF_CONFIG_REG_ENABLE_PME);
        _mdelay(1);
        trans->clearBit(CSR_DBG_LINK_PWR_MGMT_REG,
                        CSR_RESET_LINK_PWR_MGMT_DISABLED);
        _mdelay(5);
    }
    clear_bit(STATUS_DEVICE_ENABLED, &trans->status);
    /* Stop device's DMA activity */
    trans->apmStopMaster();
    trans->swReset();
    /*
     * Clear "initialization complete" bit to move adapter from
     * D0A* (powered-up Active) --> D0U* (Uninitialized) state.
     */
    trans->clearBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
}

void IWLMvmTransOpsGen2::fwAlive(UInt32 scd_addr)
{
    trans->state = IWL_TRANS_FW_ALIVE;
    trans->resetICT();
    /* make sure all queue are not stopped/used */
    memset(trans->queue_stopped, 0, sizeof(trans->queue_stopped));
    memset(trans->queue_used, 0, sizeof(trans->queue_used));
    
    /* now that we got alive we can free the fw image & the context info.
     * paging memory cannot be freed included since FW will still use it
     */
    trans->iwl_pcie_ctxt_info_free();
    
    /*
     * Re-enable all the interrupts, including the RF-Kill one, now that
     * the firmware is alive.
     */
    trans->enableIntr();
    IOLockLock(trans->mutex);
    checkHWRFKill();
    IOLockUnlock(trans->mutex);
}

int IWLMvmTransOpsGen2::startFW(const struct fw_img *fw, bool run_in_rfkill)
{
    IWL_INFO(0, "gen2 start firmware\n");
    clear_bit(STATUS_FW_ERROR, &trans->status);
    bool hw_rfkill;
    int ret;
    
    /* This may fail if AMT took ownership of the device */
    if (trans->prepareCardHW()) {
        IWL_WARN(trans, "Exit HW not ready\n");
        ret = -EIO;
        goto out;
    }
    
    trans->enableRFKillIntr();
    
    trans->iwlWrite32(CSR_INT, 0xFFFFFFFF);
    
    /*
     * We enabled the RF-Kill interrupt and the handler may very
     * well be running. Disable the interrupts to make sure no other
     * interrupt can be fired.
     */
    trans->disableIntr();
    
    //        /* Make sure it finished running */
    //        iwl_pcie_synchronize_irqs(trans);
    IOLockLock(trans->mutex);
    
    /* If platform's RF_KILL switch is NOT set to KILL */
    hw_rfkill = checkHWRFKill();
    if (hw_rfkill && !run_in_rfkill) {
        ret = -ERFKILL;
        goto out;
    }
    
    /* Someone called stop_device, don't try to start_fw */
    if (trans->is_down) {
        IWL_WARN(trans,
                 "Can't start_fw since the HW hasn't been started\n");
        ret = -EIO;
        goto out;
    }
    
    /* make sure rfkill handshake bits are cleared */
    trans->iwlWrite32(CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_SW_BIT_RFKILL);
    trans->iwlWrite32(CSR_UCODE_DRV_GP1_CLR,
                      CSR_UCODE_DRV_GP1_BIT_CMD_BLOCKED);
    
    /* clear (again), then enable host interrupts */
    trans->iwlWrite32(CSR_INT, 0xFFFFFFFF);
    
    ret = nicInit();
    
    if (ret) {
        IWL_ERR(trans, "Unable to init nic\n");
        goto out;
    }
    
    if (trans->m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_AX210)
        ret = trans->iwl_pcie_ctxt_info_gen3_init(fw);
    else
        ret = trans->iwl_pcie_ctxt_info_init(fw);
    if (ret)
        goto out;
    
    /* re-check RF-Kill state since we may have missed the interrupt */
    hw_rfkill = checkHWRFKill();
    if (hw_rfkill && !run_in_rfkill)
        ret = -ERFKILL;
    
out:
    IOLockUnlock(trans->mutex);
    return ret;
}

void IWLMvmTransOpsGen2::stopDevice()
{
    bool was_in_rfkill;
    IOLockLock(trans->mutex);
    trans->opmode_down = true;
    was_in_rfkill = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    stopDeviceDirectly();
    handleStopRFKill(was_in_rfkill);
    IOLockUnlock(trans->mutex);
}

void IWLMvmTransOpsGen2::stopDeviceDirectly()
{
    if (trans->is_down) {
        return;
    }
    trans->is_down = true;
    /* tell the device to stop sending interrupts */
    trans->disableIntr();
    /* device going down, Stop using ICT table */
    trans->disableICT();
    /*
     * If a HW restart happens during firmware loading,
     * then the firmware loading might call this function
     * and later it might be called again due to the
     * restart. So don't process again if the device is
     * already dead.
     */
    if (test_and_clear_bit(STATUS_DEVICE_ENABLED, &trans->status)) {
        IWL_INFO(trans,
                 "DEVICE_ENABLED bit was set and is now cleared\n");
        trans->txStop();
        trans->rxStop();
        /* Power-down device's busmaster DMA clocks */
        if (!trans->m_pDevice->cfg->apmg_not_supported) {
            trans->iwlWritePRPH(APMG_CLK_DIS_REG,
                                APMG_CLK_VAL_DMA_CLK_RQT);
            _udelay(5);
        }
    }
    /* Make sure (redundant) we've released our request to stay awake */
    trans->clearBit(CSR_GP_CNTRL,
                    CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
    /* Stop the device, and put it in low power state */
    apmStop(false);
    trans->swReset();
    /*
     * Upon stop, the IVAR table gets erased, so msi-x won't
     * work. This causes a bug in RF-KILL flows, since the interrupt
     * that enables radio won't fire on the correct irq, and the
     * driver won't be able to handle the interrupt.
     * Configure the IVAR table again after reset.
     */
    trans->configMsixHw();
    /*
     * Upon stop, the APM issues an interrupt if HW RF kill is set.
     * This is a bug in certain verions of the hardware.
     * Certain devices also keep sending HW RF kill interrupt all
     * the time, unless the interrupt is ACKed even if the interrupt
     * should be masked. Re-ACK all the interrupts here.
     */
    trans->disableIntr();
    /* clear all status bits */
    clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
    clear_bit(STATUS_INT_ENABLED, &trans->status);
    clear_bit(STATUS_TPOWER_PMI, &trans->status);
    /*
     * Even if we stop the HW, we still want the RF kill
     * interrupt
     */
    trans->enableRFKillIntr();
    /* re-take ownership to prevent other users from stealing the device */
    trans->prepareCardHW();
}

int IWLMvmTransOpsGen2::forcePowerGating()
{
    if (trans->m_pDevice->cfg->trans.device_family == IWL_DEVICE_FAMILY_22000 &&
        trans->m_pDevice->cfg->trans.integrated) {
        int ret;
        ret = trans->finishNicInit();
        if (ret < 0)
            return ret;
        trans->iwlSetBitsPRPH(HPM_HIPM_GEN_CFG,
                              HPM_HIPM_GEN_CFG_CR_FORCE_ACTIVE);
        _udelay(20);
        trans->iwlSetBitsPRPH(HPM_HIPM_GEN_CFG,
                              HPM_HIPM_GEN_CFG_CR_PG_EN |
                              HPM_HIPM_GEN_CFG_CR_SLP_EN);
        _udelay(20);
        trans->iwlClearBitsPRPH(HPM_HIPM_GEN_CFG,
                                HPM_HIPM_GEN_CFG_CR_FORCE_ACTIVE);
        trans->swReset();
        return 0;
    }
    return 0;
}
