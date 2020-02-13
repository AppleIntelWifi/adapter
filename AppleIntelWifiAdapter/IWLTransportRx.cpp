//
//  IWLTransportRx.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"
#include <IOKit/IOLocks.h>

/* a device (PCI-E) page is 4096 bytes long */
#define ICT_SHIFT    12
#define ICT_SIZE    (1 << ICT_SHIFT)
#define ICT_COUNT    (ICT_SIZE / sizeof(u32))

/*
 * iwl_rxq_space - Return number of free slots available in queue.
 */
static int iwl_rxq_space(const struct iwl_rxq *rxq)
{
    /* Make sure rx queue size is a power of 2 */
    WARN_ON(rxq->queue_size & (rxq->queue_size - 1));
    
    /*
     * There can be up to (RX_QUEUE_SIZE - 1) free slots, to avoid ambiguity
     * between empty and completely full queues.
     * The following is equivalent to modulo by RX_QUEUE_SIZE and is well
     * defined for negative dividends.
     */
    return (rxq->read - rxq->write - 1) & (rxq->queue_size - 1);
}

/*
 * iwl_dma_addr2rbd_ptr - convert a DMA address to a uCode read buffer ptr
 */
static inline __le32 iwl_pcie_dma_addr2rbd_ptr(dma_addr_t dma_addr)
{
    return cpu_to_le32((u32)(dma_addr >> 8));
}

static int iwl_pcie_free_bd_size(IWLTransport *trans, bool use_rx_td)
{
    struct iwl_rx_transfer_desc *rx_td;

    if (use_rx_td)
        return sizeof(*rx_td);
    else
        return trans->m_pDevice->cfg->trans.mq_rx_supported ? sizeof(__le64) :
            sizeof(__le32);
}

static void iwl_pcie_free_rxq_dma(IWLTransport *trans,
                  struct iwl_rxq *rxq)
{
    IWLDevice *dev = trans->m_pDevice;
    bool use_rx_td = (trans->m_pDevice->cfg->trans.device_family >=
              IWL_DEVICE_FAMILY_AX210);
    int free_size = iwl_pcie_free_bd_size(trans, use_rx_td);

    if (rxq->bd)
        free_dma_buf(rxq->bd_dma_ptr);
    rxq->bd_dma = 0;
    rxq->bd = NULL;

    rxq->rb_stts_dma = 0;
    rxq->rb_stts = NULL;

    if (rxq->used_bd)
        free_dma_buf(rxq->used_bd_dma_ptr);
    rxq->used_bd_dma = 0;
    rxq->used_bd = NULL;

    if (trans->m_pDevice->cfg->trans.device_family < IWL_DEVICE_FAMILY_AX210)
        return;

    if (rxq->tr_tail)
        free_dma_buf(rxq->tr_tail_dma_ptr);
    rxq->tr_tail_dma = 0;
    rxq->tr_tail = NULL;

    if (rxq->cr_tail)
        free_dma_buf(rxq->cr_tail_dma_ptr);
    rxq->cr_tail_dma = 0;
    rxq->cr_tail = NULL;
}

static int iwl_pcie_alloc_rxq_dma(IWLTransport *trans_pcie,
                  struct iwl_rxq *rxq)
{
    IWLDevice *dev = trans_pcie->m_pDevice;
    int i;
    int free_size;
    bool use_rx_td = (dev->cfg->trans.device_family >=
              IWL_DEVICE_FAMILY_AX210);
    size_t rb_stts_size = use_rx_td ? sizeof(__le16) :
                  sizeof(struct iwl_rb_status);

    rxq->lock = IOSimpleLockAlloc();
    if (trans_pcie->m_pDevice->cfg->trans.mq_rx_supported)
        rxq->queue_size = trans_pcie->m_pDevice->cfg->num_rbds;
    else
        rxq->queue_size = RX_QUEUE_SIZE;

    free_size = iwl_pcie_free_bd_size(trans_pcie, use_rx_td);

    /*
     * Allocate the circular buffer of Read Buffer Descriptors
     * (RBDs)
     */
    rxq->bd_dma_ptr = allocate_dma_buf(free_size * rxq->queue_size, trans_pcie->dma_mask);
    if (!rxq->bd_dma_ptr)
        goto err;
    rxq->bd = rxq->bd_dma_ptr->addr;
    rxq->bd_dma = rxq->bd_dma_ptr->dma;

