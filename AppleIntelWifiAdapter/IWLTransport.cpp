//
//  IWLTransport.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"
#include "IWLMvmTransOpsGen2.hpp"
#include "IWLMvmTransOpsGen1.hpp"
#include "IWLDebug.h"

#define super IWLIO

IWLTransport::IWLTransport()
{
}

IWLTransport::~IWLTransport()
{
}

bool IWLTransport::init(IWLDevice *device)
{
    if (!super::init(device)) {
        IOLog("IWLTransport init fail\n");
        return false;
    }
    if (m_pDevice->cfg->trans.gen2) {
        trans_ops = new IWLMvmTransOpsGen2(this);
    } else {
        trans_ops = new IWLMvmTransOpsGen1(this);
    }
    this->irq_lock = IOSimpleLockAlloc();
    disableIntr();
    /*
     * In the 8000 HW family the format of the 4 bytes of CSR_HW_REV have
     * changed, and now the revision step also includes bit 0-1 (no more
     * "dash" value). To keep hw_rev backwards compatible - we'll store it
     * in the old format.
     */
    if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_8000) {
        m_pDevice->hw_rev = (m_pDevice->hw_rev & 0xfff0) | (CSR_HW_REV_STEP(m_pDevice->hw_rev << 2) << 2);
        if (trans_ops->prepareCardHW()) {
            IWL_ERR(0, "Error while preparing HW\n");
            return false;
        }
        if (finishNicInit()) {
            return false;
        }
    }
    m_pDevice->hw_id = (m_pDevice->deviceID << 16) + m_pDevice->subSystemDeviceID;
    m_pDevice->hw_rf_id = iwlRead32(CSR_HW_RF_ID);
    m_pDevice->hw_rev = iwlRead32(CSR_HW_REV);
    if (m_pDevice->hw_rev == 0xffffffff) {
        IWL_ERR(0, "HW_REV=0xFFFFFFFF, PCI issues?\n");
        return false;
    }
    snprintf(m_pDevice->hw_id_str, sizeof(m_pDevice->hw_id_str),
             "PCI ID: 0x%04X:0x%04X", m_pDevice->deviceID, m_pDevice->subSystemDeviceID);
    IOLog("%s\n", m_pDevice->hw_id_str);
    if (this->msix_enabled) {
        //TODO now not needed
//        iwl_pcie_set_interrupt_capa()
    } else {
//        ret = iwl_pcie_alloc_ict(trans);
//        if (ret)
//            goto out_no_pci;
//        ret = devm_request_threaded_irq(&pdev->dev, pdev->irq,
//        iwl_pcie_isr,
//        iwl_pcie_irq_handler,
//        IRQF_SHARED, DRV_NAME, trans);

        this->inta_mask = CSR_INI_SET_MASK;
    }
    IWL_ERR(0, "init succeed\n");
    return true;
}

bool IWLTransport::finishNicInit()
{
    IWL_INFO(0, "finishNicInit\n");
    if (m_pDevice->cfg->trans.bisr_workaround) {
        IODelay(2);
    }
    /*
    * Set "initialization complete" bit to move adapter from
    * D0U* --> D0A* (powered-up active) state.
    */
    setBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
    if (m_pDevice->cfg->trans.device_family == IWL_DEVICE_FAMILY_8000) {
        IODelay(2);
    }
    /*
    * Wait for clock stabilization; once stabilized, access to
    * device-internal resources is supported, e.g. iwl_write_prph()
    * and accesses to uCode SRAM.
    */
    int err = iwlPollBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY, CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY, 25000);
    if (err < 0)
        IWL_INFO(0, "Failed to wake NIC\n");
    if (m_pDevice->cfg->trans.bisr_workaround) {
        /* ensure BISR shift has finished */
        IODelay(200);
    }
    return err < 0 ? err : 0;
}

