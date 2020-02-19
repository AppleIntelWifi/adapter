//
//  IWLTransport.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"
#include "IWLDebug.h"
#include "IWLFH.h"
#include "tx.h"
#include "IWLSCD.h"

#define super IWLIO

IWLTransport::IWLTransport()
{
}

IWLTransport::~IWLTransport()
{
}

bool IWLTransport::prepareCardHW()
{
    IWL_INFO(0, "prepareCardHW\n");
    int t = 0;
    int ret = setHWReady();
    /* If the card is ready, exit 0 */
    if (ret >= 0)
        return false;
    this->setBit(CSR_DBG_LINK_PWR_MGMT_REG, CSR_RESET_LINK_PWR_MGMT_DISABLED);
    IODelay(2000);
    for (int iter = 0; iter < 10; iter++) {
        /* If HW is not ready, prepare the conditions to check again */
        this->setBit(CSR_HW_IF_CONFIG_REG,
                     CSR_HW_IF_CONFIG_REG_PREPARE);
        do {
            ret = setHWReady();
            if (ret >= 0)
                return false;
            
            IODelay(1000);
            t += 200;
        } while (t < 150000);
        IODelay(25);
    }
    IWL_ERR(0, "Couldn't prepare the card\n");
    return true;
}

#define HW_READY_TIMEOUT (50)

int IWLTransport::setHWReady()
{
    int ret;
    this->setBit(CSR_HW_IF_CONFIG_REG,
                 CSR_HW_IF_CONFIG_REG_BIT_NIC_READY);
    /* See if we got it */
    ret = this->iwlPollBit(CSR_HW_IF_CONFIG_REG,
                           CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
                           CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
                           HW_READY_TIMEOUT);
    if (ret >= 0)
        this->setBit(CSR_MBOX_SET_REG, CSR_MBOX_SET_REG_OS_ALIVE);
    IWL_WARN(0, "hardware%s ready\n", ret < 0 ? " not" : "");
    return ret;
}