    if (trans_pcie->m_pDevice->cfg->trans.mq_rx_supported) {
        rxq->used_bd_dma_ptr = allocate_dma_buf((use_rx_td ? sizeof(*rxq->cd) : sizeof(__le32)) * rxq->queue_size, trans_pcie->dma_mask);
        if (!rxq->used_bd_dma_ptr)
            goto err;
        rxq->used_bd = rxq->used_bd_dma_ptr->addr;
        rxq->used_bd_dma = rxq->used_bd_dma_ptr->dma;
    }

    rxq->rb_stts = (char *)trans_pcie->base_rb_stts + rxq->id * rb_stts_size;
    rxq->rb_stts_dma =
        trans_pcie->base_rb_stts_dma + rxq->id * rb_stts_size;

    if (!use_rx_td)
        return 0;

    /* Allocate the driver's pointer to TR tail */
    rxq->tr_tail_dma_ptr = allocate_dma_buf(sizeof(__le16), trans_pcie->dma_mask);
    if (!rxq->tr_tail_dma_ptr)
        goto err;
    rxq->tr_tail = (__le16 *)rxq->tr_tail_dma_ptr->addr;
    rxq->tr_tail_dma = rxq->tr_tail_dma_ptr->dma;

    /* Allocate the driver's pointer to CR tail */
    rxq->cr_tail_dma_ptr = allocate_dma_buf(sizeof(__le16), trans_pcie->dma_mask);
    if (!rxq->cr_tail_dma_ptr)
        goto err;
    rxq->cr_tail = (__le16*)rxq->cr_tail_dma_ptr->addr;
    rxq->cr_tail_dma = rxq->cr_tail_dma_ptr->dma;

    return 0;

err:
    for (i = 0; i < trans_pcie->num_rx_queues; i++) {
        rxq = &trans_pcie->rxq[i];

        iwl_pcie_free_rxq_dma(trans_pcie, rxq);
    }

    return -ENOMEM;
}

static int iwl_pcie_rx_alloc(IWLTransport *trans_pcie)
{
    struct iwl_rb_allocator *rba = &trans_pcie->rba;
    int i, ret;
    size_t rb_stts_size = trans_pcie->m_pDevice->cfg->trans.device_family >=
    IWL_DEVICE_FAMILY_AX210 ?
    sizeof(__le16) : sizeof(struct iwl_rb_status);
    
    if (WARN_ON(trans_pcie->rxq))
        return -EINVAL;
    
    trans_pcie->rxq = (iwl_rxq *)kcalloc(trans_pcie->num_rx_queues, sizeof(struct iwl_rxq));
    trans_pcie->rx_pool = (struct iwl_rx_mem_buffer *)kcalloc(RX_POOL_SIZE(trans_pcie->num_rx_bufs),
                                                              sizeof(struct iwl_rx_mem_buffer));
    trans_pcie->global_table =
    (struct iwl_rx_mem_buffer **)kcalloc(RX_POOL_SIZE(trans_pcie->num_rx_bufs),
                                         sizeof(struct iwl_rx_mem_buffer *));
                                         
    if (!trans_pcie->rxq || !trans_pcie->rx_pool ||
        !trans_pcie->global_table) {
        ret = -ENOMEM;
        goto err;
    }
    
    rba->lock = IOSimpleLockAlloc();
    
    /*
     * Allocate the driver's pointer to receive buffer status.
     * Allocate for all queues continuously (HW requirement).
     */
    trans_pcie->base_rb_stts_ptr = allocate_dma_buf(rb_stts_size * trans_pcie->num_rx_queues, trans_pcie->dma_mask);
    if (!trans_pcie->base_rb_stts_ptr) {
        ret = -ENOMEM;
        goto err;
    }
    trans_pcie->base_rb_stts = trans_pcie->base_rb_stts_ptr->addr;
    trans_pcie->base_rb_stts_dma = trans_pcie->base_rb_stts_ptr->dma;
    
    for (i = 0; i < trans_pcie->num_rx_queues; i++) {
        struct iwl_rxq *rxq = &trans_pcie->rxq[i];
        
        rxq->id = i;
        ret = iwl_pcie_alloc_rxq_dma(trans_pcie, rxq);
        if (ret)
            goto err;
    }
    return 0;
    
err:
    if (trans_pcie->base_rb_stts) {
        free_dma_buf(trans_pcie->base_rb_stts_ptr);
        trans_pcie->base_rb_stts = NULL;
        trans_pcie->base_rb_stts_dma = 0;
    }
    IOFree(trans_pcie->rx_pool, RX_POOL_SIZE(trans_pcie->num_rx_bufs) * sizeof(struct iwl_rx_mem_buffer));
    IOFree(trans_pcie->global_table, RX_POOL_SIZE(trans_pcie->num_rx_bufs) * sizeof(struct iwl_rx_mem_buffer *));
    IOFree(trans_pcie->rxq, trans_pcie->num_rx_queues * sizeof(struct iwl_rxq));
    if (trans_pcie->rxq->lock) {
        IOSimpleLockFree(trans_pcie->rxq->lock);
    }
    if (rba->lock) {
        IOSimpleLockFree(rba->lock);
    }
    return ret;
}

