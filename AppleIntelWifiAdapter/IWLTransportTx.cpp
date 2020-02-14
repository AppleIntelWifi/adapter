//
//  IWLTransportTx.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"

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

static u8 iwl_pcie_tfd_get_num_tbs(IWLTransport *trans, void *_tfd)
{
    if (trans->m_pDevice->cfg->trans.use_tfh) {
        struct iwl_tfh_tfd *tfd = (struct iwl_tfh_tfd *)_tfd;
        
        return le16_to_cpu(tfd->num_tbs) & 0x1f;
    } else {
        struct iwl_tfd *tfd = (struct iwl_tfd *)_tfd;
        
        return tfd->num_tbs & 0x1f;
    }
}

static inline int iwl_queue_inc_wrap(int index)
{
    return ++index & (TFD_QUEUE_SIZE_MAX - 1);
}

/**
 * iwl_queue_dec_wrap - decrement queue index, wrap back to end
 * @index -- current index
 */
static inline int iwl_queue_dec_wrap(int index)
{
    return --index & (TFD_QUEUE_SIZE_MAX - 1);
}


static inline u8 iwl_pcie_get_cmd_index(struct iwl_txq *q, u32 index)
{
    return index & (q->n_window - 1);
}

static inline void *iwl_pcie_get_tfd(IWLTransport *trans_pcie, struct iwl_txq *txq, int idx)
{
    return (u8*)txq->tfds + trans_pcie->tfd_size * iwl_pcie_get_cmd_index(txq, idx);
}

int iwl_queue_space(const struct iwl_txq *q)
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
    
    //allocate_dma_buf(free_size * rxq->queue_size, trans_pcie->dma_mask);
    //struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
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

    trans_pcie->txq_memory = (struct iwl_txq *)kzalloc(sizeof(struct iwl_txq) * trans_pcie->m_pDevice->cfg->trans.base_params->num_of_queues);
    if (!trans_pcie->txq_memory) {
        IWL_ERR(trans, "Not enough memory for txq\n");
        ret = -ENOMEM;
        goto error;
    }
    
    /* Alloc and init all Tx queues, including the command queue (#4/#9) */
    for (txq_id = 0; txq_id < trans->m_pDevice->cfg->trans.base_params->num_of_queues; txq_id++) {
        bool cmd_queue = (txq_id == trans_pcie->cmd_queue);
        
        slots_num = cmd_queue ? TFD_CMD_SLOTS : TFD_TX_CMD_SLOTS;
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
    IOFree(trans, sizeof(trans));
    
    return ret;
}

int iwl_pcie_txq_init(IWLTransport *trans, struct iwl_txq *txq, int slots_num, bool cmd_queue)
{
    int ret;
    
    txq->need_update = false;
    
    /* TFD_QUEUE_SIZE_MAX must be power-of-two size, otherwise
     * iwl_queue_inc_wrap and iwl_queue_dec_wrap are broken. */
    BUILD_BUG_ON(TFD_QUEUE_SIZE_MAX & (TFD_QUEUE_SIZE_MAX - 1));
    
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

static void iwl_pcie_tfd_unmap(IWLTransport *trans, struct iwl_cmd_meta *meta, struct iwl_txq *txq, int index)
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
        
        slots_num = cmd_queue ? TFD_CMD_SLOTS : TFD_TX_CMD_SLOTS;
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
        //iwl_write_direct32(trans, FH_MEM_CBBC_QUEUE(trans, txq_id),
        //                   (u32)trans_pcie->txq[txq_id]->dma_addr >> 8);
    }
    iwlSetBitsPRPH(SCD_GP_CTRL, SCD_GP_CTRL_AUTO_ACTIVE_MODE);
    //iwl_set_bits_prph(trans, SCD_GP_CTRL, SCD_GP_CTRL_AUTO_ACTIVE_MODE);
    if (m_pDevice->cfg->trans.base_params->num_of_queues > 20)
        iwlSetBitsPRPH(SCD_GP_CTRL, SCD_GP_CTRL_ENABLE_31_QUEUES);
        //iwl_set_bits_prph(trans, SCD_GP_CTRL, SCD_GP_CTRL_ENABLE_31_QUEUES);
    
    return 0;
error:
    IWL_ERR(0, "Could not init tx ring\n");
    return -EIO;
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

static void iwl_pcie_clear_cmd_in_flight(IWLTransport *trans)
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