bool IWLTransport::init(IWLDevice *device)
{
    if (!super::init(device)) {
        IOLog("IWLTransport init fail\n");
        return false;
    }
    if (!m_pDevice->cfg->trans.gen2) {
        txcmd_size = sizeof(struct iwl_tx_cmd);
        txcmd_align = sizeof(void *);
    } else if (m_pDevice->cfg->trans.device_family < IWL_DEVICE_FAMILY_AX210) {
        txcmd_size = sizeof(struct iwl_tx_cmd_gen2);
        txcmd_align = 64;
    } else {
        txcmd_size = sizeof(struct iwl_tx_cmd_gen3);
        txcmd_align = 128;
    }
    txcmd_size += sizeof(struct iwl_cmd_header);
    txcmd_size += 36; /* biggest possible 802.11 header */
    /* Ensure device TX cmd cannot reach/cross a page boundary in gen2 */
    if (WARN_ON(m_pDevice->cfg->trans.gen2 && txcmd_size >= txcmd_align)) {
        IOLog("Ensure device TX cmd fail\n");
        return false;
    }
    //TODO empty tx queues from [iwl_trans_alloc] may be function [iwl_trans_pcie_wait_txq_empty]
    this->opmode_down = true;
    this->irq_lock = IOSimpleLockAlloc();
    this->mutex = IOLockAlloc();
    this->syncCmdLock = IOLockAlloc();
    //waitlocks
    this->ucode_write_waitq = IOLockAlloc();
    this->wait_command_queue = IOLockAlloc();
    this->def_rx_queue = 0;
    int addr_size;
    if (m_pDevice->cfg->trans.use_tfh) {
        addr_size = 64;
        this->max_tbs = IWL_TFH_NUM_TBS;
        this->tfd_size = sizeof(struct iwl_tfh_tfd);
    } else {
        addr_size = 36;
        this->max_tbs = IWL_NUM_OF_TBS;
        this->tfd_size = sizeof(struct iwl_tfd);
    }
    this->dma_mask = DMA_BIT_MASK(addr_size);
    this->num_rx_queues = 1;//iwl_trans_alloc
    disableIntr();
    
    m_pDevice->hw_id = (m_pDevice->deviceID << 16) + m_pDevice->subSystemDeviceID;
    m_pDevice->hw_rf_id = iwlRead32(CSR_HW_RF_ID);
    m_pDevice->hw_rev = iwlRead32(CSR_HW_REV);
    if (m_pDevice->hw_rev == 0xffffffff) {
        IWL_ERR(0, "HW_REV=0xFFFFFFFF, PCI issues?\n");
        return false;
    }
    
    /*
     * In the 8000 HW family the format of the 4 bytes of CSR_HW_REV have
     * changed, and now the revision step also includes bit 0-1 (no more
     * "dash" value). To keep hw_rev backwards compatible - we'll store it
     * in the old format.
     */
    if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_8000) {
        IWL_INFO(0, "Patch CSR_HW_REV (prev: 0x%x)", m_pDevice->hw_rev);
        m_pDevice->hw_rev = (m_pDevice->hw_rev & 0xfff0) | (CSR_HW_REV_STEP(m_pDevice->hw_rev << 2) << 2);
        if (this->prepareCardHW()) {
            IWL_ERR(0, "Error while preparing HW\n");
            return false;
        }
        
        // recognize the C-step drivers too
        
        
        if (finishNicInit()) {
            return false;
        }
    }
    
    snprintf(m_pDevice->hw_id_str, sizeof(m_pDevice->hw_id_str),
             "PCI ID: 0x%04X:0x%04X", m_pDevice->deviceID, m_pDevice->subSystemDeviceID);
    IOLog("%s\n", m_pDevice->hw_id_str);
    if (this->msix_enabled) {
        this->msix_enabled = false; //OSX does not support MSIX
        //this->inta_mask = CSR_INI_SET_MASK;
        //TODO now not needed
        //        ret = iwl_pcie_init_msix_handler(pdev, trans_pcie);
        //        if (ret)
        //            goto out_no_pci;
    }
        //        ret = iwl_pcie_alloc_ict(trans);
        //        if (ret)
        //            goto out_no_pci;
        //        ret = devm_request_threaded_irq(&pdev->dev, pdev->irq,
        //        iwl_pcie_isr,
        //        iwl_pcie_irq_handler,
        //        IRQF_SHARED, DRV_NAME, trans);
        
        this->inta_mask = CSR_INI_SET_MASK;
    
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
    if (this->mutex) {
        IOLockFree(this->mutex);
        this->mutex = NULL;
    }
    if (this->syncCmdLock) {
        IOLockFree(this->syncCmdLock);
        this->syncCmdLock = NULL;
    }
    if (this->ucode_write_waitq) {
        IOLockFree(this->ucode_write_waitq);
        this->ucode_write_waitq = NULL;
    }
    if (this->wait_command_queue) {
        IOLockFree(this->wait_command_queue);
        this->wait_command_queue = NULL;
    }
    super::release();
}