static void iwl_pcie_free_rbs_pool(IWLTransport *trans)
{
    int i;
    for (i = 0; i < RX_POOL_SIZE(trans->num_rx_bufs); i++) {
        if (!trans->rx_pool[i].page)
            continue;
//        dma_unmap_page(trans->dev, trans_pcie->rx_pool[i].page_dma,
//                   trans_pcie->rx_buf_bytes, DMA_FROM_DEVICE);
        mbuf_freem(trans->rx_pool[i].page);
        trans->rx_pool[i].page = NULL;
    }
}

/* line 352
 * iwl_pcie_rx_alloc_page - allocates and returns a page.
 *
 */
static mbuf_t iwl_pcie_rx_alloc_page(IWLTransport *trans, u32 *offset)
{
    mbuf_t m;
    unsigned int size;
    mbuf_allocpacket(MBUF_DONTWAIT, PAGE_SIZE, &size, &m);
    return m;
}

/*
 * iwl_pcie_rxq_alloc_rbs - allocate a page for each used RBD
 *
 * A used RBD is an Rx buffer that has been given to the stack. To use it again
 * a page must be allocated and the RBD must point to the page. This function
 * doesn't change the HW pointer but handles the list of pages that is used by
 * iwl_pcie_rxq_restock. The latter function will update the HW to use the newly
 * allocated buffers.
 */
void iwl_pcie_rxq_alloc_rbs(IWLTransport *trans,
                struct iwl_rxq *rxq)
{
    struct iwl_rx_mem_buffer *rxb;
    mbuf_t page;

    while (1) {
        unsigned int offset;

        IOSimpleLockLock(rxq->lock);
        if (TAILQ_EMPTY(&rxq->rx_used)) {
            IOSimpleLockUnlock(rxq->lock);
            return;
        }
        IOSimpleLockUnlock(rxq->lock);

        page = iwl_pcie_rx_alloc_page(trans, &offset);
        if (!page) {
            IWL_ERR(0, "iwl_pcie_rxq_alloc_rbs alloc page failed\n");
            return;
        }

        IOSimpleLockLock(rxq->lock);

        if (TAILQ_EMPTY(&rxq->rx_used)) {
            IOSimpleLockUnlock(rxq->lock);
            mbuf_freem(page);
            return;
        }
        rxb = TAILQ_FIRST(&rxq->rx_used);
        TAILQ_REMOVE(&rxq->rx_used, rxb, list);
        IOSimpleLockUnlock(rxq->lock);

        rxb->page = page;
        rxb->offset = offset;
        /* Get physical address of the RB */
        struct IOMemoryCursor::PhysicalSegment vec;
        int ret = trans->cursor->getPhysicalSegments(page, &vec);
        if (ret == 0) {
            rxb->page = NULL;
            IOSimpleLockLock(rxq->lock);
            TAILQ_INSERT_HEAD(&rxq->rx_used, rxb, list);
            IOSimpleLockUnlock(rxq->lock);
            mbuf_freem(page);
            return;
        }
        rxb->page_dma = vec.location;

        IOSimpleLockLock(rxq->lock);

        TAILQ_INSERT_TAIL(&rxq->rx_free, rxb, list);
        rxq->free_count++;

        IOSimpleLockUnlock(rxq->lock);
        
        IWL_INFO(0, "iwl_pcie_rxq_alloc_rbs succeed\n");
    }
}

