//
//  IWLTransportTx.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"
#include "IWLSCD.h"

extern const void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));

#define HOST_COMPLETE_TIMEOUT    (2 * HZ * CPTCFG_IWL_TIMEOUT_FACTOR)

/* Comparator for struct iwl_hcmd_names.
 * Used in the binary search over a list of host commands.
 *
 * @key: command_id that we're looking for.
 * @elt: struct iwl_hcmd_names candidate for match.
 *
 * @return 0 iff equal.
 */


static int iwl_queue_space(const struct iwl_txq *q)
{
    unsigned int max;
    unsigned int used;
    
    /*
     * To avoid ambiguity between empty and completely full queues, there
     * should always be less than TFD_QUEUE_SIZE_MAX elements in the queue.
     * If q->n_window is smaller than TFD_QUEUE_SIZE_MAX, there is no need
     * to reserve any queue entries for this purpose.
     */
    if (q->n_window < TFD_QUEUE_SIZE_MAX)
        max = q->n_window;
    else
        max = TFD_QUEUE_SIZE_MAX - 1;
    
    /*
     * TFD_QUEUE_SIZE_MAX is a power of 2, so the following is equivalent to
     * modulo by TFD_QUEUE_SIZE_MAX and is well defined.
     */
    used = (q->write_ptr - q->read_ptr) & (TFD_QUEUE_SIZE_MAX - 1);
    
    if (WARN_ON(used > max))
        return 0;
    
    return max - used;
}

/*
 * iwl_queue_init - Initialize queue's high/low-water and read/write indexes
 */
static int iwl_queue_init(struct iwl_txq *q, int slots_num)
{
    q->n_window = slots_num;
    
    /* slots_num must be power-of-two size, otherwise
     * iwl_pcie_get_cmd_index is broken. */
    if (WARN_ON(!is_power_of_2(slots_num)))
        return -EINVAL;
    
    q->low_mark = q->n_window / 4;
    if (q->low_mark < 4)
        q->low_mark = 4;
    
    q->high_mark = q->n_window / 8;
    if (q->high_mark < 2)
        q->high_mark = 2;
    
    q->write_ptr = 0;
    q->read_ptr = 0;
    
    return 0;
}

int iwl_pcie_alloc_dma_ptr(IWLTransport *trans_pcie, struct iwl_dma_ptr **ptr, size_t size)
{
    if (*ptr && (*ptr)->addr) {
        return -EINVAL;
    }
    
    *ptr = allocate_dma_buf(size, trans_pcie->dma_mask);
    if (!(*ptr)) {
        return -ENOMEM;
    }
    return 0;
}

void iwl_pcie_free_dma_ptr(IWLTransport *trans, struct iwl_dma_ptr *ptr)
{
    if (unlikely(!ptr->addr))
        return;
    
    free_dma_buf(ptr);
    //memset(ptr, 0, sizeof(*ptr));
}



static int iwl_pcie_txq_alloc(IWLTransport *trans, struct iwl_txq *txq, int slots_num, bool cmd_queue)
{
    IWLTransport *trans_pcie = trans;
    size_t tfd_sz = trans_pcie->tfd_size * TFD_QUEUE_SIZE_MAX;
    size_t tb0_buf_sz;
    int i;
    int ret;
    struct iwl_dma_ptr *tfds_dma = NULL;
    struct iwl_dma_ptr *first_tb_bufs_dma = NULL;
    
    if (WARN_ON(txq->entries || txq->tfds))
        return -EINVAL;
    
    // TODO: Implement
    //    setup_timer(&txq->stuck_timer, iwl_pcie_txq_stuck_timer, (unsigned long)txq);
    //txq->trans_pcie = trans->m_pDevice->tr;
    
    txq->n_window = slots_num;
    
    txq->entries = (struct iwl_pcie_txq_entry *) iwh_zalloc(sizeof(struct iwl_pcie_txq_entry) * slots_num);
    
    if (!txq->entries)
        goto error;
    
    if (cmd_queue)
        for (i = 0; i < slots_num; i++) {
            txq->entries[i].cmd = (struct iwl_device_cmd *)iwh_zalloc(sizeof(struct iwl_device_cmd));
            if (!txq->entries[i].cmd)
                goto error;
        }
    
    /* Circular buffer of transmit frame descriptors (TFDs),
     * shared with device */
    ret = iwl_pcie_alloc_dma_ptr(trans, &tfds_dma, tfd_sz);
    if (ret) {
        goto error;
    }
    
    
    txq->tfds_dma = tfds_dma;
    txq->tfds = tfds_dma->addr;
    txq->dma_addr = tfds_dma->dma;
    
    tb0_buf_sz = sizeof(*txq->first_tb_bufs) * slots_num;
    
    ret = iwl_pcie_alloc_dma_ptr(trans, &first_tb_bufs_dma, tb0_buf_sz);
    if (ret) {
        goto err_free_tfds;
    }
    
    txq->first_tb_dma_ptr = first_tb_bufs_dma;
    txq->first_tb_bufs = (struct iwl_pcie_first_tb_buf *)first_tb_bufs_dma->addr;
    txq->first_tb_dma = first_tb_bufs_dma->dma;
    
    return 0;
err_free_tfds:
    free_dma_buf(txq->tfds_dma);
error:
    if (txq->entries && cmd_queue)
        for (i = 0; i < slots_num; i++)
            iwh_free(txq->entries[i].cmd);
    //kfree(txq->entries[i].cmd);
    
    iwh_free(txq->entries);
    //OFree(txq->entries, sizeof(iwl_pcie_txq_entry) * slots_num);
    txq->entries = NULL;
    return -ENOMEM;
    
}

#define TFD_TX_CMD_SLOTS 256
#define TFD_CMD_SLOTS 32