void IWLTransport::release()
{
    if (this->irq_lock) {
        IOSimpleLockFree(this->irq_lock);
        this->irq_lock = NULL;
    }
    super::release();
}

void IWLTransport::initMsix()
{
    configMsixHw();
    if (!msix_enabled) {
        return;
    }
    this->fh_init_mask = ~iwlRead32(CSR_MSIX_FH_INT_MASK_AD);
    this->fh_mask = this->fh_init_mask;
    this->hw_init_mask = ~iwlRead32(CSR_MSIX_HW_INT_MASK_AD);
    this->hw_mask = this->fh_init_mask;
}

void IWLTransport::configMsixHw()
{
    if (!this->msix_enabled) {
        if (m_pDevice->cfg->trans.mq_rx_supported &&
            test_bit(STATUS_DEVICE_ENABLED, &this->status))
            iwlWriteUmacPRPH(UREG_CHICK,
                             UREG_CHICK_MSI_ENABLE);
        return;
    }
    /*
     * The IVAR table needs to be configured again after reset,
     * but if the device is disabled, we can't write to
     * prph.
     */
    if (test_bit(STATUS_DEVICE_ENABLED, &this->status)) {
        iwlWriteUmacPRPH(UREG_CHICK, UREG_CHICK_MSIX_ENABLE);
    }
    
    /*
     * Each cause from the causes list above and the RX causes is
     * represented as a byte in the IVAR table. The first nibble
     * represents the bound interrupt vector of the cause, the second
     * represents no auto clear for this cause. This will be set if its
     * interrupt vector is bound to serve other causes.
     */
    //TODO
    //iwl_pcie_map_rx_causes(trans);
    //iwl_pcie_map_non_rx_causes(trans);
}

void IWLTransport::disableIntr()
{
    IWL_INFO(0, "disable interrupt\n");
    IOSimpleLockLock(this->irq_lock);
    clear_bit(STATUS_INT_ENABLED, &this->status);
    if (!msix_enabled) {
        /* disable interrupts from uCode/NIC to host */
        iwlWrite32(CSR_INT_MASK, 0x00000000);
        /* acknowledge/clear/reset any interrupts still pending
         * from uCode or flow handler (Rx/Tx DMA) */
        iwlWrite32(CSR_INT, 0xffffffff);
        iwlWrite32(CSR_FH_INT_STATUS, 0xffffffff);
    } else {
        /* disable all the interrupt we might use */
        iwlWrite32(CSR_MSIX_FH_INT_MASK_AD,
                   fh_init_mask);
        iwlWrite32(CSR_MSIX_HW_INT_MASK_AD,
                   hw_init_mask);
    }
    IOSimpleLockUnlock(this->irq_lock);
}

void IWLTransport::enableIntr()
{
    IWL_INFO(0, "enable interrupt\n");
    IOSimpleLockLock(this->irq_lock);
    set_bit(STATUS_INT_ENABLED, &this->status);
    if (!msix_enabled) {
        this->inta_mask = CSR_INI_SET_MASK;
        iwlWrite32(CSR_INT_MASK, this->inta_mask);
    } else {
        /*
         * fh/hw_mask keeps all the unmasked causes.
         * Unlike msi, in msix cause is enabled when it is unset.
         */
        this->hw_mask = this->hw_init_mask;
        this->fh_mask = this->fh_init_mask;
        iwlWrite32(CSR_MSIX_FH_INT_MASK_AD,
                   ~this->fh_mask);
        iwlWrite32(CSR_MSIX_HW_INT_MASK_AD,
                   ~this->hw_mask);
    }
    IOSimpleLockUnlock(this->irq_lock);
}

IWLTransOps *IWLTransport::getTransOps()
{
    return trans_ops;
}

void IWLTransport::setPMI(bool state)
{
    if (state) {
        set_bit(STATUS_TPOWER_PMI, &this->status);
    } else {
        clear_bit(STATUS_TPOWER_PMI, &this->status);
    }
}

