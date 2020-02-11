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
#include "IWLDevice.hpp"
#include "TransHdr.h"
#include "IWLCtxtInfo.hpp"
#include "IWLInternal.hpp"

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
    void disableICT();
    
    void rxStop();
    
    u32 intrCauseICT();
    
    u32 intrCauseNonICT();
    
    void irqHandleError();
    
    //tx
    void freeResp(struct iwl_host_cmd *cmd);
    
    int pcieSendHCmd(iwl_host_cmd *cmd);
    
    void txStop();
    
    //cmd
    int sendCmd(struct iwl_host_cmd *cmd);
    
public:
    //a bit-mask of transport status flags
    unsigned long status;
    IOSimpleLock *irq_lock;
    IOLock *mutex;
    
    //pci
    mach_vm_address_t dma_mask;
    
    int tfd_size;
    int max_tbs;
    
    //waitLocks
    IOLock *ucode_write_waitq;
    
    //fw
    enum iwl_trans_state state;
    bool ucode_write_complete;
    union {
        struct iwl_context_info *ctxt_info;
        struct iwl_context_info_gen3 *ctxt_info_gen3;
    };
    struct iwl_prph_info *prph_info;
    struct iwl_prph_scratch *prph_scratch;
    dma_addr_t ctxt_info_dma_addr;
    iwl_dma_ptr *ctxt_info_dma_ptr;
    dma_addr_t prph_info_dma_addr;
    iwl_dma_ptr *prph_info_dma_ptr;
    dma_addr_t prph_scratch_dma_addr;
    iwl_dma_ptr *prph_scratch_dma_ptr;
    dma_addr_t iml_dma_addr;
    iwl_dma_ptr *iml_dma_ptr;
    iwl_self_init_dram init_dram;
    
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
    iwl_txq *txq[IWL_MAX_TVQM_QUEUES];
    
    //rx
    bool use_ict;
    int num_rx_queues;
    int def_rx_queue;
    int rx_buf_size;
    iwl_rxq rxq;
    
    bool opmode_down;
    bool is_down;
    
    //interrupt
    u32 inta_mask;
    
private:
    
    int setHWReady();
    
    void enableHWIntrMskMsix(u32 msk);//iwl_enable_hw_int_msk_msix
    
private:
    
    
    //msix
    u32 fh_init_mask;
    u32 hw_init_mask;
    u32 fh_mask;
    u32 hw_mask;
    bool msix_enabled;
    
    
};

#endif /* IWLTransport_hpp */
