//
//  IWLMvmSta.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/19/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmSta.hpp"

bool
iwl_trans_txq_enable_cfg(IWLTransport *trans, int queue, u16 ssn,
             const struct iwl_trans_txq_scd_cfg *cfg,
                         unsigned int queue_wdg_timeout);

int iwl_enable_txq(IWLMvmDriver* drv, int sta_id, int qid, int fifo) {
    struct iwl_scd_txq_cfg_cmd cfg = {
        .tx_fifo = fifo,
        .sta_id = sta_id,
        .action = 1,
        .scd_queue = qid,
        .window = IWL_FRAME_LIMIT,
        .aggregate = false,
    };
    int err;
    
    if((err = drv->sendCmdPdu(SCD_QUEUE_CFG, 0, sizeof(cfg), &cfg))) {
        return err;
    }
    IOInterruptState state;
    if(!drv->trans->grabNICAccess(&state))
        return 1;
    
    drv->trans->iwlWritePRPHNoGrab(SCD_EN_CTRL, drv->trans->iwlReadPRPHNoGrab(SCD_EN_CTRL) | qid);
    
    drv->trans->releaseNICAccess(&state);
    
    return err;
}

int iwl_mvm_add_aux_sta(IWLMvmDriver* drv) {
    iwl_mvm_add_sta_cmd cmd;
    int err;
    uint32_t status;
    
    err = iwl_enable_txq(drv, 1, IWL_MVM_DQA_AUX_QUEUE, IWL_MVM_TX_FIFO_MCAST);
    if(err)
        return err;
    
    memset(&cmd, 0, sizeof(cmd));
    cmd.sta_id = 1;
    cmd.mac_id_n_color =
        htole32(FW_CMD_ID_AND_COLOR(MAC_INDEX_AUX, 0));
    cmd.tfd_queue_msk = htole32(1 << IWL_MVM_DQA_AUX_QUEUE);
    cmd.tid_disable_tx = htole16(0xffff);
    
    status = ADD_STA_SUCCESS;
    err = drv->sendCmdPduStatus(ADD_STA, sizeof(cmd), &cmd, &status);
    if (err == 0 && status != ADD_STA_SUCCESS)
        err = -EIO;
    
    return err;
}