void iwl_pcie_rx_init_rxb_lists(struct iwl_rxq *rxq)
{
    TAILQ_INIT(&rxq->rx_free);
    TAILQ_INIT(&rxq->rx_used);
    rxq->free_count = 0;
    rxq->used_count = 0;
}

int IWLTransport::rxInit()
{
    struct iwl_rxq *def_rxq;
    struct iwl_rb_allocator *rba = &this->rba;
    int i, err, queue_size, allocator_pool_size, num_alloc;
    if (!this->cursor) {
        this->cursor = IOMbufNaturalMemoryCursor::withSpecification(PAGE_SIZE, 1);
    }
    if (!this->rxq) {
        err = iwl_pcie_rx_alloc(this);
        if (err) {
            IWL_ERR(0, "iwl_pcie_rx_alloc failed\n");
            return err;
        }
    }
    def_rxq = this->rxq;

    IOSimpleLockLock(rba->lock);
    rba->req_pending = 0;
    rba->req_ready = 0;
    TAILQ_INIT(&rba->rbd_allocated);
    TAILQ_INIT(&rba->rbd_empty);
    IOSimpleLockUnlock(rba->lock);

    /* free all first - we might be reconfigured for a different size */
    iwl_pcie_free_rbs_pool(this);

    for (i = 0; i < RX_QUEUE_SIZE; i++)
        def_rxq->queue[i] = NULL;

    for (i = 0; i < this->num_rx_queues; i++) {
        struct iwl_rxq *rxq = &this->rxq[i];

        IOSimpleLockLock(rxq->lock);
        /*
         * Set read write pointer to reflect that we have processed
         * and used all buffers, but have not restocked the Rx queue
         * with fresh buffers
         */
        rxq->read = 0;
        rxq->write = 0;
        rxq->write_actual = 0;
        memset(rxq->rb_stts, 0,
               (m_pDevice->cfg->trans.device_family >=
                IWL_DEVICE_FAMILY_AX210) ?
               sizeof(__le16) : sizeof(struct iwl_rb_status));

        iwl_pcie_rx_init_rxb_lists(rxq);

        IOSimpleLockUnlock(rxq->lock);
    }

    /* move the pool to the default queue and allocator ownerships */
    queue_size = m_pDevice->cfg->trans.mq_rx_supported ?
    this->num_rx_bufs - 1 : RX_QUEUE_SIZE;
    allocator_pool_size = this->num_rx_queues *
    (RX_CLAIM_REQ_ALLOC - RX_POST_REQ_ALLOC);
    num_alloc = queue_size + allocator_pool_size;

    for (i = 0; i < num_alloc; i++) {
        struct iwl_rx_mem_buffer *rxb = &this->rx_pool[i];

        if (i < allocator_pool_size)
            TAILQ_INSERT_HEAD(&rba->rbd_empty, rxb, list);
        else
            TAILQ_INSERT_HEAD(&def_rxq->rx_used, rxb, list);
        this->global_table[i] = rxb;
        rxb->vid = (u16)(i + 1);
        rxb->invalid = true;
    }

    iwl_pcie_rxq_alloc_rbs(this, def_rxq);

    IWL_INFO(0, "init done\n");
    
    return 0;
}

