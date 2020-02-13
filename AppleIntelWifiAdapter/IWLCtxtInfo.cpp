//
//  IWLCtxtInfo.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/9.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLCtxtInfo.hpp"

static void *iwl_pcie_ctxt_info_dma_alloc_coherent(IWLTransport *trans, size_t size, dma_addr_t *phys, iwl_dma_ptr **ptr)
{
    iwl_dma_ptr *buf = allocate_dma_buf(size, trans->dma_mask);
    if (!buf) {
        return NULL;
    }
    *ptr = buf;
    *phys = buf->dma;
    return buf->addr;
}

int IWLTransport::iwl_pcie_ctxt_info_init(const struct fw_img *fw)
{
    IWL_INFO(0, "iwl_pcie_ctxt_info_init\n");
    struct iwl_context_info *ctxt_info;
    struct iwl_context_info_rbd_cfg *rx_cfg;
    u32 control_flags = 0, rb_size;
    dma_addr_t phys;
    int ret;
    
    ctxt_info = (struct iwl_context_info *)iwl_pcie_ctxt_info_dma_alloc_coherent(this,
                                                                                 sizeof(*ctxt_info),
                                                                                 &phys, &this->ctxt_info_dma_ptr);
    if (!ctxt_info)
        return -ENOMEM;
    this->ctxt_info_dma_addr = phys;
    
    ctxt_info->version.version = 0;
    ctxt_info->version.mac_id =
    cpu_to_le16((u16)iwlRead32(CSR_HW_REV));
    /* size is in DWs */
    ctxt_info->version.size = cpu_to_le16(sizeof(*ctxt_info) / 4);
    
    switch (this->rx_buf_size) {
        case IWL_AMSDU_2K:
            rb_size = IWL_CTXT_INFO_RB_SIZE_2K;
            break;
        case IWL_AMSDU_4K:
            rb_size = IWL_CTXT_INFO_RB_SIZE_4K;
            break;
        case IWL_AMSDU_8K:
            rb_size = IWL_CTXT_INFO_RB_SIZE_8K;
            break;
        case IWL_AMSDU_12K:
            rb_size = IWL_CTXT_INFO_RB_SIZE_12K;
            break;
        default:
            WARN_ON(1);
            rb_size = IWL_CTXT_INFO_RB_SIZE_4K;
    }
    
    //TODO two function are not known to convert...
    //    WARN_ON(RX_QUEUE_CB_SIZE(m_pDevice->cfg->num_rbds) > 12);
    //    control_flags = IWL_CTXT_INFO_TFD_FORMAT_LONG;
    //    control_flags |=
    //    u32_encode_bits(RX_QUEUE_CB_SIZE(m_pDevice->cfg->num_rbds),
    //                    IWL_CTXT_INFO_RB_CB_SIZE);
    //    control_flags |= u32_encode_bits(rb_size, IWL_CTXT_INFO_RB_SIZE);
    //    ctxt_info->control.control_flags = cpu_to_le32(control_flags);
    
    /* initialize RX default queue */
    rx_cfg = &ctxt_info->rbd_cfg;
    rx_cfg->free_rbd_addr = cpu_to_le64(this->rxq->bd_dma);
    rx_cfg->used_rbd_addr = cpu_to_le64(this->rxq->used_bd_dma);
    rx_cfg->status_wr_ptr = cpu_to_le64(this->rxq->rb_stts_dma);
    
    /* initialize TX command queue */
//    ctxt_info->hcmd_cfg.cmd_queue_addr = cpu_to_le64(this->txq[this->cmd_queue]->dma_addr);
    ctxt_info->hcmd_cfg.cmd_queue_size = 1;
    //    ctxt_info->hcmd_cfg.cmd_queue_size =
    //    TFD_QUEUE_CB_SIZE(IWL_CMD_QUEUE_SIZE);
    
    /* allocate ucode sections in dram and set addresses */
    ret = iwl_pcie_init_fw_sec(fw, &ctxt_info->dram);
    if (ret) {
        free_dma_buf(this->ctxt_info_dma_ptr);
        this->ctxt_info_dma_ptr = NULL;
        this->ctxt_info = NULL;
        this->ctxt_info_dma_addr = NULL;
        return ret;
    }
    
    this->ctxt_info = ctxt_info;
    
    iwl_enable_fw_load_int_ctx_info();
    
    /* kick FW self load */
    iwlWrite64(CSR_CTXT_INFO_BA, this->ctxt_info_dma_addr);
    iwlWritePRPH(UREG_CPU_INIT_RUN, 1);
    
    /* Context info will be released upon alive or failure to get one */
    
    return 0;
}

