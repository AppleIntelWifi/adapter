//
//  IWLTransport.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLTransport_hpp
#define IWLTransport_hpp

#include "IWLIO.hpp"
#include "../IWLDevice.hpp"
#include "TransHdr.h"
#include "../IWLCtxtInfo.hpp"
#include "../IWLInternal.hpp"

struct sk_buff { int something; };


class IWLTransport : public IWLIO
{
    
public:
    IWLTransport();
    ~IWLTransport();
    
    bool init(IWLDevice *device);//iwl_trans_pcie_alloc
    
    bool prepareCardHW();
    
    void release();
    
    bool finishNicInit();
    
    ///
    ///msix
    void initMsix();
    
    void configMsixHw();
    
    int clearPersistenceBit();
    
    
    ///apm
    void apmConfig();
    
    void swReset();
    /*
    * Enable LP XTAL to avoid HW bug where device may consume much power if
    * FW is not loaded after device reset. LP XTAL is disabled by default
    * after device HW reset. Do it only if XTAL is fed by internal source.
    * Configure device's "persistence" mode to avoid resetting XTAL again when
    * SHRD_HW_RST occurs in S3.
    */
    void apmLpXtalEnable();
    
    void apmStopMaster();
    
    void disableIntr();
    
    void disableIntrDirectly();
    
    void enableIntr();
    
    void enableIntrDirectly();
    
    void enableRFKillIntr();
    
    void enableFWLoadIntr();//iwl_enable_fw_load_int
    
    void enableFHIntrMskMsix(u32 msk);//iwl_enable_fh_int_msk_msix
    
    bool isRFKikkSet();
    
    void setRfKillState(bool state);//iwl_trans_pcie_rf_kill
    
    ///fw
    void loadFWChunkFh(u32 dst_addr, dma_addr_t phy_addr, u32 byte_cnt);
    
    int loadFWChunk(u32 dst_addr, dma_addr_t phy_addr,
                    u32 byte_cnt);
    
    int loadSection(u8 section_num, const struct fw_desc *section);
    
    int loadCPUSections8000(const struct fw_img *image, int cpu, int *first_ucode_section);
    
    int loadCPUSections(const struct fw_img *image, int cpu, int *first_ucode_section);
    
    int loadGivenUcode(const struct fw_img *image);
    
    int loadGivenUcode8000(const struct fw_img *image);
    
    //ctxt
    int iwl_pcie_ctxt_info_init(const struct fw_img *fw);
    void iwl_pcie_ctxt_info_free();
    void iwl_pcie_ctxt_info_free_paging();
    int iwl_pcie_init_fw_sec(const struct fw_img *fw,
                 struct iwl_context_info_dram *ctxt_dram);
    int iwl_pcie_ctxt_info_gen3_init(const struct fw_img *fw);
    void iwl_pcie_ctxt_info_gen3_free();
    void iwl_enable_fw_load_int_ctx_info();
    void iwl_pcie_ctxt_info_free_fw_img();
    int iwl_pcie_get_num_sections(const struct fw_img *fw, int start);
    
    ///
    ///power
    void setPMI(bool state);
    
    //rx
    int rxInit();//_iwl_pcie_rx_init
    
    void rxFree();//iwl_pcie_rx_free
    
    void disableICT();
    
    int rxStop();//iwl_pcie_rx_stop
    
    void rxMqHWInit();
    
    void rxHWInit(struct iwl_rxq *rxq);
    
    void rxqRestok(struct iwl_rxq *rxq);
    
    void handleRx(int queue);//iwl_pcie_rx_handle
    
    void restockBd(struct iwl_rxq *rxq,
                   struct iwl_rx_mem_buffer *rxb);//iwl_pcie_restock_bd
    
    void rxMqRestock(struct iwl_rxq *rxq);//iwl_pcie_rxmq_restock
    
    void rxSqRestock(struct iwl_rxq *rxq);//iwl_pcie_rxsq_restock
    
    void rxqIncWrPtr(struct iwl_rxq *rxq);//iwl_pcie_rxq_inc_wr_ptr
    
    void rxqCheckWrPtr();//iwl_pcie_rxq_check_wrptr
    
    void resetICT();
    
    int allocICT();
    
    void freeICT();
    
    u32 intrCauseICT();//iwl_pcie_int_cause_ict
    
    u32 intrCauseNonICT();//iwl_pcie_int_cause_non_ict
    
    void irqHandleError();
    
    //tx
    void freeResp(struct iwl_host_cmd *cmd);
    
    int pcieSendHCmd(iwl_host_cmd *cmd);
    
    int txInit();
    
    int txFree();
    
    void txStop();
    
    void txStart();
    
    void txqCheckWrPtrs();//iwl_pcie_txq_check_wrptrs
    
    void syncNmi();
    
    //cmd
    int sendCmd(struct iwl_host_cmd *cmd);
    
public:
    //a bit-mask of transport status flags
    unsigned long status;
    IOSimpleLock *irq_lock;
    IOLock *mutex;//to protect stop_device / start_fw / start_hw
    