void IWLTransport::rxHWInit(struct iwl_rxq *rxq)
{
    u32 rb_size;
    IOInterruptState flags;
    const u32 rfdnlog = RX_QUEUE_SIZE_LOG; /* 256 RBDs */
    
    switch (this->rx_buf_size) {
        case IWL_AMSDU_4K:
            rb_size = FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_4K;
            break;
        case IWL_AMSDU_8K:
            rb_size = FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_8K;
            break;
        case IWL_AMSDU_12K:
            rb_size = FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_12K;
            break;
        default:
            WARN_ON(1);
            rb_size = FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_4K;
    }
    
    if (!grabNICAccess(&flags))
        return;
    
    /* Stop Rx DMA */
    iwlWrite32(FH_MEM_RCSR_CHNL0_CONFIG_REG, 0);
    /* reset and flush pointers */
    iwlWrite32(FH_MEM_RCSR_CHNL0_RBDCB_WPTR, 0);
    iwlWrite32(FH_MEM_RCSR_CHNL0_FLUSH_RB_REQ, 0);
    iwlWrite32(FH_RSCSR_CHNL0_RDPTR, 0);
    
    /* Reset driver's Rx queue write index */
    iwlWrite32(FH_RSCSR_CHNL0_RBDCB_WPTR_REG, 0);
    
    /* Tell device where to find RBD circular buffer in DRAM */
    iwlWrite32(FH_RSCSR_CHNL0_RBDCB_BASE_REG,
               (u32)(rxq->bd_dma >> 8));
    
    /* Tell device where in DRAM to update its Rx status */
    iwlWrite32(FH_RSCSR_CHNL0_STTS_WPTR_REG,
               rxq->rb_stts_dma >> 4);
    
    /* Enable Rx DMA
     * FH_RCSR_CHNL0_RX_IGNORE_RXF_EMPTY is set because of HW bug in
     *      the credit mechanism in 5000 HW RX FIFO
     * Direct rx interrupts to hosts
     * Rx buffer size 4 or 8k or 12k
     * RB timeout 0x10
     * 256 RBDs
     */
    iwlWrite32(FH_MEM_RCSR_CHNL0_CONFIG_REG,
               FH_RCSR_RX_CONFIG_CHNL_EN_ENABLE_VAL |
               FH_RCSR_CHNL0_RX_IGNORE_RXF_EMPTY |
               FH_RCSR_CHNL0_RX_CONFIG_IRQ_DEST_INT_HOST_VAL |
               rb_size |
               (RX_RB_TIMEOUT << FH_RCSR_RX_CONFIG_REG_IRQ_RBTH_POS) |
               (rfdnlog << FH_RCSR_RX_CONFIG_RBDCB_SIZE_POS));
    
    releaseNICAccess(&flags);
    
    /* Set interrupt coalescing timer to default (2048 usecs) */
    iwlWrite8(CSR_INT_COALESCING, IWL_HOST_INT_TIMEOUT_DEF);
    
    /* W/A for interrupt coalescing bug in 7260 and 3160 */
    if (m_pDevice->cfg->host_interrupt_operation_mode)
        setBit(CSR_INT_COALESCING, IWL_HOST_INT_OPER_MODE);
}

void IWLTransport::rxMqHWInit()
{
    u32 rb_size, enabled = 0;
    IOInterruptState flags;
    int i;
    
    switch (this->rx_buf_size) {
        case IWL_AMSDU_2K:
            rb_size = RFH_RXF_DMA_RB_SIZE_2K;
            break;
        case IWL_AMSDU_4K:
            rb_size = RFH_RXF_DMA_RB_SIZE_4K;
            break;
        case IWL_AMSDU_8K:
            rb_size = RFH_RXF_DMA_RB_SIZE_8K;
            break;
        case IWL_AMSDU_12K:
            rb_size = RFH_RXF_DMA_RB_SIZE_12K;
            break;
        default:
            WARN_ON(1);
            rb_size = RFH_RXF_DMA_RB_SIZE_4K;
    }
    
    if (!grabNICAccess(&flags))
        return;
    
    /* Stop Rx DMA */
    iwlWritePRPHNoGrab(RFH_RXF_DMA_CFG, 0);
    /* disable free amd used rx queue operation */
    iwlWritePRPHNoGrab(RFH_RXF_RXQ_ACTIVE, 0);
    
    for (i = 0; i < this->num_rx_queues; i++) {
        /* Tell device where to find RBD free table in DRAM */
        iwlWritePRPH64NoGrab(RFH_Q_FRBDCB_BA_LSB(i),
                             this->rxq[i].bd_dma);
        /* Tell device where to find RBD used table in DRAM */
        iwlWritePRPH64NoGrab(RFH_Q_URBDCB_BA_LSB(i),
                             this->rxq[i].used_bd_dma);
        /* Tell device where in DRAM to update its Rx status */
        iwlWritePRPH64NoGrab(RFH_Q_URBD_STTS_WPTR_LSB(i),
                             this->rxq[i].rb_stts_dma);
        /* Reset device indice tables */
        iwlWritePRPHNoGrab(RFH_Q_FRBDCB_WIDX(i), 0);
        iwlWritePRPHNoGrab(RFH_Q_FRBDCB_RIDX(i), 0);
        iwlWritePRPHNoGrab(RFH_Q_URBDCB_WIDX(i), 0);
        
        enabled |= BIT(i) | BIT(i + 16);
    }
    
    /*
     * Enable Rx DMA
     * Rx buffer size 4 or 8k or 12k
     * Min RB size 4 or 8
     * Drop frames that exceed RB size
     * 512 RBDs
     */
    iwlWritePRPHNoGrab(RFH_RXF_DMA_CFG,
                       RFH_DMA_EN_ENABLE_VAL | rb_size |
                       RFH_RXF_DMA_MIN_RB_4_8 |
                       RFH_RXF_DMA_DROP_TOO_LARGE_MASK |
                       RFH_RXF_DMA_RBDCB_SIZE_512);
    
    /*
     * Activate DMA snooping.
     * Set RX DMA chunk size to 64B for IOSF and 128B for PCIe
     * Default queue is 0
     */
    iwlWritePRPHNoGrab(RFH_GEN_CFG,
                       RFH_GEN_CFG_RFH_DMA_SNOOP |
                       RFH_GEN_CFG_VAL(DEFAULT_RXQ_NUM, 0) |
                       RFH_GEN_CFG_SERVICE_DMA_SNOOP |
                       RFH_GEN_CFG_VAL(RB_CHUNK_SIZE,
                                       m_pDevice->cfg->trans.integrated ?
                                       RFH_GEN_CFG_RB_CHUNK_SIZE_64 :
                                       RFH_GEN_CFG_RB_CHUNK_SIZE_128));
    /* Enable the relevant rx queues */
    iwlWritePRPHNoGrab(RFH_RXF_RXQ_ACTIVE, enabled);
    
    releaseNICAccess(&flags);
    
    /* Set interrupt coalescing timer to default (2048 usecs) */
    iwlWrite8(CSR_INT_COALESCING, IWL_HOST_INT_TIMEOUT_DEF);
}