int IWLTransport::iwl_pcie_ctxt_info_gen3_init(const struct fw_img *fw)
{
    IWL_INFO(0, "iwl_pcie_ctxt_info_gen3_init\n");
    struct iwl_context_info_gen3 *ctxt_info_gen3;
    struct iwl_prph_scratch *prph_scratch;
    struct iwl_prph_scratch_ctrl_cfg *prph_sc_ctrl;
    struct iwl_prph_info *prph_info;
    void *iml_img;
    u32 control_flags = 0;
    int ret;
    int cmdq_size = max_t(u32, IWL_CMD_QUEUE_SIZE,
                          m_pDevice->cfg->min_txq_size);
    /* Allocate prph scratch */
    iwl_dma_ptr *buf = allocate_dma_buf(sizeof(*prph_scratch), this->dma_mask);
    if (!buf) {
        return -ENOMEM;
    }
    this->prph_scratch_dma_ptr = buf;
    prph_scratch = (struct iwl_prph_scratch *)buf->addr;
    this->prph_scratch_dma_addr = buf->dma;
    prph_sc_ctrl = &prph_scratch->ctrl_cfg;
    
    prph_sc_ctrl->version.version = 0;
    prph_sc_ctrl->version.mac_id =
    cpu_to_le16((u16)iwlRead32(CSR_HW_REV));
    prph_sc_ctrl->version.size = cpu_to_le16(sizeof(*prph_scratch) / 4);
    
    control_flags = IWL_PRPH_SCRATCH_RB_SIZE_4K |
    IWL_PRPH_SCRATCH_MTR_MODE |
    (IWL_PRPH_MTR_FORMAT_256B &
     IWL_PRPH_SCRATCH_MTR_FORMAT);
    
    /* initialize RX default queue */
    prph_sc_ctrl->rbd_cfg.free_rbd_addr =
    cpu_to_le64(this->rxq->bd_dma);
    
    //        iwl_pcie_ctxt_info_dbg_enable(trans, &prph_sc_ctrl->hwm_cfg,
    //                          &control_flags);
    prph_sc_ctrl->control.control_flags = cpu_to_le32(control_flags);
    /* allocate ucode sections in dram and set addresses */
    ret = iwl_pcie_init_fw_sec(fw, &prph_scratch->dram);
    if (ret)
        goto err_free_prph_scratch;
    buf = allocate_dma_buf(sizeof(*prph_info), this->dma_mask);
    if (!buf) {
        ret = -ENOMEM;
        goto err_free_prph_scratch;
    }
    this->prph_info_dma_ptr = buf;
    prph_info = (struct iwl_prph_info *)buf->addr;
    prph_info_dma_addr = buf->dma;
    /* Allocate context info */
    buf = allocate_dma_buf(sizeof(*ctxt_info_gen3), this->dma_mask);
    if (!buf) {
        ret = -ENOMEM;
        goto err_free_prph_info;
    }
    this->ctxt_info_dma_ptr = buf;
    ctxt_info_gen3 = (struct iwl_context_info_gen3 *)buf->addr;
    ctxt_info_dma_addr = buf->dma;
    ctxt_info_gen3->prph_info_base_addr =
    cpu_to_le64(this->prph_info_dma_addr);
    ctxt_info_gen3->prph_scratch_base_addr =
    cpu_to_le64(this->prph_scratch_dma_addr);
    ctxt_info_gen3->prph_scratch_size =
    cpu_to_le32(sizeof(*prph_scratch));
    ctxt_info_gen3->cr_head_idx_arr_base_addr =
    cpu_to_le64(this->rxq->rb_stts_dma);
    ctxt_info_gen3->tr_tail_idx_arr_base_addr =
    cpu_to_le64(this->rxq->tr_tail_dma);
    ctxt_info_gen3->cr_tail_idx_arr_base_addr =
    cpu_to_le64(this->rxq->cr_tail_dma);
    ctxt_info_gen3->cr_idx_arr_size =
    cpu_to_le16(IWL_NUM_OF_COMPLETION_RINGS);
    ctxt_info_gen3->tr_idx_arr_size =
    cpu_to_le16(IWL_NUM_OF_TRANSFER_RINGS);
    ctxt_info_gen3->mtr_base_addr =
    cpu_to_le64(this->txq[this->cmd_queue]->dma_addr);
    ctxt_info_gen3->mcr_base_addr =
    cpu_to_le64(this->rxq->used_bd_dma);
    //TODO
    //    ctxt_info_gen3->mtr_size =
    //    cpu_to_le16(TFD_QUEUE_CB_SIZE(cmdq_size));
    //    ctxt_info_gen3->mcr_size =
    //    cpu_to_le16(RX_QUEUE_CB_SIZE(m_pDevice->cfg->num_rbds));
    
    this->ctxt_info_gen3 = ctxt_info_gen3;
    this->prph_info = prph_info;
    this->prph_scratch = prph_scratch;
    /* Allocate IML */
    buf = allocate_dma_buf(m_pDevice->fw.iml_len, this->dma_mask);
    if (!buf) {
        return -ENOMEM;
    }
    this->iml_dma_ptr = buf;
    iml_img = buf->addr;
    this->iml_dma_addr = buf->dma;
    memcpy(iml_img, m_pDevice->fw.iml, m_pDevice->fw.iml_len);
    iwl_enable_fw_load_int_ctx_info();
    /* kick FW self load */
    iwlWrite64(CSR_CTXT_INFO_ADDR,
               this->ctxt_info_dma_addr);
    iwlWrite64(CSR_IML_DATA_ADDR,
               this->iml_dma_addr);
    iwlWrite32(CSR_IML_SIZE_ADDR, m_pDevice->fw.iml_len);
    
    setBit(CSR_CTXT_INFO_BOOT_CTRL,
           CSR_AUTO_FUNC_BOOT_ENA);
    if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_AX210)
        iwlWriteUmacPRPH(UREG_CPU_INIT_RUN, 1);
    else
        setBit(CSR_GP_CNTRL, CSR_AUTO_FUNC_INIT);
    return 0;