    //pci
    mach_vm_address_t dma_mask;
    
    int tfd_size;
    int max_tbs;
    
    //fw
    enum iwl_trans_state state;
    bool ucode_write_complete;//indicates that the ucode has been copied.
    IOLock *ucode_write_waitq;//wait queue for uCode load
    IOLock *wait_command_queue;
    union {
        struct iwl_context_info *ctxt_info;
        struct iwl_context_info_gen3 *ctxt_info_gen3;
    };
    struct iwl_prph_info *prph_info;//rph info for self init
    struct iwl_prph_scratch *prph_scratch;//prph scratch for self init
    dma_addr_t ctxt_info_dma_addr;//dma addr of context information
    iwl_dma_ptr *ctxt_info_dma_ptr;
    dma_addr_t prph_info_dma_addr;//dma addr of prph info
    iwl_dma_ptr *prph_info_dma_ptr;
    dma_addr_t prph_scratch_dma_addr;//dma addr of prph scratch
    iwl_dma_ptr *prph_scratch_dma_ptr;
    dma_addr_t iml_dma_addr;
    iwl_dma_ptr *iml_dma_ptr;
    iwl_self_init_dram init_dram;//DRAM data of firmware image (including paging). Context information addresses will be taken from here. This is driver's local copy for keeping track of size and count for allocating and freeing the memory.
    
    //tx--cmd
    int cmd_queue;
    bool wide_cmd_header;
    IOLock *syncCmdLock;//sync and async lock
    const struct iwl_hcmd_arr *command_groups;
    int command_groups_size;
    int cmd_fifo;
    int txcmd_size;
    int txcmd_align;
    
    //tx
    struct iwl_txq *txq_memory;
    iwl_txq *txq[IWL_MAX_TVQM_QUEUES];
    unsigned long queue_used[BITS_TO_LONGS(IWL_MAX_TVQM_QUEUES)];
    unsigned long queue_stopped[BITS_TO_LONGS(IWL_MAX_TVQM_QUEUES)];
    unsigned int cmd_q_wdg_timeout;
    
    //rx
    int num_rx_queues;
    int def_rx_queue;//default rx queue number
    int rx_buf_size;//Rx buffer size
    iwl_rxq *rxq;//all the RX queue data
    int num_rx_bufs;
    int global_table_array_size;
    struct iwl_rx_mem_buffer *rx_pool;
    struct iwl_rx_mem_buffer **global_table;
    struct iwl_rb_allocator rba;
    struct isr_statistics isr_stats;
    
    void *base_rb_stts;//base virtual address of receive buffer status for all queues
    dma_addr_t base_rb_stts_dma;//base physical address of receive buffer status
    iwl_dma_ptr *base_rb_stts_ptr;//base physical dma object of receive buffer status
    
    //interrupt
    bool use_ict;
    __le32 *ict_tbl;
    dma_addr_t ict_tbl_dma;
    iwl_dma_ptr *ict_tbl_ptr;
    bool opmode_down;
    bool is_down;
    int ict_index;
    u32 inta_mask;
    bool scd_set_active;
    u32 scd_base_addr;//scheduler sram base address in SRAM
    iwl_dma_ptr *scd_bc_tbls;//pointer to the byte count table of the scheduler
    iwl_dma_ptr *kw;//keep warm address
    u8 no_reclaim_cmds[1];
    
private:
    
    int setHWReady();
    
    void enableHWIntrMskMsix(u32 msk);//iwl_enable_hw_int_msk_msix
    
private:
    
    
    //msix
    u32 fh_init_mask;
    u32 hw_init_mask;
    u32 fh_mask;
    u32 hw_mask;
//    struct msix_entry msix_entries[IWL_MAX_RX_HW_QUEUES];
    bool msix_enabled;//true if managed to enable MSI-X
    
    
};

#include <sys/kpi_mbuf.h>

void iwl_pcie_tfd_unmap(IWLTransport *trans, struct iwl_cmd_meta *meta, struct iwl_txq *txq, int index);
const char *iwl_get_cmd_string(IWLTransport *trans, u32 id);
void iwl_pcie_clear_cmd_in_flight(IWLTransport *trans);

static inline void *rxb_addr(struct iwl_rx_cmd_buffer *r)
{
    return (void *)((u8*)mbuf_data((mbuf_t)r->_page) + r->_offset);
}

static inline int rxb_offset(struct iwl_rx_cmd_buffer *r)
{
    return r->_offset;
}

static inline mbuf_t rxb_steal_page(struct iwl_rx_cmd_buffer *r)
{
    r->_page_stolen = true;
    return (mbuf_t)r->_page;
}

static inline void iwl_free_rxb(struct iwl_rx_cmd_buffer *r)
{
    //__free_pages(r->_page, r->_rx_page_order);
}

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

#endif /* IWLTransport_hpp */