void IWLTransport::initMsix()
{
    this->msix_enabled = false;
    
    configMsixHw();
    
    IWL_INFO(0, "initMsix (%s)", msix_enabled ? "true" : "false");
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

int IWLTransport::clearPersistenceBit()
{
    u32 hpm, wprot;
    switch (m_pDevice->cfg->trans.device_family) {
        case IWL_DEVICE_FAMILY_9000:
            wprot = PREG_PRPH_WPROT_9000;
            break;
        case IWL_DEVICE_FAMILY_22000:
            wprot = PREG_PRPH_WPROT_22000;
            break;
        default:
            return 0;
    }
    hpm = iwlReadUmacPRPHNoGrab(HPM_DEBUG);
    if (hpm != 0xa5a5a5a0 && (hpm & PERSISTENCE_BIT)) {
        u32 wprot_val = iwlReadUmacPRPHNoGrab(wprot);
        
        if (wprot_val & PREG_WFPM_ACCESS) {
            IWL_ERR(trans,
                    "Error, can not clear persistence bit\n");
            return -EPERM;
        }
        iwlWriteUmacPRPHNoGrab(HPM_DEBUG,
                               hpm & ~PERSISTENCE_BIT);
    }
    return 0;
}

void IWLTransport::disableIntr()
{
    IOSimpleLockLock(this->irq_lock);
    disableIntrDirectly();
    IOSimpleLockUnlock(this->irq_lock);
}

void IWLTransport::disableIntrDirectly()
{
    IWL_INFO(0, "disable interrupt\n");
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
}

void IWLTransport::enableIntr()
{
    IOSimpleLockLock(this->irq_lock);
    enableIntrDirectly();
    IOSimpleLockUnlock(this->irq_lock);
}

void IWLTransport::enableIntrDirectly()
{
    IWL_INFO(0, "enable interrupt\n");
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
}

void IWLTransport::enableRFKillIntr()
{
    IWL_INFO(0, "Enabling rfkill interrupt\n");
    if (!this->msix_enabled) {
        this->inta_mask = CSR_INT_BIT_RF_KILL;
        iwlWrite32(CSR_INT_MASK, this->inta_mask);
    } else {
        iwlWrite32(CSR_MSIX_FH_INT_MASK_AD,
                   this->fh_init_mask);
        enableHWIntrMskMsix(MSIX_HW_INT_CAUSES_REG_RF_KILL);
    }
    if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_9000) {
        /*
         * On 9000-series devices this bit isn't enabled by default, so
         * when we power down the device we need set the bit to allow it
         * to wake up the PCI-E bus for RF-kill interrupts.
         */
        setBit(CSR_GP_CNTRL,
               CSR_GP_CNTRL_REG_FLAG_RFKILL_WAKE_L1A_EN);
    }
}

void IWLTransport::enableHWIntrMskMsix(u32 msk)
{
    iwlWrite32(CSR_MSIX_HW_INT_MASK_AD, ~msk);
    this->hw_mask = msk;
}

void IWLTransport::enableFHIntrMskMsix(u32 msk)
{
    iwlWrite32(CSR_MSIX_FH_INT_MASK_AD, ~msk);
    this->fh_mask = msk;
}

void IWLTransport::enableFWLoadIntr()
{
    IWL_INFO(trans, "Enabling FW load interrupt\n");
    if (!this->msix_enabled) {
        this->inta_mask = CSR_INT_BIT_FH_TX;
        iwlWrite32(CSR_INT_MASK, this->inta_mask);
    } else {
        iwlWrite32(CSR_MSIX_HW_INT_MASK_AD,
                   this->hw_init_mask);
        
        enableFHIntrMskMsix(MSIX_FH_INT_CAUSES_D2S_CH0_NUM);
    }
}

bool IWLTransport::isRFKikkSet()
{
    return !(iwlRead32(CSR_GP_CNTRL) &
             CSR_GP_CNTRL_REG_FLAG_HW_RF_KILL_SW);
}

void IWLTransport::loadFWChunkFh(u32 dst_addr, dma_addr_t phy_addr, u32 byte_cnt)
{
    this->iwlWrite32(FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL), FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_PAUSE);
    this->iwlWrite32(FH_SRVC_CHNL_SRAM_ADDR_REG(FH_SRVC_CHNL),
                     dst_addr);
    
    this->iwlWrite32(FH_TFDIB_CTRL0_REG(FH_SRVC_CHNL),
                     phy_addr & FH_MEM_TFDIB_DRAM_ADDR_LSB_MSK);
    
    this->iwlWrite32(FH_TFDIB_CTRL1_REG(FH_SRVC_CHNL),
                     (iwl_get_dma_hi_addr(phy_addr)
                      << FH_MEM_TFDIB_REG1_ADDR_BITSHIFT) | byte_cnt);
    
    this->iwlWrite32(FH_TCSR_CHNL_TX_BUF_STS_REG(FH_SRVC_CHNL),
                     BIT(FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_NUM) |
                     BIT(FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_IDX) |
                     FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_VALID);
    
    this->iwlWrite32(FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL),
                     FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE |
                     FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_DISABLE |
                     FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_ENDTFD);
}

