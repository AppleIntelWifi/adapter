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
    
    //TODO wait for interrupt to refresh this state
    uint64_t begin_time = 0;
    uint64_t cur_time = 0;
    clock_get_uptime(&begin_time);
    while (!this->ucode_write_complete) {
        IOSleep(10);
        clock_get_uptime(&cur_time);
        if (cur_time - begin_time > 5000) {
            ret = -1;
            break;
        }
    }
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
        DebugLog("Writing [%d] with offset %d", section_num, offset);
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
    
#ifdef CPTCFG_IWLWIFI_DEVICE_TESTMODE
    iwl_dnt_configure(trans, image);
#endif
    
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
    if (ret)
        return ret;
    
    /* load to FW the binary sections of CPU2 */
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