err_free_prph_info:
    free_dma_buf(this->prph_info_dma_ptr);
    this->prph_info_dma_ptr = NULL;
err_free_prph_scratch:
    free_dma_buf(this->prph_scratch_dma_ptr);
    this->prph_scratch_dma_ptr = NULL;
    return ret;
}

void IWLTransport::iwl_pcie_ctxt_info_gen3_free()
{
    if (!this->ctxt_info_gen3)
        return;
    free_dma_buf(this->ctxt_info_dma_ptr);
    this->ctxt_info_dma_ptr = NULL;
    this->ctxt_info_dma_addr = 0;
    this->ctxt_info_gen3 = NULL;
    
    iwl_pcie_ctxt_info_free_fw_img();
    
    free_dma_buf(this->prph_scratch_dma_ptr);
    this->prph_scratch_dma_ptr = NULL;
    this->prph_scratch_dma_addr = 0;
    this->prph_scratch = NULL;
    
    free_dma_buf(this->prph_info_dma_ptr);
    this->prph_info_dma_ptr = NULL;
    this->prph_info_dma_addr = 0;
    this->prph_info = NULL;
}

void IWLTransport::iwl_enable_fw_load_int_ctx_info()
{
    IWL_INFO(trans, "Enabling ALIVE interrupt only, msix_enabled=%s\n", this->msix_enabled ? "true" : "false");
    if (!this->msix_enabled) {
        /*
         * When we'll receive the ALIVE interrupt, the ISR will call
         * iwl_enable_fw_load_int_ctx_info again to set the ALIVE
         * interrupt (which is not really needed anymore) but also the
         * RX interrupt which will allow us to receive the ALIVE
         * notification (which is Rx) and continue the flow.
         */
        this->inta_mask =  CSR_INT_BIT_ALIVE | CSR_INT_BIT_FH_RX;
        iwlWrite32(CSR_INT_MASK, this->inta_mask);
    } else {
        enableHWIntrMskMsix(MSIX_HW_INT_CAUSES_REG_ALIVE);
        /*
         * Leave all the FH causes enabled to get the ALIVE
         * notification.
         */
        enableFHIntrMskMsix(this->fh_init_mask);
    }
}

int IWLTransport::iwl_pcie_get_num_sections(const struct fw_img *fw, int start)
{
    int i = 0;
    while (start < fw->num_sec &&
           fw->sec[start].offset != CPU1_CPU2_SEPARATOR_SECTION &&
           fw->sec[start].offset != PAGING_SEPARATOR_SECTION) {
        start++;
        i++;
    }
    return i;
}

static int iwl_pcie_ctxt_info_alloc_dma(IWLTransport *trans,
                                        const struct fw_desc *sec,
                                        struct iwl_dram_data *dram)
{
    dram->block = iwl_pcie_ctxt_info_dma_alloc_coherent(trans, sec->len,
                                                        &dram->physical, &dram->physical_ptr);
    if (!dram->block)
        return -ENOMEM;
    
    dram->size = sec->len;
    memcpy(dram->block, sec->data, sec->len);
    
    return 0;
}