int IWLTransport::loadFWChunk(u32 dst_addr, dma_addr_t phy_addr, u32 byte_cnt)
{
    IOInterruptState flags;
    int ret = 0;
    this->ucode_write_complete = false;
    
    if (!this->grabNICAccess(&flags)) {
        return -EIO;
    }
    
    loadFWChunkFh(dst_addr, phy_addr, byte_cnt);
    this->releaseNICAccess(&flags);
    IOLockLock(this->ucode_write_waitq);
    AbsoluteTime deadline;
    clock_interval_to_deadline(5, kSecondScale, (UInt64 *) &deadline);
    ret = IOLockSleepDeadline(this->ucode_write_waitq, &this->ucode_write_complete,
                              deadline, THREAD_INTERRUPTIBLE);
    IOLockUnlock(this->ucode_write_waitq);
    IWL_INFO(0, "loadFWChunk wakeup\n");
    if (ret == -1) {
        IWL_ERR(0, "Failed to load firmware chunk!\n");
        return -ETIMEDOUT;
    }
    
    return 0;
}

/* extended range in FW SRAM */
#define IWL_FW_MEM_EXTENDED_START    0x40000
#define IWL_FW_MEM_EXTENDED_END        0x57FFF

int IWLTransport::loadSection(u8 section_num, const struct fw_desc *section)
{
    u32 offset, chunk_sz = min_t(u32, FH_MEM_TB_MAX_LENGTH, section->len);
    int ret = 0;
    IWL_INFO(0, "[%d] uCode section being loaded...\n", section_num);
    IOBufferMemoryDescriptor *bmd = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, 0, section->len, 0x00000000ffffffffULL);
    bmd->prepare();
    IODMACommand *cmd = IODMACommand::withSpecification(kIODMACommandOutputHost32, 32, 0, IODMACommand::kMapped, 0, 1);
    cmd->setMemoryDescriptor(bmd);
    cmd->prepare();
    IODMACommand::Segment32 seg;
    UInt64 ofs = 0;
    UInt32 numSegs = 1;
    if (cmd->gen32IOVMSegments(&ofs, &seg, &numSegs) != kIOReturnSuccess) {
        IWL_ERR(0, "咋了？？\n");
        return -1;
    }
    for (offset = 0; offset < section->len; offset += chunk_sz) {
        u32 copy_size, dst_addr;
        bool extended_addr = false;
        
        copy_size = min(chunk_sz, (u32)(section->len - offset));
        dst_addr = section->offset + offset;
        
        if (dst_addr >= IWL_FW_MEM_EXTENDED_START &&
            dst_addr <= IWL_FW_MEM_EXTENDED_END)
            extended_addr = true;
        
        if (extended_addr)
            this->iwlSetBitsPRPH(LMPM_CHICK, LMPM_CHICK_EXTENDED_ADDR_SPACE);
        
        cmd->writeBytes(0, (u8 *)section->data + offset, copy_size);
        
        ret = loadFWChunk(dst_addr, seg.fIOVMAddr, copy_size);
        
        if (extended_addr)
            this->iwlClearBitsPRPH(LMPM_CHICK, LMPM_CHICK_EXTENDED_ADDR_SPACE);
        
        if (ret) {
            IWL_ERR(trans, "Could not load the [%d] uCode section\n", section_num);
            break;
        }
    }
    IWL_INFO(0, "write ucode complete\n");
    cmd->complete();
    cmd->clearMemoryDescriptor();
    cmd->release();
    return ret;
}