static int iwl_pcie_send_hcmd_sync(IWLTransport *trans,
                   struct iwl_host_cmd *cmd)
{
//    struct iwl_txq *txq = trans->txq[trans->cmd_queue];
//    int cmd_idx;
//    int ret;
//
//    IWL_INFO(trans, "Attempting to send sync command %s\n",
//               iwl_get_cmd_string(trans, cmd->id));
//
//    if (test_and_set_bit(STATUS_SYNC_HCMD_ACTIVE,
//                         &trans->status)){
//        IWL_INFO(0, "Command %s: a command is already active!\n",
//                 iwl_get_cmd_string(trans, cmd->id));
//    }
//        return -EIO;
//
//    IWL_INFO(trans, "Setting HCMD_ACTIVE for command %s\n",
//               iwl_get_cmd_string(trans, cmd->id));
//
//    cmd_idx = iwl_pcie_enqueue_hcmd(trans, cmd);
//    if (cmd_idx < 0) {
//        ret = cmd_idx;
//        clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
//        IWL_ERR(trans,
//            "Error sending %s: enqueue_hcmd failed: %d\n",
//            iwl_get_cmd_string(trans, cmd->id), ret);
//        return ret;
//    }
//
//    ret = wait_event_timeout(trans_pcie->wait_command_queue,
//                 !test_bit(STATUS_SYNC_HCMD_ACTIVE,
//                       &trans->status),
//                 HOST_COMPLETE_TIMEOUT);
//    if (!ret) {
//        IWL_ERR(trans, "Error sending %s: time out after %dms.\n",
//            iwl_get_cmd_string(trans, cmd->id),
//            jiffies_to_msecs(HOST_COMPLETE_TIMEOUT));
//
//        IWL_ERR(trans, "Current CMD queue read_ptr %d write_ptr %d\n",
//            txq->read_ptr, txq->write_ptr);
//
//        clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
//        IWL_INFO(trans, "Clearing HCMD_ACTIVE for command %s\n",
//                   iwl_get_cmd_string(trans, cmd->id));
//        ret = -ETIMEDOUT;
//
//        iwl_trans_pcie_sync_nmi(trans);
//        goto cancel;
//    }
//
//    if (test_bit(STATUS_FW_ERROR, &trans->status)) {
//        iwl_trans_pcie_dump_regs(trans);
//        IWL_ERR(trans, "FW error in SYNC CMD %s\n",
//            iwl_get_cmd_string(trans, cmd->id));
//        dump_stack();
//        ret = -EIO;
//        goto cancel;
//    }
//
//    if (!(cmd->flags & CMD_SEND_IN_RFKILL) &&
//        test_bit(STATUS_RFKILL_OPMODE, &trans->status)) {
//        IWL_INFO(trans, "RFKILL in SYNC CMD... no rsp\n");
//        ret = -ERFKILL;
//        goto cancel;
//    }
//
//    if ((cmd->flags & CMD_WANT_SKB) && !cmd->resp_pkt) {
//        IWL_ERR(trans, "Error: Response NULL in '%s'\n",
//            iwl_get_cmd_string(trans, cmd->id));
//        ret = -EIO;
//        goto cancel;
//    }

    return 0;

//cancel:
//    if (cmd->flags & CMD_WANT_SKB) {
//        /*
//         * Cancel the CMD_WANT_SKB flag for the cmd in the
//         * TX cmd queue. Otherwise in case the cmd comes
//         * in later, it will possibly set an invalid
//         * address (cmd->meta.source).
//         */
//        txq->entries[cmd_idx].meta.flags &= ~CMD_WANT_SKB;
//    }
//
//    if (cmd->resp_pkt) {
//        trans->freeResp(cmd);
//        cmd->resp_pkt = NULL;
//    }
//zwz
//    return ret;
}

static int iwl_pcie_send_hcmd_async(IWLTransport *trans,
                    struct iwl_host_cmd *cmd)
{
//    int ret;
//    /* An asynchronous command can not expect an SKB to be set. */
//    if (WARN_ON(cmd->flags & CMD_WANT_SKB))
//        return -EINVAL;
//
//    //TODO
//    ret = iwl_pcie_enqueue_hcmd(trans, cmd);
//    if (ret < 0) {
//        IWL_ERR(trans,
//            "Error sending %s: enqueue_hcmd failed: %d\n",
//            iwl_get_cmd_string(trans, cmd->id), ret);
//        return ret;
//    }
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

void IWLTransport::txStop()
{
    
}