/*
 * iwl_pcie_rxq_restock - refill RX queue from pre-allocated pool
 *
 * If there are slots in the RX queue that need to be restocked,
 * and we have free pre-allocated buffers, fill the ranks as much
 * as we can, pulling from rx_free.
 *
 * This moves the 'write' index forward to catch up with 'processed', and
 * also updates the memory address in the firmware to reference the new
 * target buffer.
 */
void IWLTransport::rxqRestok(struct iwl_rxq *rxq)
{
    if (m_pDevice->cfg->trans.mq_rx_supported)
        rxMqRestock(rxq);
    else
        rxSqRestock(rxq);
}

void IWLTransport::rxMqRestock(struct iwl_rxq *rxq)
{
    struct iwl_rx_mem_buffer *rxb;
    
    /*
     * If the device isn't enabled - no need to try to add buffers...
     * This can happen when we stop the device and still have an interrupt
     * pending. We stop the APM before we sync the interrupts because we
     * have to (see comment there). On the other hand, since the APM is
     * stopped, we cannot access the HW (in particular not prph).
     * So don't try to restock if the APM has been already stopped.
     */
    if (!test_bit(STATUS_DEVICE_ENABLED, &this->status))
        return;
    
    IOSimpleLockLock(rxq->lock);
    while (rxq->free_count) {
        /* Get next free Rx buffer, remove from free list */
        rxb = TAILQ_FIRST(&rxq->rx_free);
        TAILQ_REMOVE(&rxq->rx_free, rxb, list);
        rxb->invalid = false;
        /* some low bits are expected to be unset (depending on hw) */
        //        WARN_ON(rxb->page_dma & this->supported_dma_mask);
        /* Point to Rx buffer via next RBD in circular buffer */
        restockBd(rxq, rxb);
        rxq->write = (rxq->write + 1) & (rxq->queue_size - 1);
        rxq->free_count--;
    }
    IOSimpleLockUnlock(rxq->lock);
    
    /*
     * If we've added more space for the firmware to place data, tell it.
     * Increment device's write pointer in multiples of 8.
     */
    if (rxq->write_actual != (rxq->write & ~0x7)) {
        IOSimpleLockLock(rxq->lock);
        rxqIncWrPtr(rxq);
        IOSimpleLockUnlock(rxq->lock);
    }
}