int IWLTransport::loadCPUSections8000(const struct fw_img *image, int cpu, int *first_ucode_section)
{
    int shift_param;
    int i, ret = 0, sec_num = 0x1;
    u32 val, last_read_idx = 0;
    
    if (cpu == 1) {
        shift_param = 0;
        *first_ucode_section = 0;
    } else {
        shift_param = 16;
        (*first_ucode_section)++;
    }
    
    for (i = *first_ucode_section; i < image->num_sec; i++) {
        last_read_idx = i;
        
        /*
         * CPU1_CPU2_SEPARATOR_SECTION delimiter - separate between
         * CPU1 to CPU2.
         * PAGING_SEPARATOR_SECTION delimiter - separate between
         * CPU2 non paged to CPU2 paging sec.
         */
        if (!image->sec[i].data ||
            image->sec[i].offset == CPU1_CPU2_SEPARATOR_SECTION ||
            image->sec[i].offset == PAGING_SEPARATOR_SECTION) {
            IWL_INFO(0,
                     "Break since Data not valid or Empty section, sec = %d\n",
                     i);
            break;
        }
        ret = loadSection(i, &image->sec[i]);
        if (ret)
            return ret;
        
        /* Notify ucode of loaded section number and status */
        val = this->iwlReadDirect32(FH_UCODE_LOAD_STATUS);
        val = val | (sec_num << shift_param);
        this->iwlWriteDirect32(FH_UCODE_LOAD_STATUS, val);
        sec_num = (sec_num << 1) | 0x1;
    }
    
    *first_ucode_section = last_read_idx;
    
    enableIntr();
    
    
    if (m_pDevice->cfg->trans.use_tfh) {
        if (cpu == 1)
            this->iwlWritePRPH(UREG_UCODE_LOAD_STATUS, 0xFFFF);
        else
            this->iwlWritePRPH(UREG_UCODE_LOAD_STATUS, 0xFFFFFFFF);
    } else {
        if (cpu == 1)
            this->iwlWriteDirect32(FH_UCODE_LOAD_STATUS, 0xFFFF);
        else
            this->iwlWriteDirect32(FH_UCODE_LOAD_STATUS, 0xFFFFFFFF);
    }
    
    return 0;
}

int IWLTransport::loadCPUSections(const struct fw_img *image, int cpu, int *first_ucode_section)
{
    int i, ret = 0;
    u32 last_read_idx = 0;
    
    if (cpu == 1)
        *first_ucode_section = 0;
    else
        (*first_ucode_section)++;
    
    for (i = *first_ucode_section; i < image->num_sec; i++) {
        last_read_idx = i;
        
        /*
         * CPU1_CPU2_SEPARATOR_SECTION delimiter - separate between
         * CPU1 to CPU2.
         * PAGING_SEPARATOR_SECTION delimiter - separate between
         * CPU2 non paged to CPU2 paging sec.
         */
        if (!image->sec[i].data ||
            image->sec[i].offset == CPU1_CPU2_SEPARATOR_SECTION ||
            image->sec[i].offset == PAGING_SEPARATOR_SECTION) {
            IWL_INFO(0,
                     "Break since Data not valid or Empty section, sec = %d\n",
                     i);
            break;
        }
        
        ret = loadSection(i, &image->sec[i]);
        if (ret)
            return ret;
    }
    
    *first_ucode_section = last_read_idx;
    
    return 0;
}

int IWLTransport::loadGivenUcode(const struct fw_img *image)
{
    int ret = 0;
    int first_ucode_section;
    
    IWL_INFO(0, "working with %s CPU\n",
             image->is_dual_cpus ? "Dual" : "Single");
    
    /* load to FW the binary non secured sections of CPU1 */
    ret = loadCPUSections(image, 1, &first_ucode_section);
    if (ret)
        return ret;
    
    if (image->is_dual_cpus) {
        /* set CPU2 header address */
        this->iwlWritePRPH(
                           LMPM_SECURE_UCODE_LOAD_CPU2_HDR_ADDR,
                           LMPM_SECURE_CPU2_HDR_MEM_SPACE);
        
        /* load to FW the binary sections of CPU2 */
        ret = loadCPUSections(image, 2, &first_ucode_section);
        if (ret)
            return ret;
    }
    //TODO now not need to enable debug mode
    //
    //        /* supported for 7000 only for the moment */
    //        if (iwlwifi_mod_params.fw_monitor &&
    //            trans->trans_cfg->device_family == IWL_DEVICE_FAMILY_7000) {
    //            struct iwl_dram_data *fw_mon = &trans->dbg.fw_mon;
    //
    //            iwl_pcie_alloc_fw_monitor(trans, 0);
    //            if (fw_mon->size) {
    //                iwl_write_prph(trans, MON_BUFF_BASE_ADDR,
    //                           fw_mon->physical >> 4);
    //                iwl_write_prph(trans, MON_BUFF_END_ADDR,
    //                           (fw_mon->physical + fw_mon->size) >> 4);
    //            }
    //        } else if (iwl_pcie_dbg_on(trans)) {
    //            iwl_pcie_apply_destination(trans);
    //        }
    
    enableIntr();
    
    /* release CPU reset */
    iwlWrite32(CSR_RESET, 0);
    
    return 0;
}