int iwl_pcie_txq_init(IWLTransport *trans, struct iwl_txq *txq, int slots_num, bool cmd_queue)
{
    int ret;
    u32 tfd_queue_max_size =
    trans->m_pDevice->cfg->trans.base_params->max_tfd_queue_size;
    
    txq->need_update = false;
    
    /* max_tfd_queue_size must be power-of-two size, otherwise
     * iwl_queue_inc_wrap and iwl_queue_dec_wrap are broken. */
    if (tfd_queue_max_size & (tfd_queue_max_size - 1)) {
        IWL_ERR(0, "Max tfd queue size must be a power of two, but is %d",
                tfd_queue_max_size);
        return -EINVAL;
    }
    
    /* Initialize queue's high/low-water marks, and head/tail indexes */
    ret = iwl_queue_init(txq, slots_num);
    if (ret)
        return ret;
    
    txq->lock = IOSimpleLockAlloc();
    
    if (cmd_queue) {
        // TODO: Implement
        //        static struct lock_class_key iwl_pcie_cmd_queue_lock_class;
        //
        //        lockdep_set_class(&txq->lock, &iwl_pcie_cmd_queue_lock_class);
    }
    
    // TODO: Implement
    //__skb_queue_head_init(&txq->overflow_q);
    
    return 0;
}

void iwl_pcie_tfd_unmap(IWLTransport *trans, struct iwl_cmd_meta *meta, struct iwl_txq *txq, int index)
{
    //struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int i, num_tbs;
    void *tfd = iwl_pcie_get_tfd(trans, txq, index);
    
    /* Sanity check on number of chunks */
    num_tbs = iwl_pcie_tfd_get_num_tbs(trans, tfd);
    
    if (num_tbs >= trans->max_tbs) {
        IWL_ERR(trans, "Too many chunks: %i\n", num_tbs);
        /* @todo issue fatal error, it is quite serious situation */
        return;
    }
    
    // Since working with DMA is quite different in OSX, unmapping is basically just freeing of buffer descriptors
    // that were previously allocated
    
    for (i = 0; i < ARRAY_SIZE(meta->dma); ++i) {
        if (meta->dma[i]) {
            free_dma_buf(meta->dma[i]);
        }
        meta->dma[i] = NULL;
    }
    
    /* first TB is never freed - it's the bidirectional DMA data */
    //    for (i = 1; i < num_tbs; i++) {
    //        if (meta->tbs & BIT(i))
    //            dma_unmap_page(trans->dev,
    //                           iwl_pcie_tfd_tb_get_addr(trans, tfd, i),
    //                           iwl_pcie_tfd_tb_get_len(trans, tfd, i),
    //                           DMA_TO_DEVICE);
    //        else
    //            dma_unmap_single(trans->dev,
    //                             iwl_pcie_tfd_tb_get_addr(trans, tfd, i),
    //                             iwl_pcie_tfd_tb_get_len(trans, tfd, i),
    //                             DMA_TO_DEVICE);
    //    }
    
    if (trans->m_pDevice->cfg->trans.use_tfh) {
        struct iwl_tfh_tfd *tfd_fh = (struct iwl_tfh_tfd *)tfd;
        
        tfd_fh->num_tbs = 0;
    } else {
        struct iwl_tfd *tfd_fh = (struct iwl_tfd *)tfd;
        
        tfd_fh->num_tbs = 0;
    }
    
}

void iwl_pcie_txq_free_tfd(IWLTransport *trans, struct iwl_txq *txq)
{
    /* rd_ptr is bounded by TFD_QUEUE_SIZE_MAX and
     * idx is bounded by n_window
     */
    int rd_ptr = txq->read_ptr;
    int idx = iwl_pcie_get_cmd_index(txq, rd_ptr);
    
    //lockdep_assert_held(&txq->lock);
    
    /* We have only q->n_window txq->entries, but we use
     * TFD_QUEUE_SIZE_MAX tfds
     */
    iwl_pcie_tfd_unmap(trans, &txq->entries[idx].meta, txq, rd_ptr);
    
    /* free SKB */
    if (txq->entries) {
        sk_buff *skb;
        
        skb = (sk_buff*)txq->entries[idx].skb;
        
        /* Can be called from irqs-disabled context
         * If skb is not NULL, it means that the whole queue is being
         * freed and that the queue is not empty - free the skb
         */
        if (skb) {
            // TODO: Implement
            // iwl_op_mode_free_skb(trans->op_mode, skb);
            txq->entries[idx].skb = NULL;
        }
    }
}


static void iwl_pcie_txq_unmap(IWLTransport *trans, int txq_id)
{
    struct IWLTransport *trans_pcie = trans;
    struct iwl_txq *txq = trans_pcie->txq[txq_id];
    
    //spin_lock_bh(&txq->lock);
    while (txq->write_ptr != txq->read_ptr) {
        //IWL_DEBUG_TX_REPLY(trans, "Q %d Free %d\n", txq_id, txq->read_ptr);
        
        if (txq_id != trans_pcie->cmd_queue) {
            //struct sk_buff *skb = txq->entries[txq->read_ptr].skb;
            
            //if (WARN_ON_ONCE(!skb))
            //    continue;
            
            // TODO: Implement
            // iwl_pcie_free_tso_page(trans_pcie, skb);
        }
        iwl_pcie_txq_free_tfd(trans, txq);
        txq->read_ptr = iwl_queue_inc_wrap(txq->read_ptr);
        
        if (txq->read_ptr == txq->write_ptr) {
            // unsigned long flags;
            
            //spin_lock_irqsave(&trans_pcie->reg_lock, flags);
            if (txq_id != trans_pcie->cmd_queue) {
                //IWL_DEBUG_RPM(trans, "Q %d - last tx freed\n", txq->id);
                //trans->m_pDevice->ref
                //iwl_trans_unref(trans);
            } else {
                iwl_pcie_clear_cmd_in_flight(trans);
            }
            //spin_unlock_irqrestore(&trans_pcie->reg_lock, flags);
        }
    }
    
    // TODO: Implement
    //    while (!skb_queue_empty(&txq->overflow_q)) {
    //        struct sk_buff *skb = __skb_dequeue(&txq->overflow_q);
    //
    //        iwl_op_mode_free_skb(trans->op_mode, skb);
    //    }
    
    //spin_unlock_bh(&txq->lock);
    
    /* just in case - this queue may have been stopped */
    // TODO: Implement
    // iwl_wake_queue(trans, txq);
}

