//
//  IWLMvmTransOpsGen1.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmTransOpsGen1.hpp"

#define super IWLTransOps

int IWLMvmTransOpsGen1::nicInit()
{
    int ret;
    
    /* nic_init */
    IOSimpleLockLock(trans->irq_lock);
    ret = apmInit();
    IOSimpleLockUnlock(trans->irq_lock);
    
    if (ret)
        return ret;
    
    setPwr(false);
    
    nicConfig();
    
    /* Allocate the RX queue, or reset if it is already allocated */
    rxInit();
    
    /* Allocate or reset and init all Tx and Command queues */
    if (txInit())
        return -ENOMEM;
    
    if (trans->m_pDevice->cfg->trans.base_params->shadow_reg_enable) {
        /* enable shadow regs in HW */
        trans->setBit(CSR_MAC_SHADOW_REG_CTRL, 0x800FFFFF);
        IWL_INFO(trans, "Enabling shadow registers in device\n");
    }
    
    return 0;
    return 0;
}

int IWLMvmTransOpsGen1::rxInit()
{
    
    return 0;
}

int IWLMvmTransOpsGen1::txInit()
{
    
    return 0;
}

void IWLMvmTransOpsGen1::setPwr(bool vaux)
{
    if (trans->m_pDevice->cfg->apmg_not_supported)
        return;
    
    //TODO check pcie
    if (vaux /*&& pci_pme_capable(to_pci_dev(trans->dev), PCI_D3cold)*/)
        trans->iwlSetBitsMaskPRPH(APMG_PS_CTRL_REG,
                                  APMG_PS_CTRL_VAL_PWR_SRC_VAUX,
                                  ~APMG_PS_CTRL_MSK_PWR_SRC);
    else
        trans->iwlSetBitsMaskPRPH(APMG_PS_CTRL_REG,
                                  APMG_PS_CTRL_VAL_PWR_SRC_VMAIN,
                                  ~APMG_PS_CTRL_MSK_PWR_SRC);
}

void IWLMvmTransOpsGen1::fwAlive(UInt32 scd_addr)
{
    
}

int IWLMvmTransOpsGen1::startFW()
{
    return 0;
}

void IWLMvmTransOpsGen1::stopDevice()
{
    bool was_in_rfkill;
    IOLockLock(trans->mutex);
    trans->opmode_down = true;
    was_in_rfkill = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    stopDeviceDirectly();
    handleStopRFKill(was_in_rfkill);
    IOLockUnlock(trans->mutex);
}

void IWLMvmTransOpsGen1::stopDeviceDirectly()
{
    if (trans->is_down)
        return;
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

int IWLMvmTransOpsGen1::apmInit()
{
    IWL_INFO(0, "Init card's basic functions\n");
    /*
     * Use "set_bit" below rather than "write", to preserve any hardware
     * bits already set by default after reset.
     */
    
    /* Disable L0S exit timer (platform NMI Work/Around) */
    if (trans->m_pDevice->cfg->trans.device_family < IWL_DEVICE_FAMILY_8000)
        trans->setBit(CSR_GIO_CHICKEN_BITS,
                      CSR_GIO_CHICKEN_BITS_REG_BIT_DIS_L0S_EXIT_TIMER);
    
    /*
     * Disable L0s without affecting L1;
     *  don't wait for ICH L0s (ICH bug W/A)
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
    
    /* Configure analog phase-lock-loop before activating to D0A */
    if (trans->m_pDevice->cfg->trans.base_params->pll_cfg)
        trans->setBit(CSR_ANA_PLL_CFG, CSR50_ANA_PLL_CFG_VAL);
    
    int ret = trans->finishNicInit();
    if (ret)
        return ret;
    
    if (trans->m_pDevice->cfg->host_interrupt_operation_mode) {
        /*
         * This is a bit of an abuse - This is needed for 7260 / 3160
         * only check host_interrupt_operation_mode even if this is
         * not related to host_interrupt_operation_mode.
         *
         * Enable the oscillator to count wake up time for L1 exit. This
         * consumes slightly more power (100uA) - but allows to be sure
         * that we wake up from L1 on time.
         *
         * This looks weird: read twice the same register, discard the
         * value, set a bit, and yet again, read that same register
         * just to discard the value. But that's the way the hardware
         * seems to like it.
         */
        trans->iwlReadPRPH(OSC_CLK);
        trans->iwlReadPRPH(OSC_CLK);
        trans->iwlSetBitsPRPH(OSC_CLK, OSC_CLK_FORCE_CONTROL);
        trans->iwlReadPRPH(OSC_CLK);
        trans->iwlReadPRPH(OSC_CLK);
    }
    
    /*
     * Enable DMA clock and wait for it to stabilize.
     *
     * Write to "CLK_EN_REG"; "1" bits enable clocks, while "0"
     * bits do not disable clocks.  This preserves any hardware
     * bits already set by default in "CLK_CTRL_REG" after reset.
     */
    if (!trans->m_pDevice->cfg->apmg_not_supported) {
        trans->iwlWritePRPH(APMG_CLK_EN_REG,
                            APMG_CLK_VAL_DMA_CLK_RQT);
        _udelay(20);
        /* Disable L1-Active */
        trans->iwlSetBitsPRPH(APMG_PCIDEV_STT_REG,
                              APMG_PCIDEV_STT_VAL_L1_ACT_DIS);
        /* Clear the interrupt in APMG if the NIC is in RFKILL */
        trans->iwlWritePRPH(APMG_RTC_INT_STT_REG,
                            APMG_RTC_INT_STT_RFKILL);
    }
    set_bit(STATUS_DEVICE_ENABLED, &trans->status);
    return 0;
}

void IWLMvmTransOpsGen1::apmStop(bool op_mode_leave)
{
    IWL_INFO(trans, "Stop card, put in low power state\n");
    if (op_mode_leave) {
        if (!test_bit(STATUS_DEVICE_ENABLED, &trans->status))
            apmInit();
        /* inform ME that we are leaving */
        if (trans->m_pDevice->cfg->trans.device_family == IWL_DEVICE_FAMILY_7000)
            trans->iwlSetBitsPRPH(APMG_PCIDEV_STT_REG,
                                  APMG_PCIDEV_STT_VAL_WAKE_ME);
        else if (trans->m_pDevice->cfg->trans.device_family >=
                 IWL_DEVICE_FAMILY_8000) {
            trans->setBit(CSR_DBG_LINK_PWR_MGMT_REG,
                          CSR_RESET_LINK_PWR_MGMT_DISABLED);
            trans->setBit(CSR_HW_IF_CONFIG_REG,
                          CSR_HW_IF_CONFIG_REG_PREPARE |
                          CSR_HW_IF_CONFIG_REG_ENABLE_PME);
            _mdelay(1);
            trans->clearBit(CSR_DBG_LINK_PWR_MGMT_REG,
                            CSR_RESET_LINK_PWR_MGMT_DISABLED);
        }
        _mdelay(5);
    }
    clear_bit(STATUS_DEVICE_ENABLED, &trans->status);
    /* Stop device's DMA activity */
    trans->apmStopMaster();
    
    if (trans->m_pDevice->cfg->lp_xtal_workaround) {
        trans->apmLpXtalEnable();
        return;
    }
    trans->swReset();
    /*
     * Clear "initialization complete" bit to move adapter from
     * D0A* (powered-up Active) --> D0U* (Uninitialized) state.
     */
    trans->clearBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
}