int IWLTransport::loadGivenUcode8000(const struct fw_img *image)
{
    int ret = 0;
    int first_ucode_section;
    
    IWL_INFO(0, "working with %s CPU\n",
             image->is_dual_cpus ? "Dual" : "Single");
    //TODO now not need to enable debug mode
    //        if (iwl_pcie_dbg_on(trans))
    //            iwl_pcie_apply_destination(trans);
    //
    //    #ifdef CPTCFG_IWLWIFI_DEVICE_TESTMODE
    //        iwl_dnt_configure(trans, image);
    //    #endif
    //
    //    #ifdef CPTCFG_IWLWIFI_SUPPORT_DEBUG_OVERRIDES
    //        iwl_pcie_override_secure_boot_cfg(trans);
    //    #endif
    
    IWL_INFO(0, "Original WFPM value = 0x%08X\n",
             this->iwlReadPRPH(WFPM_GP2));
    
    /*
     * Set default value. On resume reading the values that were
     * zeored can provide debug data on the resume flow.
     * This is for debugging only and has no functional impact.
     */
    this->iwlWritePRPH(WFPM_GP2, 0x01010101);
    
    /* configure the ucode to be ready to get the secured image */
    /* release CPU reset */
    this->iwlWritePRPH(RELEASE_CPU_RESET, RELEASE_CPU_RESET_BIT);
    
    /* load to FW the binary Secured sections of CPU1 */
    ret = loadCPUSections8000(image, 1, &first_ucode_section);
    
    IWL_INFO(0, "Load status = %d\n", ret);
    if (ret)
        return ret;
    
    /* load to FW the binary sections of CPU2 */
    IWL_INFO(0, "Loading second CPU\n");
    return loadCPUSections8000(image, 2, &first_ucode_section);
}

void IWLTransport::setPMI(bool state)
{
    if (state) {
        set_bit(STATUS_TPOWER_PMI, &this->status);
    } else {
        clear_bit(STATUS_TPOWER_PMI, &this->status);
    }
}

void IWLTransport::apmConfig()
{
    /*
     * L0S states have been found to be unstable with our devices
     * and in newer hardware they are not officially supported at
     * all, so we must always set the L0S_DISABLED bit.
     */
    
    
    this->setBit(CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_DISABLED);
    //TODO
    //    pcie_capability_read_word(trans_pcie->pci_dev, PCI_EXP_LNKCTL, &lctl);
    //        trans->pm_support = !(lctl & PCI_EXP_LNKCTL_ASPM_L0S);
    //
    //        pcie_capability_read_word(trans_pcie->pci_dev, PCI_EXP_DEVCTL2, &cap);
    //        trans->ltr_enabled = cap & PCI_EXP_DEVCTL2_LTR_EN;
    //        IWL_DEBUG_POWER(trans, "L1 %sabled - LTR %sabled\n",
    //                (lctl & PCI_EXP_LNKCTL_ASPM_L1) ? "En" : "Dis",
    //                trans->ltr_enabled ? "En" : "Dis");
}

void IWLTransport::swReset()
{
    /* Reset entire device - do controller reset (results in SHRD_HW_RST) */
    setBit(CSR_RESET, CSR_RESET_REG_FLAG_SW_RESET);
    usleep_range(5000, 6000);
}

void IWLTransport::apmStopMaster()
{
    int ret;
    
    /* stop device's busmaster DMA activity */
    setBit(CSR_RESET, CSR_RESET_REG_FLAG_STOP_MASTER);
    
    ret = iwlPollBit(CSR_RESET,
                     CSR_RESET_REG_FLAG_MASTER_DISABLED,
                     CSR_RESET_REG_FLAG_MASTER_DISABLED, 100);
    if (ret < 0)
        IWL_WARN(0, "Master Disable Timed Out, 100 usec\n");
    
    IWL_INFO(0, "stop master\n");
}

