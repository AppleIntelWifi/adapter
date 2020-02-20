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
    IWL_INFO(0, "gen1 nic init");
    int ret;
    /* nic_init */
    IOSimpleLockLock(trans->irq_lock);
    ret = apmInit();
    IOSimpleLockUnlock(trans->irq_lock);
    if (ret)
        return ret;
    
    if(trans->m_pDevice->cfg->trans.device_family == IWL_DEVICE_FAMILY_7000)
        setPwr(false);
    
    nicConfig();
    
    IWL_INFO(0, "Allocating both queues\n");
    
    /* Allocate the RX queue, or reset if it is already allocated */
    if(rxInit())
        return -ENOMEM;
    /* Allocate or reset and init all Tx and Command queues */
    if (txInit())
        return -ENOMEM;
    if (trans->m_pDevice->cfg->trans.base_params->shadow_reg_enable) {
        /* enable shadow regs in HW */
        trans->setBit(CSR_MAC_SHADOW_REG_CTRL, 0x800FFFFF);
        IWL_INFO(trans, "Enabling shadow registers in device\n");
    }

    
    
    ieee80211com* ic = &trans->m_pDevice->ie_ic;
    ic->ic_phytype = IEEE80211_T_OFDM;
    ic->ic_opmode = IEEE80211_M_STA;
    ic->ic_caps =
            IEEE80211_C_IBSS |
            IEEE80211_C_WEP |
            IEEE80211_C_TX_AMPDU |
            IEEE80211_C_PMGT |
            IEEE80211_C_SHSLOT |
            IEEE80211_C_SHPREAMBLE |
            IEEE80211_C_RSN |
            IEEE80211_C_QOS |
            IEEE80211_C_TXPMGT
            //IEEE80211_C_BGSCAN     
            ;
    
    
    for(int i = 0; i < NUM_PHY_CTX; i++) {
        trans->m_pDevice->phy_ctx[i].id = i;
        trans->m_pDevice->phy_ctx[i].color = 0;
        trans->m_pDevice->phy_ctx[i].ref = 0;
        trans->m_pDevice->phy_ctx[i].channel = NULL;
        
    }
    
    
    
    
    
    
    return 0;
}

int IWLMvmTransOpsGen1::rxInit()
{
    IWL_INFO(0, "gen1 rx init");
    int ret = trans->rxInit();
    if (ret) {
        return ret;
    }
    if (trans->m_pDevice->cfg->trans.mq_rx_supported)
        trans->rxMqHWInit();
    else
        trans->rxHWInit(trans->rxq);
    trans->rxqRestok(trans->rxq);
    IOSimpleLockLock(trans->rxq->lock);
    trans->rxqIncWrPtr(trans->rxq);
    IOSimpleLockUnlock(trans->rxq->lock);
    return 0;
}

int IWLMvmTransOpsGen1::txInit()
{
    int ret = trans->txInit();
    return ret;
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
    trans->state = IWL_TRANS_FW_ALIVE;
    trans->resetICT();
    trans->txStart();
}

int IWLMvmTransOpsGen1::startFW(const struct fw_img *fw, bool run_in_rfkill)
{
    clear_bit(STATUS_FW_ERROR, &trans->status);
    trans->state = IWL_TRANS_FW_ALIVE;
    bool hw_rfkill;
    int ret;
    
    IWL_INFO(trans, "Starting firmware\n");
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
    
    //TODO
//    /* Make sure it finished running */
//    iwl_pcie_synchronize_irqs(trans);
    
    IWL_INFO(trans, "Locking mutex\n");
    
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
    
    /*
     * Now, we load the firmware and don't want to be interrupted, even
     * by the RF-Kill interrupt (hence mask all the interrupt besides the
     * FH_TX interrupt which is needed to load the firmware). If the
     * RF-Kill switch is toggled, we will find out after having loaded
     * the firmware and return the proper value to the caller.
     */
    
    IWL_INFO(trans, "Enabling fw load interrupt\n");
    trans->enableFWLoadIntr();
    
    /* really make sure rfkill handshake bits are cleared */
    trans->iwlWrite32(CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_SW_BIT_RFKILL);
    trans->iwlWrite32(CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_SW_BIT_RFKILL);
    
    /* Load the given image to the HW */
    
    IWL_INFO(trans, "Loading ucode\n");
    if (trans->m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_8000)
        trans->loadGivenUcode8000(fw);
    else
        trans->loadGivenUcode(fw);
    
    /* re-check RF-Kill state since we may have missed the interrupt */
    hw_rfkill = checkHWRFKill();
    if (hw_rfkill && !run_in_rfkill)
        ret = -ERFKILL;
    
out:
    IOLockUnlock(trans->mutex);
    return ret;
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
    apmStop(true);
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
    /*
     * Clear "initialization complete" bit to move adapter from
     * D0A* (powered-up Active) --> D0U* (Uninitialized) state.
     */
    trans->clearBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
}