void IWLTransport::rxSqRestock(struct iwl_rxq *rxq)
{
    struct iwl_rx_mem_buffer *rxb;
    
    /*
     * If the device isn't enabled - not need to try to add buffers...
     * This can happen when we stop the device and still have an interrupt
     * pending. We stop the APM before we sync the interrupts because we
     * have to (see comment there). On the other hand, since the APM is
     * stopped, we cannot access the HW (in particular not prph).
     * So don't try to restock if the APM has been already stopped.
     */
    if (!test_bit(STATUS_DEVICE_ENABLED, &this->status))
        return;
    
    IOSimpleLockLock(rxq->lock);
    while ((iwl_rxq_space(rxq) > 0) && (rxq->free_count)) {
        __le32 *bd = (__le32 *)rxq->bd;
        /* The overwritten rxb must be a used one */
        rxb = rxq->queue[rxq->write];
        //        BUG_ON(rxb && rxb->page);
        
        /* Get next free Rx buffer, remove from free list */
        rxb = TAILQ_FIRST(&rxq->rx_free);
        TAILQ_REMOVE(&rxq->rx_free, rxb, list);
        rxb->invalid = false;
        
        /* Point to Rx buffer via next RBD in circular buffer */
        bd[rxq->write] = iwl_pcie_dma_addr2rbd_ptr(rxb->page_dma);
        rxq->queue[rxq->write] = rxb;
        rxq->write = (rxq->write + 1) & RX_QUEUE_MASK;
        rxq->free_count--;
    }
    IOSimpleLockUnlock(rxq->lock);
    
    /* If we've added more space for the firmware to place data, tell it.
     * Increment device's write pointer in multiples of 8. */
    if (rxq->write_actual != (rxq->write & ~0x7)) {
        IOSimpleLockLock(rxq->lock);
        rxqIncWrPtr(rxq);
        IOSimpleLockUnlock(rxq->lock);
    }
}

void IWLTransport::restockBd(struct iwl_rxq *rxq, struct iwl_rx_mem_buffer *rxb)
{
    if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_AX210) {
        struct iwl_rx_transfer_desc *bd = (struct iwl_rx_transfer_desc *)rxq->bd;
        
        BUILD_BUG_ON(sizeof(*bd) != 2 * sizeof(u64));
        
        bd[rxq->write].addr = cpu_to_le64(rxb->page_dma);
        bd[rxq->write].rbid = cpu_to_le16(rxb->vid);
    } else {
        __le64 *bd = (__le64 *)rxq->bd;
        
        bd[rxq->write] = cpu_to_le64(rxb->page_dma | rxb->vid);
    }
    
    IWL_INFO(0, "Assigned virtual RB ID %u to queue %d index %d\n",
             (u32)rxb->vid, rxq->id, rxq->write);
}

/*
 * iwl_pcie_rxq_inc_wr_ptr - Update the write pointer for the RX queue
 */