void IWLTransport::apmLpXtalEnable()
{
    int ret;
    u32 apmg_gp1_reg;
    u32 apmg_xtal_cfg_reg;
    u32 dl_cfg_reg;
    
    /* Force XTAL ON */
    setBit(CSR_GP_CNTRL,
           CSR_GP_CNTRL_REG_FLAG_XTAL_ON);
    swReset();
    ret = finishNicInit();
    if (WARN_ON(ret)) {
        /* Release XTAL ON request */
        clearBit(CSR_GP_CNTRL,
                 CSR_GP_CNTRL_REG_FLAG_XTAL_ON);
        return;
    }
    /*
     * Clear "disable persistence" to avoid LP XTAL resetting when
     * SHRD_HW_RST is applied in S3.
     */
    iwlClearBitsPRPH(APMG_PCIDEV_STT_REG,
                     APMG_PCIDEV_STT_VAL_PERSIST_DIS);
    /*
     * Force APMG XTAL to be active to prevent its disabling by HW
     * caused by APMG idle state.
     */
    apmg_xtal_cfg_reg = iwlReadShr(SHR_APMG_XTAL_CFG_REG);
    iwlWriteShr(SHR_APMG_XTAL_CFG_REG,
                apmg_xtal_cfg_reg |
                SHR_APMG_XTAL_CFG_XTAL_ON_REQ);
    swReset();
    /* Enable LP XTAL by indirect access through CSR */
    apmg_gp1_reg = iwlReadShr(SHR_APMG_GP1_REG);
    iwlWriteShr(SHR_APMG_GP1_REG, apmg_gp1_reg |
                SHR_APMG_GP1_WF_XTAL_LP_EN |
                SHR_APMG_GP1_CHICKEN_BIT_SELECT);
    /* Clear delay line clock power up */
    dl_cfg_reg = iwlReadShr(SHR_APMG_DL_CFG_REG);
    iwlWriteShr(SHR_APMG_DL_CFG_REG, dl_cfg_reg &
                ~SHR_APMG_DL_CFG_DL_CLOCK_POWER_UP);
    /*
     * Enable persistence mode to avoid LP XTAL resetting when
     * SHRD_HW_RST is applied in S3.
     */
    setBit(CSR_HW_IF_CONFIG_REG,
           CSR_HW_IF_CONFIG_REG_PERSIST_MODE);
    /*
     * Clear "initialization complete" bit to move adapter from
     * D0A* (powered-up Active) --> D0U* (Uninitialized) state.
     */
    clearBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
    /* Activates XTAL resources monitor */
    setBit(CSR_MONITOR_CFG_REG,
           CSR_MONITOR_XTAL_RESOURCES);
    /* Release XTAL ON request */
    clearBit(CSR_GP_CNTRL,
             CSR_GP_CNTRL_REG_FLAG_XTAL_ON);
    _udelay(10);
    /* Release APMG XTAL */
    iwlWriteShr(SHR_APMG_XTAL_CFG_REG,
                apmg_xtal_cfg_reg &
                ~SHR_APMG_XTAL_CFG_XTAL_ON_REQ);
}

void IWLTransport::freeResp(struct iwl_host_cmd *cmd)
{
    if (cmd->resp_pkt) {
        //IOFree(cmd->resp_pkt, cmd->resp_pkt_len);
        IWL_WARN(0, "LEAKING PAGE\n");
        //this->m_pDevice->controller->freePacket((mbuf_t)cmd->resp_pkt);
    }
    cmd->resp_pkt = NULL;
}

int IWLTransport::sendCmd(iwl_host_cmd *cmd)
{
    int ret;
    if (unlikely(!(cmd->flags & CMD_SEND_IN_RFKILL) &&
                 test_bit(STATUS_RFKILL_OPMODE, &this->status)))
        return -ERFKILL;
    if (unlikely(test_bit(STATUS_FW_ERROR, &this->status)))
        return -EIO;
    if (unlikely(this->state != IWL_TRANS_FW_ALIVE)) {
        IWL_ERR(trans, "%s bad state = %d\n", __func__, this->state);
        return -EIO;
    }
    if (WARN_ON((cmd->flags & CMD_WANT_ASYNC_CALLBACK) &&
                !(cmd->flags & CMD_ASYNC)))
        return -EINVAL;
    if (this->wide_cmd_header && !iwl_cmd_groupid(cmd->id))
        cmd->id = DEF_ID(cmd->id);
    if (!(cmd->flags & CMD_ASYNC)) {
        IOLockLock(this->syncCmdLock);
    }
    ret = pcieSendHCmd(cmd);
    if (!(cmd->flags & CMD_ASYNC)) {
        IOLockUnlock(this->syncCmdLock);
    }
    return ret;
}