static void iwl_pcie_txq_free(IWLTransport *trans, int txq_id)
{
    //struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    struct iwl_txq *txq = trans->txq[txq_id];
    //struct device *dev = trans->dev;
    int i;
    
    if (WARN_ON(!txq))
        return;
    
    iwl_pcie_txq_unmap(trans, txq_id);
    
    /* De-alloc array of command/tx buffers */
    if (txq_id == trans->cmd_queue)
        for (i = 0; i < txq->n_window; i++) {
            iwh_free(txq->entries[i].cmd);
            iwh_free((void *)txq->entries[i].free_buf);
        }
    
    /* De-alloc circular buffer of TFDs */
    if (txq->tfds) {
        
        free_dma_buf(txq->tfds_dma);
        txq->dma_addr = 0;
        txq->tfds = NULL;
        
        free_dma_buf(txq->first_tb_dma_ptr);
    }
    
    iwh_free(txq->entries);
    txq->entries = NULL;
    
    //del_timer_sync(&txq->stuck_timer);
    
    /* 0-fill queue descriptor structure */
    bzero(txq, sizeof(*txq));
}

void iwl_pcie_tx_free(IWLTransport *trans_pcie)
{
    int txq_id;
    
    memset(trans_pcie->queue_used, 0, sizeof(trans_pcie->queue_used));
    
    /* Tx queues */
    if (trans_pcie->txq_memory) {
        for (txq_id = 0; txq_id < trans_pcie->m_pDevice->cfg->trans.base_params->num_of_queues; txq_id++) {
            iwl_pcie_txq_free(trans_pcie, txq_id);
            trans_pcie->txq[txq_id] = NULL;
        }
    }
    
    iwh_free(trans_pcie->txq_memory);
    trans_pcie->txq_memory = NULL;
    
    iwl_pcie_free_dma_ptr(trans_pcie, trans_pcie->kw);
    iwl_pcie_free_dma_ptr(trans_pcie, trans_pcie->scd_bc_tbls);
}

int iwl_pcie_tx_alloc(IWLTransport *trans_pcie)
{
    int ret;
    int txq_id, slots_num;
    // struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    IWLTransport* trans = trans_pcie;
    //tans->m_pDevice->cfg->trans.base_params->num_of_queues
    u16 bc_tbls_size = trans_pcie->m_pDevice->cfg->trans.base_params->num_of_queues;
    bc_tbls_size *= (trans->m_pDevice->cfg->trans.device_family >=
                     IWL_DEVICE_FAMILY_AX210) ?
    sizeof(struct iwl_gen3_bc_tbl) :
    sizeof(struct iwlagn_scd_bc_tbl);
    
    /*It is not allowed to alloc twice, so warn when this happens.
     * We cannot rely on the previous allocation, so free and fail */
    if (WARN_ON(trans_pcie->txq_memory)) {
        IWL_ERR(trans, "txq_memory should NOT exist\n");
        ret = -EINVAL;
        goto error;
    }
    trans_pcie->scd_bc_tbls = 0;
    
    ret = iwl_pcie_alloc_dma_ptr(trans, &trans_pcie->scd_bc_tbls, bc_tbls_size);
    if (ret) {
        IWL_ERR(trans, "Scheduler BC Table allocation failed\n");
        goto error;
    }
    
    trans_pcie->kw = 0;
    /* Alloc keep-warm buffer */
    ret = iwl_pcie_alloc_dma_ptr(trans, &trans_pcie->kw, IWL_KW_SIZE);
    if (ret) {
        IWL_ERR(trans, "Keep Warm allocation failed\n");
        goto error;
    }
    
    trans_pcie->txq_memory = (struct iwl_txq *)iwh_zalloc(sizeof(struct iwl_txq) * trans->m_pDevice->cfg->trans.base_params->num_of_queues);
    if (!trans_pcie->txq_memory) {
        IWL_ERR(trans, "Not enough memory for txq\n");
        ret = -ENOMEM;
        goto error;
    }
    
    /* Alloc and init all Tx queues, including the command queue (#4/#9) */
    for (txq_id = 0; txq_id < trans->m_pDevice->cfg->trans.base_params->num_of_queues; txq_id++) {
        bool cmd_queue = (txq_id == trans_pcie->cmd_queue);
        
        slots_num = cmd_queue ? max_t(u32, IWL_CMD_QUEUE_SIZE,
        trans->m_pDevice->cfg->min_txq_size) : max_t(u32, IWL_DEFAULT_QUEUE_SIZE,
        trans->m_pDevice->cfg->min_256_ba_txq_size);
        trans_pcie->txq[txq_id] = &trans_pcie->txq_memory[txq_id];
        ret = iwl_pcie_txq_alloc(trans, trans_pcie->txq[txq_id], slots_num, cmd_queue);
        if (ret) {
            IWL_ERR(trans, "Tx %d queue alloc failed\n", txq_id);
            goto error;
        }
        trans_pcie->txq[txq_id]->id = txq_id;
    }
    
    return 0;
    
error:
    iwl_pcie_tx_free(trans);
    
    return ret;
}

int IWLTransport::txInit() {
    int ret;
    int txq_id, slots_num;
    bool alloc = false;
    if(!this->txq_memory) {
        ret = iwl_pcie_tx_alloc(this);
        if(ret) {
            goto error;
        }
        alloc = true;
    }
    
    iwlWritePRPH(SCD_TXFACT, 0);
    iwlWriteDirect32(FH_KW_MEM_ADDR_REG, (u32)this->kw->dma >> 4);
    
    for (txq_id = 0; txq_id < m_pDevice->cfg->trans.base_params->num_of_queues; txq_id++) {
        bool cmd_queue = (txq_id == this->cmd_queue);
        
        slots_num = cmd_queue ? max_t(u32, IWL_CMD_QUEUE_SIZE,
        m_pDevice->cfg->min_txq_size) : max_t(u32, IWL_DEFAULT_QUEUE_SIZE,
        m_pDevice->cfg->min_256_ba_txq_size);
        ret = iwl_pcie_txq_init(this, txq[txq_id], slots_num, cmd_queue);
        if (ret) {
            IWL_ERR(trans, "Tx %d queue init failed\n", txq_id);
            goto error;
        }
        
        /*
         * Tell nic where to find circular buffer of TFDs for a
         * given Tx queue, and enable the DMA channel used for that
         * queue.
         * Circular buffer (TFD queue in DRAM) physical base address
         */
        iwlWriteDirect32(FH_MEM_CBBC_QUEUE(this->m_pDevice, txq_id),
                         (u32)txq[txq_id]->dma_addr >> 8);
    }
    iwlSetBitsPRPH(SCD_GP_CTRL, SCD_GP_CTRL_AUTO_ACTIVE_MODE);
    //iwl_set_bits_prph(trans, SCD_GP_CTRL, SCD_GP_CTRL_AUTO_ACTIVE_MODE);
    if (m_pDevice->cfg->trans.base_params->num_of_queues > 20)
        iwlSetBitsPRPH(SCD_GP_CTRL, SCD_GP_CTRL_ENABLE_31_QUEUES);
    
    return 0;
error:
    IWL_ERR(0, "Could not init tx ring\n");
    if (alloc) {
        iwl_pcie_tx_free(this);
    }
    return -EIO;
}