int IWLTransport::iwl_pcie_init_fw_sec(const struct fw_img *fw, struct iwl_context_info_dram *ctxt_dram)
{
    int i, ret, lmac_cnt, umac_cnt, paging_cnt;
    struct iwl_self_init_dram *dram = &this->init_dram;
    if (dram->paging) {
        IWL_ERR(0, "paging shouldn't already be initialized (%d pages)\n",
                dram->paging_cnt);
        iwl_pcie_ctxt_info_free_paging();
    }
    
    lmac_cnt = iwl_pcie_get_num_sections(fw, 0);
    /* add 1 due to separator */
    umac_cnt = iwl_pcie_get_num_sections(fw, lmac_cnt + 1);
    /* add 2 due to separators */
    paging_cnt = iwl_pcie_get_num_sections(fw, lmac_cnt + umac_cnt + 2);
    
    dram->fw = (struct iwl_dram_data *)kcalloc(umac_cnt + lmac_cnt, sizeof(*dram->fw));
    if (!dram->fw)
        return -ENOMEM;
    dram->paging = (struct iwl_dram_data *)kcalloc(paging_cnt, sizeof(*dram->paging));
    if (!dram->paging)
        return -ENOMEM;
    
    /* initialize lmac sections */
    for (i = 0; i < lmac_cnt; i++) {
        ret = iwl_pcie_ctxt_info_alloc_dma(this, &fw->sec[i],
                                           &dram->fw[dram->fw_cnt]);
        
        if (ret)
            return ret;
        ctxt_dram->lmac_img[i] =
        cpu_to_le64(dram->fw[dram->fw_cnt].physical);
        dram->fw_cnt++;
    }
    
    /* initialize umac sections */
    for (i = 0; i < umac_cnt; i++) {
        /* access FW with +1 to make up for lmac separator */
        ret = iwl_pcie_ctxt_info_alloc_dma(this,
                                           &fw->sec[dram->fw_cnt + 1],
                                           &dram->fw[dram->fw_cnt]);
        if (ret)
            return ret;
        ctxt_dram->umac_img[i] =
        cpu_to_le64(dram->fw[dram->fw_cnt].physical);
        dram->fw_cnt++;
    }
    
    /*
     * Initialize paging.
     * Paging memory isn't stored in dram->fw as the umac and lmac - it is
     * stored separately.
     * This is since the timing of its release is different -
     * while fw memory can be released on alive, the paging memory can be
     * freed only when the device goes down.
     * Given that, the logic here in accessing the fw image is a bit
     * different - fw_cnt isn't changing so loop counter is added to it.
     */
    for (i = 0; i < paging_cnt; i++) {
        /* access FW with +2 to make up for lmac & umac separators */
        int fw_idx = dram->fw_cnt + i + 2;
        
        ret = iwl_pcie_ctxt_info_alloc_dma(this, &fw->sec[fw_idx],
                                           &dram->paging[i]);
        if (ret)
            return ret;
        
        ctxt_dram->virtual_img[i] =
        cpu_to_le64(dram->paging[i].physical);
        dram->paging_cnt++;
    }
    
    return 0;
}

void IWLTransport::iwl_pcie_ctxt_info_free()
{
    struct iwl_self_init_dram *dram = &this->init_dram;
    int i;
    
    if (!dram->paging) {
        WARN_ON(dram->paging_cnt);
        return;
    }
    
    /* free paging*/
    for (i = 0; i < dram->paging_cnt; i++) {
        free_dma_buf(dram->paging[i].physical_ptr);
    }
    //    IOFree(dram->paging, sizeof(*dram->paging));
    dram->paging_cnt = 0;
    dram->paging = NULL;
}

void IWLTransport::iwl_pcie_ctxt_info_free_paging()
{
    struct iwl_self_init_dram *dram = &this->init_dram;
    int i;
    
    if (!dram->paging) {
        WARN_ON(dram->paging_cnt);
        return;
    }
    
    /* free paging*/
    for (i = 0; i < dram->paging_cnt; i++) {
        free_dma_buf(dram->paging[i].physical_ptr);
    }
    
    //    IOFree(dram->paging, sizeof(*dram->paging));
    dram->paging_cnt = 0;
    dram->paging = NULL;
}

void IWLTransport::iwl_pcie_ctxt_info_free_fw_img()
{
    struct iwl_self_init_dram *dram = &this->init_dram;
    int i;
    
    if (!dram->fw) {
        WARN_ON(dram->fw_cnt);
        return;
    }
    for (i = 0; i < dram->fw_cnt; i++) {
        free_dma_buf(dram->fw[i].physical_ptr);
    }
    //    IOFree(dram->fw, sizeof(*dram->fw));
    dram->fw_cnt = 0;
    dram->fw = NULL;
}
