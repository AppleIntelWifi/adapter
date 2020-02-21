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
#include <libkern/c++/OSObject.h>

#include "IWLTransOps.h"
#include "../fw/IWLUcodeParse.hpp"
#include "IWLNvmParser.hpp"
#include "IWLPhyDb.hpp"
#include "IWMHdr.h"

class IWLMvmDriver : public OSObject {
    OSDeclareDefaultStructors( IWLMvmDriver );
public:
    
    /* MARK: IOKit-related initialization code */
    
    bool init(IOPCIDevice *pciDevice);
    
    void release();
    
    bool probe();
    
    bool start();
    
    /* MARK: Firmware loading */
    static void reqFWCallback(
                              OSKextRequestTag requestTag,
                              OSReturn result,
                              const void* resourceData,
                              uint32_t resourceDataLength,
                              void* context);
    
    /* MARK: Callbacks from the IOKit driver */
    
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
    
    int sendCmdPduStatus(u32 id, u16 len, const void* data, u32* status);
    
    
    bool enableDevice();
    
    
    ///openbsd ieee80211
    bool ieee80211Init();
    
    void ieee80211Release();
    
    bool ieee80211Run();
    
    void iwm_setup_ht_rates();
    
    int iwm_binding_cmd(struct iwm_node *in, uint32_t action);
    
    typedef int (*BgScanAction)(struct ieee80211com *ic);
    int iwm_bgscan(struct ieee80211com *ic);
    
    typedef struct ieee80211_node *(*NodeAllocAction)(struct ieee80211com *ic);
    struct ieee80211_node *iwm_node_alloc(struct ieee80211com *ic);
    
    typedef int (*NewStateAction)(struct ieee80211com *, enum ieee80211_state, int);
    int iwm_newstate(struct ieee80211com *, enum ieee80211_state, int);
    
public:
    IOEthernetController* controller;
    IWLDevice *m_pDevice;
    
    IWLTransport *trans;
    
    IWLTransOps *trans_ops;
    
/* MARK: Rx handlers */
    static void btCoexNotif(struct iwl_mvm *mvm, struct iwl_rx_cmd_buffer *rxb);
    static void rxFwErrorNotif(struct iwl_mvm *mvm, struct iwl_rx_cmd_buffer *rxb);
    static void rxMfuartNotif(struct iwl_mvm* mvm, struct iwl_rx_cmd_buffer *rxb);
    
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
