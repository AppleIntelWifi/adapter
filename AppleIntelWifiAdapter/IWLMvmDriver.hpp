//
//  IWLMvmDriver.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/17.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmDriver_hpp
#define IWLMvmDriver_hpp

#include <libkern/OSKextLib.h>
#include <IOKit/IOLib.h>

#include "IWLTransOps.h"
#include "fw/IWLUcodeParse.hpp"
#include "IWLNvmParser.hpp"
#include "IWLPhyDb.hpp"

class IWLMvmDriver {
    
public:
    
    bool init(IOPCIDevice *pciDevice);
    
    void release();
    
    bool probe();
    
    bool start();
    
    static void reqFWCallback(
                              OSKextRequestTag requestTag,
                              OSReturn result,
                              const void* resourceData,
                              uint32_t resourceDataLength,
                              void* context);
    
    bool drvStart();
    
    int irqHandler(int irq, void *dev_id);
    
    void stopDevice();//iwl_mvm_stop_device
    
    
    //fw
    
    /**
     * iwl_get_nvm - retrieve NVM data from firmware
     *
     * Allocates a new iwl_nvm_data structure, fills it with
     * NVM data, and returns it to caller.
     */
    struct iwl_nvm_data *getNvm(IWLTransport *trans,
                                const struct iwl_fw *fw);
    
    int loadUcodeWaitAlive(enum iwl_ucode_type ucode_type);//iwl_mvm_load_ucode_wait_alive
    
    int runInitMvmUCode(bool read_nvm);//iwl_run_init_mvm_ucode
    
    int runUnifiedMvmUcode(bool read_nvm);
    
    int nvmInit();//iwl_nvm_init
    
    int sendPhyCfgCmd();//iwl_send_phy_cfg_cmd
    
    int sendTXAntCfg(u8 valid_tx_ant);//iwl_send_tx_ant_cfg
    
    struct iwl_mcc_update_resp *updateMcc(const char *alpha2, enum iwl_mcc_source src_id);//iwl_mvm_update_mcc
    
    int initMcc();//iwl_mvm_init_mcc
    
    void rxChubUpdateMcc(struct iwl_rx_cmd_buffer *rxb);//iwl_mvm_rx_chub_update_mcc
    
    //bt coex
    int sendBTInitConf(); //iwl_mvm_send_bt_init_conf
    
    //utils
    int sendCmd(struct iwl_host_cmd *cmd);
    
    int sendCmdPdu(u32 id, u32 flags, u16 len, const void *data);//iwl_mvm_send_cmd_pdu
    
    int sendCmdStatus(struct iwl_host_cmd *cmd, u32 *status);//iwl_mvm_send_cmd_status
    
public:
    IOEthernetController* controller;
    IWLDevice *m_pDevice;
    
    IWLTransport *trans;
    
    IWLTransOps *trans_ops;
    
private:
    
    IOLock *fwLoadLock;
    
    struct iwl_phy_db phy_db;
    
};

static inline bool iwl_mvm_has_new_rx_api(struct iwl_fw *fw)
{
    return fw_has_capa(&fw->ucode_capa,
                       IWL_UCODE_TLV_CAPA_MULTI_QUEUE_RX_SUPPORT);
}

#endif /* IWLMvmDriver_hpp */