void iwl_trans_ac_txq_enable(IWLTransport *trans, int queue, int fifo,
                             unsigned int queue_wdg_timeout);

void IWLTransport::txStart()
{
    int nq = m_pDevice->cfg->trans.base_params->num_of_queues;
    int chan;
    u32 reg_val;
    int clear_dwords = (SCD_TRANS_TBL_OFFSET_QUEUE(nq) -
                        SCD_CONTEXT_MEM_LOWER_BOUND) / sizeof(u32);
    
    /* make sure all queue are not stopped/used */
    memset(this->queue_stopped, 0, sizeof(this->queue_stopped));
    memset(this->queue_used, 0, sizeof(this->queue_used));
    
    this->scd_base_addr =
    iwlReadPRPH(SCD_SRAM_BASE_ADDR);
    
    IWL_INFO(0, "scd_base_addr: %x\n", this->scd_base_addr);
    
    WARN_ON(scd_base_addr != 0 &&
            scd_base_addr != this->scd_base_addr);
    
    /* reset context data, TX status and translation data */
    iwlWriteMem(this->scd_base_addr +
                SCD_CONTEXT_MEM_LOWER_BOUND,
                NULL, clear_dwords);
    
    IWL_INFO(0, "scd_bc_tbls->dma: %x\n", this->scd_bc_tbls->dma);
    iwlWritePRPH(SCD_DRAM_BASE_ADDR,
                 this->scd_bc_tbls->dma >> 10);
    
    /* The chain extension of the SCD doesn't work well. This feature is
     * enabled by default by the HW, so we need to disable it manually.
     */
    if (m_pDevice->cfg->trans.base_params->scd_chain_ext_wa)
        iwlWritePRPH(SCD_CHAINEXT_EN, 0);
    
    iwl_trans_ac_txq_enable(this, this->cmd_queue,
                            this->cmd_fifo,
                            this->cmd_q_wdg_timeout);
    
    /* Activate all Tx DMA/FIFO channels */
    iwl_scd_activate_fifos(this);
    //iwlWritePRPH(SCD_TXFACT, IWL_MASK(0, 7));
    
    /* Enable DMA channel */
    for (chan = 0; chan < FH_TCSR_CHNL_NUM; chan++)
        iwlWriteDirect32(FH_TCSR_CHNL_TX_CONFIG_REG(chan),
                         FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE |
                         FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_ENABLE);
    
    /* Update FH chicken bits */
    reg_val = iwlReadDirect32(FH_TX_CHICKEN_BITS_REG);
    iwlWriteDirect32(FH_TX_CHICKEN_BITS_REG,
                     reg_val | FH_TX_CHICKEN_BITS_SCD_AUTO_RETRY_EN);
    
    /* Enable L1-Active */
    if (m_pDevice->cfg->trans.device_family < IWL_DEVICE_FAMILY_8000)
        iwlClearBitsPRPH(APMG_PCIDEV_STT_REG,
                         APMG_PCIDEV_STT_VAL_L1_ACT_DIS);
}

void iwl_pcie_clear_cmd_in_flight(IWLTransport *trans)
{
    //struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    //lockdep_assert_held(&trans_pcie->reg_lock);
    //trans->cmd
    /*
    if (trans->cmd_in_flight) {
        trans_pcie->ref_cmd_in_flight = false;
        IWL_DEBUG_RPM(trans, "clear ref_cmd_in_flight - unref\n");
        iwl_trans_unref(trans);
    }
     */
    
    if (!trans->m_pDevice->cfg->trans.base_params->apmg_wake_up_wa)
        return;
    
    if (WARN_ON(!trans->m_pDevice->holdNICWake))
        return;
    
    trans->m_pDevice->holdNICWake = false;
    trans->clearBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
    //__iwl_trans_pcie_clear_bit(trans, CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
}