static int iwl_hcmd_names_cmp(const void *key, const void *elt)
{
    const struct iwl_hcmd_names *name = (const struct iwl_hcmd_names *)elt;
    u8 cmd1 = *(u8 *)key;
    u8 cmd2 = name->cmd_id;
    
    return (cmd1 - cmd2);
}

const char *iwl_get_cmd_string(IWLTransport *trans, u32 id)
{
    u8 grp, cmd;
    struct iwl_hcmd_names *ret;
    const struct iwl_hcmd_arr *arr;
    size_t size = sizeof(struct iwl_hcmd_names);
    
    grp = iwl_cmd_groupid(id);
    cmd = iwl_cmd_opcode(id);
    
    if (!trans->command_groups || grp >= trans->command_groups_size ||
        !trans->command_groups[grp].arr)
        return "UNKNOWN";
    
    arr = &trans->command_groups[grp];
    ret = (struct iwl_hcmd_names *)bsearch((const void *)&cmd, arr->arr, arr->size, size, iwl_hcmd_names_cmp);
    if (!ret)
        return "UNKNOWN";
    return ret->cmd_name;
}

int iwl_cmd_groups_verify_sorted(IWLTransport *trans)
{
    int i, j;
    const struct iwl_hcmd_arr *arr;
    
    for (i = 0; i < trans->command_groups_size; i++) {
        arr = &trans->command_groups[i];
        if (!arr->arr)
            continue;
        for (j = 0; j < arr->size - 1; j++)
            if (arr->arr[j].cmd_id > arr->arr[j + 1].cmd_id)
                return -1;
    }
    return 0;
}

static inline u16 iwl_pcie_tfd_tb_get_len(IWLTransport *trans, void *_tfd,
                                          u8 idx)
{
    if (trans->m_pDevice->cfg->trans.use_tfh) {
        struct iwl_tfh_tfd *tfd = (struct iwl_tfh_tfd *)_tfd;
        struct iwl_tfh_tb *tb = &tfd->tbs[idx];
        
        return le16_to_cpu(tb->tb_len);
    } else {
        struct iwl_tfd *tfd = (struct iwl_tfd *)_tfd;
        struct iwl_tfd_tb *tb = &tfd->tbs[idx];
        
        return le16_to_cpu(tb->hi_n_len) >> 4;
    }
}

static void iwl_pcie_tfd_set_tb(IWLTransport *trans_pcie, void *tfd, u8 idx, dma_addr_t addr, u16 len)
{
    struct iwl_tfd *tfd_fh = (struct iwl_tfd *)tfd;
    struct iwl_tfd_tb *tb = &tfd_fh->tbs[idx];
    
    u16 hi_n_len = len << 4;
    
    put_unaligned_le32((u32)addr, &tb->lo);
    hi_n_len |= iwl_get_dma_hi_addr(addr);
    
    tb->hi_n_len = cpu_to_le16(hi_n_len);
    
    tfd_fh->num_tbs = idx + 1;
}

static int iwl_pcie_txq_build_tfd(IWLTransport *trans_pcie, struct iwl_txq *txq, dma_addr_t addr, u16 len, bool reset)
{
    void *tfd;
    u32 num_tbs;
    
    tfd = (u8*)txq->tfds + trans_pcie->tfd_size * txq->write_ptr;
    
    if (reset)
        memset(tfd, 0, trans_pcie->tfd_size);
    
    num_tbs = iwl_pcie_tfd_get_num_tbs(trans_pcie, tfd);
    
    /* Each TFD can point to a maximum max_tbs Tx buffers */
    if (num_tbs >= trans_pcie->max_tbs) {
        IWL_ERR(trans, "Error can not send more than %d chunks\n", trans_pcie->max_tbs);
        return -EINVAL;
    }
    
    if (addr & ~IWL_TX_DMA_MASK)
        return -EINVAL;
    
    iwl_pcie_tfd_set_tb(trans_pcie, tfd, num_tbs, addr, len);
    
    return num_tbs;
}