void IWLTransport::rxqIncWrPtr(struct iwl_rxq *rxq)
{
    u32 reg;
    //        lockdep_assert_held(&rxq->lock);
    /*
     * explicitly wake up the NIC if:
     * 1. shadow registers aren't enabled
     * 2. there is a chance that the NIC is asleep
     */
    if (!m_pDevice->cfg->trans.base_params->shadow_reg_enable &&
        test_bit(STATUS_TPOWER_PMI, &this->status)) {
        reg = iwlRead32(CSR_UCODE_DRV_GP1);
        
        if (reg & CSR_UCODE_DRV_GP1_BIT_MAC_SLEEP) {
            IWL_INFO(0, "Rx queue requesting wakeup, GP1 = 0x%x\n",
                     reg);
            setBit(CSR_GP_CNTRL,
                   CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
            rxq->need_update = true;
            return;
        }
    }
    
    rxq->write_actual = round_down(rxq->write, 8);
    if (m_pDevice->cfg->trans.mq_rx_supported)
        iwlWrite32(RFH_Q_FRBDCB_WIDX_TRG(rxq->id),
                   rxq->write_actual);
    else
        iwlWrite32(FH_RSCSR_CHNL0_WPTR, rxq->write_actual);
}

void IWLTransport::rxFree()
{
    
}

void IWLTransport::disableICT()
{
    IOSimpleLockLock(this->irq_lock);
    this->use_ict = false;
    IOSimpleLockUnlock(this->irq_lock);
}

u32 IWLTransport::intrCauseNonICT()
{
    u32 inta;
    /* Discover which interrupts are active/pending */
    inta = iwlRead32(CSR_INT);
    /* the thread will service interrupts and re-enable them */
    return inta;
}

u32 IWLTransport::intrCauseICT()
{
    u32 inta;
    u32 val = 0;
    u32 read;
    
    /* Ignore interrupt if there's nothing in NIC to service.
     * This may be due to IRQ shared with another device,
     * or due to sporadic interrupts thrown from our NIC. */
    read = le32_to_cpu(this->ict_tbl[this->ict_index]);
    if (!read)
        return 0;
    
    /*
     * Collect all entries up to the first 0, starting from ict_index;
     * note we already read at ict_index.
     */
    do {
        val |= read;
        IWL_INFO(0, "ICT index %d value 0x%08X\n",
                 this->ict_index, read);
        this->ict_tbl[this->ict_index] = 0;
        this->ict_index =
        ((this->ict_index + 1) & (ICT_COUNT - 1));
        
        read = le32_to_cpu(this->ict_tbl[this->ict_index]);
    } while (read);
    
    /* We should not get this value, just ignore it. */
    if (val == 0xffffffff)
        val = 0;
    
    /*
     * this is a w/a for a h/w bug. the h/w bug may cause the Rx bit
     * (bit 15 before shifting it to 31) to clear when using interrupt
     * coalescing. fortunately, bits 18 and 19 stay set when this happens
     * so we use them to decide on the real state of the Rx bit.
     * In order words, bit 15 is set if bit 18 or bit 19 are set.
     */
    if (val & 0xC0000)
        val |= 0x8000;
    
    inta = (0xff & val) | ((0xff00 & val) << 16);
    return inta;
}

void IWLTransport::resetICT()
{
    u32 val;
    
    if (!this->ict_tbl)
        return;
    
    IOSimpleLockLock(this->irq_lock);
    disableIntrDirectly();
    memset(this->ict_tbl, 0, ICT_SIZE);
    val = this->ict_tbl_dma >> ICT_SHIFT;
    
    val |= CSR_DRAM_INT_TBL_ENABLE |
    CSR_DRAM_INIT_TBL_WRAP_CHECK |
    CSR_DRAM_INIT_TBL_WRITE_POINTER;
    
    IWL_INFO(0, "CSR_DRAM_INT_TBL_REG =0x%x\n", val);
    
    iwlWrite32(CSR_DRAM_INT_TBL_REG, val);
    this->use_ict = true;
    this->ict_index = 0;
    iwlWrite32(CSR_INT, this->inta_mask);
    enableIntrDirectly();
    IOSimpleLockUnlock(this->irq_lock);
}

void IWLTransport::rxqCheckWrPtr()
{
    int i;
    for (i = 0; i < this->num_rx_queues; i++) {
        struct iwl_rxq *rxq = &this->rxq[i];
        
        if (!rxq->need_update)
            continue;
        IOSimpleLockLock(rxq->lock);
        rxqIncWrPtr(rxq);
        rxq->need_update = false;
        IOSimpleLockUnlock(rxq->lock);
    }
}

/*
 * iwl_pcie_rx_stop - stops the Rx DMA
 */
int IWLTransport::rxStop()
{
    if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_AX210) {
        /* TODO: remove this once fw does it */
        iwlWriteUmacPRPH(RFH_RXF_DMA_CFG_GEN3, 0);
        return iwlPollUmacPRPHBit(RFH_GEN_STATUS_GEN3,
                                  RXF_DMA_IDLE, RXF_DMA_IDLE, 1000);
    } else if (m_pDevice->cfg->trans.mq_rx_supported) {
        iwlWritePRPH(RFH_RXF_DMA_CFG, 0);
        return iwlPollPRPHBit(RFH_GEN_STATUS,
                              RXF_DMA_IDLE, RXF_DMA_IDLE, 1000);
    } else {
        iwlWriteDirect32(FH_MEM_RCSR_CHNL0_CONFIG_REG, 0);
        return iwlPollDirectBit(FH_MEM_RSSR_RX_STATUS_REG,
                                FH_RSSR_CHNL0_RX_STATUS_CHNL_IDLE,
                                1000);
    }
}