static int iwl_pcie_set_cmd_in_flight(IWLTransport *trans,
                                      const struct iwl_host_cmd *cmd)
{
    int ret;
    
    /* Make sure the NIC is still alive in the bus */
    if (test_bit(STATUS_TRANS_DEAD, &trans->status))
        return -ENODEV;
    
    /*
     * wake up the NIC to make sure that the firmware will see the host
     * command - we will let the NIC sleep once all the host commands
     * returned. This needs to be done only on NICs that have
     * apmg_wake_up_wa set.
     */
    if (trans->m_pDevice->cfg->trans.base_params->apmg_wake_up_wa &&
        !trans->m_pDevice->holdNICWake) {
        trans->setBit(CSR_GP_CNTRL,
                      CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
        
        ret = trans->iwlPollBit(CSR_GP_CNTRL,
                                CSR_GP_CNTRL_REG_VAL_MAC_ACCESS_EN,
                                (CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY |
                                 CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP),
                                15000);
        if (ret < 0) {
            trans->clearBit(CSR_GP_CNTRL,
                            CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
            IWL_ERR(trans, "Failed to wake NIC for hcmd\n");
            return -EIO;
        }
        trans->m_pDevice->holdNICWake = true;
    }
    
    return 0;
}

/*
 * iwl_pcie_txq_inc_wr_ptr - Send new write index to hardware
 */
static void iwl_pcie_txq_inc_wr_ptr(IWLTransport *trans,
                                    struct iwl_txq *txq)
{
    u32 reg = 0;
    int txq_id = txq->id;
    
    /*
     * explicitly wake up the NIC if:
     * 1. shadow registers aren't enabled
     * 2. NIC is woken up for CMD regardless of shadow outside this function
     * 3. there is a chance that the NIC is asleep
     */
    if (!trans->m_pDevice->cfg->trans.base_params->shadow_reg_enable &&
        txq_id != trans->cmd_queue &&
        test_bit(STATUS_TPOWER_PMI, &trans->status)) {
        /*
         * wake up nic if it's powered down ...
         * uCode will wake up, and interrupt us again, so next
         * time we'll skip this part.
         */
        reg = trans->iwlRead32(CSR_UCODE_DRV_GP1);
        
        if (reg & CSR_UCODE_DRV_GP1_BIT_MAC_SLEEP) {
            IWL_INFO(trans, "Tx queue %d requesting wakeup, GP1 = 0x%x\n",
                     txq_id, reg);
            trans->setBit(CSR_GP_CNTRL,
                          CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
            txq->need_update = true;
            return;
        }
    }
    
    /*
     * if not in power-save mode, uCode will never sleep when we're
     * trying to tx (during RFKILL, we're not trying to tx).
     */
    IWL_INFO(trans, "Q:%d WR: 0x%x\n", txq_id, txq->write_ptr);
    if (!txq->block)
        trans->iwlWrite32(HBUS_TARG_WRPTR,
                          txq->write_ptr | (txq_id << 8));
}

/*************** HOST COMMAND QUEUE FUNCTIONS   *****/

/*
 * iwl_pcie_enqueue_hcmd - enqueue a uCode command
 * @priv: device private data point
 * @cmd: a pointer to the ucode command structure
 *
 * The function returns < 0 values to indicate the operation
 * failed. On success, it returns the index (>= 0) of command in the
 * command queue.
 */
static int iwl_pcie_enqueue_hcmd(IWLTransport *trans,
                                 struct iwl_host_cmd *cmd)
{
    struct iwl_txq *txq = trans->txq[trans->cmd_queue];
    struct iwl_device_cmd *out_cmd;
    struct iwl_cmd_meta *out_meta;
    IOInterruptState flags;
    void *dup_buf = NULL;
    int idx;
    u16 copy_size, cmd_size, tb0_size;
    bool had_nocopy = false;
    u8 group_id = iwl_cmd_groupid(cmd->id);
    int i, ret;
    u32 cmd_pos;
    const u8 *cmddata[IWL_MAX_CMD_TBS_PER_TFD];
    u16 cmdlen[IWL_MAX_CMD_TBS_PER_TFD];
    
    if (!trans->wide_cmd_header && group_id > IWL_ALWAYS_LONG_GROUP) {
        IWL_INFO(0, "unsupported wide command %#x\n", cmd->id);
        return -EINVAL;
    }
    
    if (group_id != 0) {
        copy_size = sizeof(struct iwl_cmd_header_wide);
        cmd_size = sizeof(struct iwl_cmd_header_wide);
    } else {
        copy_size = sizeof(struct iwl_cmd_header);
        cmd_size = sizeof(struct iwl_cmd_header);
    }
    
    /* need one for the header if the first is NOCOPY */
    BUILD_BUG_ON(IWL_MAX_CMD_TBS_PER_TFD > IWL_NUM_OF_TBS - 1);
    
    for (i = 0; i < IWL_MAX_CMD_TBS_PER_TFD; i++) {
        cmddata[i] = (const u8 *)cmd->data[i];
        cmdlen[i] = cmd->len[i];
        
        if (!cmd->len[i])
            continue;
        
        /* need at least IWL_FIRST_TB_SIZE copied */
        if (copy_size < IWL_FIRST_TB_SIZE) {
            int copy = IWL_FIRST_TB_SIZE - copy_size;
            
            if (copy > cmdlen[i])
                copy = cmdlen[i];
            cmdlen[i] -= copy;
            cmddata[i] += copy;
            copy_size += copy;
        }
        
        if (cmd->dataflags[i] & IWL_HCMD_DFL_NOCOPY) {
            had_nocopy = true;
            if (WARN_ON(cmd->dataflags[i] & IWL_HCMD_DFL_DUP)) {
                idx = -EINVAL;
                goto free_dup_buf;
            }
        } else if (cmd->dataflags[i] & IWL_HCMD_DFL_DUP) {
            /*
             * This is also a chunk that isn't copied
             * to the static buffer so set had_nocopy.
             */
            had_nocopy = true;
            
            /* only allowed once */
            if (WARN_ON(dup_buf)) {
                idx = -EINVAL;
                goto free_dup_buf;
            }
            
            dup_buf = iwh_malloc(cmdlen[i]);
            if (!dup_buf)
                return -ENOMEM;
            memcpy(dup_buf, cmddata[i], cmdlen[i]);
        } else {
            /* NOCOPY must not be followed by normal! */
            if (WARN_ON(had_nocopy)) {
                idx = -EINVAL;
                goto free_dup_buf;
            }
            copy_size += cmdlen[i];
        }
        cmd_size += cmd->len[i];
    }
    
    /*
     * If any of the command structures end up being larger than
     * the TFD_MAX_PAYLOAD_SIZE and they aren't dynamically
     * allocated into separate TFDs, then we will need to
     * increase the size of the buffers.
     */
    if (copy_size > TFD_MAX_PAYLOAD_SIZE) {
        IWL_INFO(0, "Command %s (%#x) is too large (%d bytes)\n",
                 iwl_get_cmd_string(trans, cmd->id),
                 cmd->id, copy_size);
        idx = -EINVAL;
        goto free_dup_buf;
    }
    
    //    spin_lock_bh(&txq->lock);
    
    if (iwl_queue_space(txq) < ((cmd->flags & CMD_ASYNC) ? 2 : 1)) {
        //        spin_unlock_bh(&txq->lock);
        
        IWL_ERR(trans, "No space in command queue\n");
        //TODO
        //        iwl_op_mode_cmd_queue_full(trans->op_mode);
        idx = -ENOSPC;
        goto free_dup_buf;
    }
    
    idx = iwl_pcie_get_cmd_index(txq, txq->write_ptr);
    out_cmd = (struct iwl_device_cmd *)txq->entries[idx].cmd;
    out_meta = &txq->entries[idx].meta;
    
    memset(out_meta, 0, sizeof(*out_meta));    /* re-initialize to NULL */
    if (cmd->flags & CMD_WANT_SKB)
        out_meta->source = cmd;
    
    /* set up the header */
    if (group_id != 0) {
        out_cmd->hdr_wide.cmd = iwl_cmd_opcode(cmd->id);
        out_cmd->hdr_wide.group_id = group_id;
        out_cmd->hdr_wide.version = iwl_cmd_version(cmd->id);
        out_cmd->hdr_wide.length =
        cpu_to_le16(cmd_size -
                    sizeof(struct iwl_cmd_header_wide));
        out_cmd->hdr_wide.reserved = 0;
        out_cmd->hdr_wide.sequence =
        cpu_to_le16(QUEUE_TO_SEQ(trans->cmd_queue) |
                    INDEX_TO_SEQ(txq->write_ptr));
        
        cmd_pos = sizeof(struct iwl_cmd_header_wide);
        copy_size = sizeof(struct iwl_cmd_header_wide);
    } else {
        out_cmd->hdr.cmd = iwl_cmd_opcode(cmd->id);
        out_cmd->hdr.sequence =
        cpu_to_le16(QUEUE_TO_SEQ(trans->cmd_queue) |
                    INDEX_TO_SEQ(txq->write_ptr));
        out_cmd->hdr.group_id = 0;
        
        cmd_pos = sizeof(struct iwl_cmd_header);
        copy_size = sizeof(struct iwl_cmd_header);
    }
    
    /* and copy the data that needs to be copied */
    for (i = 0; i < IWL_MAX_CMD_TBS_PER_TFD; i++) {
        int copy;
        
        if (!cmd->len[i])
            continue;
        
        /* copy everything if not nocopy/dup */
        if (!(cmd->dataflags[i] & (IWL_HCMD_DFL_NOCOPY |
                                   IWL_HCMD_DFL_DUP))) {
            copy = cmd->len[i];
            
            memcpy((u8 *)out_cmd + cmd_pos, cmd->data[i], copy);
            cmd_pos += copy;
            copy_size += copy;
            continue;
        }
        
        /*
         * Otherwise we need at least IWL_FIRST_TB_SIZE copied
         * in total (for bi-directional DMA), but copy up to what
         * we can fit into the payload for debug dump purposes.
         */
        copy = min_t(int, TFD_MAX_PAYLOAD_SIZE - cmd_pos, cmd->len[i]);
        
        memcpy((u8 *)out_cmd + cmd_pos, cmd->data[i], copy);
        cmd_pos += copy;
        
        /* However, treat copy_size the proper way, we need it below */
        if (copy_size < IWL_FIRST_TB_SIZE) {
            copy = IWL_FIRST_TB_SIZE - copy_size;
            
            if (copy > cmd->len[i])
                copy = cmd->len[i];
            copy_size += copy;
        }
    }
    
    IWL_INFO(trans,
             "Sending command %s (%.2x.%.2x), seq: 0x%04X, %d bytes at %d[%d]:%d\n",
             iwl_get_cmd_string(trans, cmd->id),
             group_id, out_cmd->hdr.cmd,
             le16_to_cpu(out_cmd->hdr.sequence),
             cmd_size, txq->write_ptr, idx, trans->cmd_queue);
    
    /* start the TFD with the minimum copy bytes */
    tb0_size = min_t(int, copy_size, IWL_FIRST_TB_SIZE);
    memcpy(&txq->first_tb_bufs[idx], &out_cmd->hdr, tb0_size);
    iwl_pcie_txq_build_tfd(trans, txq,
                           iwl_pcie_get_first_tb_dma(txq, idx),
                           tb0_size, true);
    
    /* map first command fragment, if any remains */
    if (copy_size > tb0_size) {
        struct iwl_dma_ptr *dma = allocate_dma_buf(copy_size - tb0_size, trans->dma_mask);
        if (!dma) {
            iwl_pcie_tfd_unmap(trans, out_meta, txq, txq->write_ptr);
            idx = -ENOMEM;
            goto out;
        }
        out_meta->dma[0] = dma;
        memcpy(dma->addr, ((u8 *)&out_cmd->hdr) + tb0_size, copy_size - tb0_size);
        
        iwl_pcie_txq_build_tfd(trans, txq, dma->dma, copy_size - tb0_size, false);
    }
    
    /* map the remaining (adjusted) nocopy/dup fragments */
    for (i = 0; i < IWL_MAX_CMD_TBS_PER_TFD; i++) {
        const void *data = cmddata[i];
        
        if (!cmdlen[i])
            continue;
        if (!(cmd->dataflags[i] & (IWL_HCMD_DFL_NOCOPY | IWL_HCMD_DFL_DUP)))
            continue;
        if (cmd->dataflags[i] & IWL_HCMD_DFL_DUP)
            data = dup_buf;
        
        struct iwl_dma_ptr *dma = allocate_dma_buf(cmdlen[i], trans->dma_mask);
        if (!dma) {
            iwl_pcie_tfd_unmap(trans, out_meta, txq, txq->write_ptr);
            idx = -ENOMEM;
            goto out;
        }
        out_meta->dma[i + 1] = dma;
        memcpy(dma->addr, data, cmdlen[i]);
        iwl_pcie_txq_build_tfd(trans, txq, dma->dma, cmdlen[i], false);
    }
    
    BUILD_BUG_ON(IWL_TFH_NUM_TBS > sizeof(out_meta->tbs) * BITS_PER_BYTE);
    out_meta->flags = cmd->flags;
    if (txq->entries[idx].free_buf) {
        IWL_INFO(trans, "txq->entries[%d].free_buf is not null", idx);
        iwh_free((void *)txq->entries[idx].free_buf);
    }
    txq->entries[idx].free_buf = dup_buf;
    
    /* start timer if queue currently empty */
    //TODO timer
    //    if (txq->read_ptr == txq->write_ptr && txq->wd_timeout)
    //        mod_timer(&txq->stuck_timer, jiffies + txq->wd_timeout);
    
    //    spin_lock_irqsave(&trans_pcie->reg_lock, flags);
    flags = IOSimpleLockLockDisableInterrupt(trans->m_pDevice->registerRWLock);
    ret = iwl_pcie_set_cmd_in_flight(trans, cmd);
    if (ret < 0) {
        idx = ret;
        //        spin_unlock_irqrestore(&trans_pcie->reg_lock, flags);
        IOSimpleLockUnlockEnableInterrupt(trans->m_pDevice->registerRWLock, flags);
        goto out;
    }
    
    /* Increment and update queue's write index */
    txq->write_ptr = iwl_queue_inc_wrap(txq->write_ptr);
    iwl_pcie_txq_inc_wr_ptr(trans, txq);
    
    //    spin_unlock_irqrestore(&trans_pcie->reg_lock, flags);
    IOSimpleLockUnlockEnableInterrupt(trans->m_pDevice->registerRWLock, flags);
    
out:
    //    spin_unlock_bh(&txq->lock);
free_dup_buf:
    //    if (idx < 0)
    //        kfree(dup_buf);
    return idx;
}

void IWLTransport::syncNmi()
{
    unsigned long timeout = jiffies + IWL_TRANS_NMI_TIMEOUT;
    bool interrupts_enabled = test_bit(STATUS_INT_ENABLED, &this->status);
    u32 inta_addr, sw_err_bit;
    
    if (this->msix_enabled) {
        inta_addr = CSR_MSIX_HW_INT_CAUSES_AD;
        sw_err_bit = MSIX_HW_INT_CAUSES_REG_SW_ERR;
    } else {
        inta_addr = CSR_INT;
        sw_err_bit = CSR_INT_BIT_SW_ERR;
    }
    
    /* if the interrupts were already disabled, there is no point in
     * calling iwl_disable_interrupts
     */
    if (interrupts_enabled)
        disableIntr();
    
    iwlForceNmi();
    while (time_after(timeout, jiffies)) {
        u32 inta_hw = iwlRead32(inta_addr);
        
        /* Error detected by uCode */
        if (inta_hw & sw_err_bit) {
            /* Clear causes register */
            iwlWrite32(inta_addr, inta_hw & sw_err_bit);
            break;
        }
        
        _mdelay(1);
    }
    
    /* enable interrupts only if there were already enabled before this
     * function to avoid a case were the driver enable interrupts before
     * proper configurations were made
     */
    if (interrupts_enabled)
        enableIntr();
    
    //        iwl_trans_fw_error(trans);
}

static int iwl_pcie_send_hcmd_sync(IWLTransport *trans,
                                   struct iwl_host_cmd *cmd)
{
    struct iwl_txq *txq = trans->txq[trans->cmd_queue];
    int cmd_idx;
    int ret;
    
    IWL_INFO(trans, "Attempting to send sync command %s\n",
             iwl_get_cmd_string(trans, cmd->id));
    
    if (test_and_set_bit(STATUS_SYNC_HCMD_ACTIVE,
                         &trans->status)){
        IWL_INFO(0, "Command %s: a command is already active!\n",
                 iwl_get_cmd_string(trans, cmd->id));
                 return -EIO;
    }
    
    IWL_INFO(trans, "Setting HCMD_ACTIVE for command %s\n",
             iwl_get_cmd_string(trans, cmd->id));
    
    cmd_idx = iwl_pcie_enqueue_hcmd(trans, cmd);
    if (cmd_idx < 0) {
        ret = cmd_idx;
        clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
        IWL_ERR(trans,
                "Error sending %s: enqueue_hcmd failed: %d\n",
                iwl_get_cmd_string(trans, cmd->id), ret);
        return ret;
    }
    IWL_INFO(0, "Wait command response\n");
    IOLockLock(trans->wait_command_queue);
    AbsoluteTime deadline;
    clock_interval_to_deadline(HOST_COMPLETE_TIMEOUT * 2, kMillisecondScale, (UInt64 *) &deadline);
    ret = IOLockSleepDeadline(trans->wait_command_queue, &trans->status, deadline, THREAD_INTERRUPTIBLE);
    IOLockUnlock(trans->wait_command_queue);
    if (ret != THREAD_AWAKENED) {
        IWL_ERR(trans, "Error sending %s: time out after %dms.\n",
                iwl_get_cmd_string(trans, cmd->id),
                HOST_COMPLETE_TIMEOUT);
        
        IWL_ERR(trans, "Current CMD queue read_ptr %d write_ptr %d\n",
                txq->read_ptr, txq->write_ptr);
        
        clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
        IWL_INFO(trans, "Clearing HCMD_ACTIVE for command %s\n",
                 iwl_get_cmd_string(trans, cmd->id));
        ret = -ETIMEDOUT;
        
        trans->syncNmi();
        goto cancel;
    }
    IWL_INFO(0, "command response.\n");
    if (test_bit(STATUS_FW_ERROR, &trans->status)) {
        //        iwl_trans_pcie_dump_regs(trans);
        IWL_ERR(trans, "FW error in SYNC CMD %s\n",
                iwl_get_cmd_string(trans, cmd->id));
        //        dump_stack();
        ret = -EIO;
        goto cancel;
    }
    
    if (!(cmd->flags & CMD_SEND_IN_RFKILL) &&
        test_bit(STATUS_RFKILL_OPMODE, &trans->status)) {
        IWL_INFO(trans, "RFKILL in SYNC CMD... no rsp\n");
        ret = -ERFKILL;
        goto cancel;
    }
    
    if ((cmd->flags & CMD_WANT_SKB) && !cmd->resp_pkt) {
        IWL_ERR(trans, "Error: Response NULL in '%s'\n",
                iwl_get_cmd_string(trans, cmd->id));
        ret = -EIO;
        goto cancel;
    }
    
    return 0;
    
cancel:
    if (cmd->flags & CMD_WANT_SKB) {
        /*
         * Cancel the CMD_WANT_SKB flag for the cmd in the
         * TX cmd queue. Otherwise in case the cmd comes
         * in later, it will possibly set an invalid
         * address (cmd->meta.source).
         */
        txq->entries[cmd_idx].meta.flags &= ~CMD_WANT_SKB;
    }
    
    if (cmd->resp_pkt) {
        IWL_INFO(0, "free resp pkt\n");
//        trans->freeResp(cmd);
        cmd->resp_pkt = NULL;
    }
    
    return ret;
}

static int iwl_pcie_send_hcmd_async(IWLTransport *trans,
                                    struct iwl_host_cmd *cmd)
{
        int ret;
        /* An asynchronous command can not expect an SKB to be set. */
        if (WARN_ON(cmd->flags & CMD_WANT_SKB))
            return -EINVAL;
    
        ret = iwl_pcie_enqueue_hcmd(trans, cmd);
        if (ret < 0) {
            IWL_ERR(trans,
                "Error sending %s: enqueue_hcmd failed: %d\n",
                iwl_get_cmd_string(trans, cmd->id), ret);
            return ret;
        }
    return 0;
}

int IWLTransport::pcieSendHCmd(iwl_host_cmd *cmd)
{
    /* Make sure the NIC is still alive in the bus */
    if (test_bit(STATUS_TRANS_DEAD, &this->status))
        return -ENODEV;
    
    if (!(cmd->flags & CMD_SEND_IN_RFKILL) &&
        test_bit(STATUS_RFKILL_OPMODE, &this->status)) {
        IWL_INFO(trans, "Dropping CMD 0x%x: RF KILL\n",
                 cmd->id);
        return -ERFKILL;
    }
    if (cmd->flags & CMD_ASYNC)
        return iwl_pcie_send_hcmd_async(this, cmd);
    /* We still can fail on RFKILL that can be asserted while we wait */
    return iwl_pcie_send_hcmd_sync(this, cmd);
}

#define BUILD_RAxTID(sta_id, tid)    (((sta_id) << 4) + (tid))

bool
iwl_trans_txq_enable_cfg(IWLTransport *trans, int queue, u16 ssn,
             const struct iwl_trans_txq_scd_cfg *cfg,
             unsigned int queue_wdg_timeout)
{
    might_sleep();

    if (WARN_ON_ONCE(trans->state != IWL_TRANS_FW_ALIVE)) {
        IWL_ERR(trans, "%s bad state = %d\n", __func__, trans->state);
        return false;
    }

    struct iwl_txq* txq = trans->txq[queue];
    int txq_id = queue;
    int fifo = -1;
    bool scd_bug = false;
    if(test_and_set_bit(txq_id, trans->queue_used))
        IWL_WARN(0, "queue %d used already, expect issues", txq_id);
    
    txq->wd_timeout = queue_wdg_timeout;
    if(cfg) {
        fifo = cfg->fifo;
        
        /* Disable the scheduler prior configuring the cmd queue */
        if (txq_id == trans->cmd_queue &&
            trans->scd_set_active)
            iwl_scd_enable_set_active(trans, 0);

        /* Stop this Tx queue before configuring it */
        iwl_scd_txq_set_inactive(trans, txq_id);

        /* Set this queue as a chain-building queue unless it is CMD */
        if (txq_id != trans->cmd_queue)
            iwl_scd_txq_set_chain(trans, txq_id);
        if(cfg->aggregate) {
            u16 ra_tid = BUILD_RAxTID(cfg->sta_id, cfg->tid);
            IWL_WARN(0, "need to fix aggregate\n");
            //iwl_pcie_txq_set_ratid_map(ra_tid, txq_id);
            txq->ampdu = true;
        } else {
            iwl_scd_txq_enable_agg(trans, txq_id);
            ssn = txq->read_ptr;
        }
    } else {
        scd_bug = !trans->m_pDevice->cfg->trans.mq_rx_supported &&
            !((ssn - txq->write_ptr) & 0x3f) &&
            (ssn != txq->write_ptr);
        if (scd_bug)
            ssn++;
    }
    
    txq->read_ptr = (ssn & 0xff);
    txq->write_ptr = (ssn & 0xff);
    
    if(cfg) {
        u8 frame_limit = cfg->frame_limit;
        
        trans->iwlWritePRPH(SCD_QUEUE_RDPTR(txq_id), ssn);
        trans->iwlWriteMem32(trans->scd_base_addr +
                             SCD_CONTEXT_QUEUE_OFFSET(txq_id), 0);
        
        trans->iwlWriteMem32(trans->scd_base_addr +
                            SCD_CONTEXT_QUEUE_OFFSET(txq_id) + sizeof(u32),
                             SCD_QUEUE_CTX_REG2_VAL(WIN_SIZE, frame_limit) |
                             SCD_QUEUE_CTX_REG2_VAL(FRAME_LIMIT, frame_limit));
        trans->iwlWritePRPH(SCD_QUEUE_STATUS_BITS(txq_id),
                   (1 << SCD_QUEUE_STTS_REG_POS_ACTIVE) |
                   (cfg->fifo << SCD_QUEUE_STTS_REG_POS_TXF) |
                   (1 << SCD_QUEUE_STTS_REG_POS_WSL) |
                   SCD_QUEUE_STTS_REG_MSK);
        
        if(txq_id == trans->cmd_queue &&
            trans->scd_set_active)
            iwl_scd_enable_set_active(trans, BIT(txq_id));
        
        IWL_INFO(0, "Activate queue %d on fifo %d (wrptr: %d)\n", txq_id, fifo, ssn & 0xff);
    } else {
        IWL_INFO(0, "Activate queue %d (wrptr: %d)\n", txq_id, ssn & 0xff);
    }
    
    
    return scd_bug;
}

void iwl_trans_ac_txq_enable(IWLTransport *trans, int queue, int fifo,
                 unsigned int queue_wdg_timeout)
{
    struct iwl_trans_txq_scd_cfg cfg = {
        .fifo = fifo,
        .sta_id = -1,
        .tid = IWL_MAX_TID_COUNT,
        .frame_limit = IWL_FRAME_LIMIT,
        .aggregate = false,
    };

    iwl_trans_txq_enable_cfg(trans, queue, 0, &cfg, queue_wdg_timeout);
}



void IWLTransport::txqCheckWrPtrs()
{
    
}

void IWLTransport::txStop()
{
    IWL_INFO(0, "Stopping TX DMA channels\n");
    IOInterruptState state;
    int ch, ret;
    u32 mask = 0;
    
    IOSimpleLockLock(irq_lock);
    
    if(!grabNICAccess(&state))
        goto out;
    
    for (ch = 0; ch < FH_TCSR_CHNL_NUM; ch++) {
        iwlWrite32(FH_TCSR_CHNL_TX_CONFIG_REG(ch), 0x0);
        mask |= FH_TSSR_TX_STATUS_REG_MSK_CHNL_IDLE(ch);
    }
    
    ret = iwlPollBit(FH_TSSR_TX_STATUS_REG, mask, mask, 5000);
    if(ret < 0)
        IWL_ERR(this, "failing on interrupt to disable DMA channel %d [0x%0x]\n",
                ch, iwlRead32(FH_TSSR_TX_STATUS_REG));
    
    releaseNICAccess(&state);
    
out:
    IOSimpleLockUnlock(irq_lock);
    return;
    
}
