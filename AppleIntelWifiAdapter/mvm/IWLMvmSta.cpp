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

/* send station add/update command to firmware */
int iwl_mvm_sta_send_to_fw(IWLMvmDriver* drv, struct iwm_node *in, bool update, unsigned int flags)
{
    struct ieee80211com *ic = &drv->m_pDevice->ie_ic;
    struct iwl_mvm_add_sta_cmd add_sta_cmd = {
        .sta_id = IWM_STATION_ID,
        .mac_id_n_color = cpu_to_le32(FW_CMD_ID_AND_COLOR(in->in_id, in->in_color)),
        .add_modify = update ? 1 : 0,
        .tid_disable_tx = cpu_to_le16(0xffff),
    };
    add_sta_cmd.station_flags_msk =
    cpu_to_le32(STA_FLG_FAT_EN_MSK | STA_FLG_MIMO_EN_MSK | STA_FLG_RTS_MIMO_PROT);
    int ret;
    u32 status;
    u32 agg_size = 0, mpdu_dens = 0;
    
    if (fw_has_api(&drv->m_pDevice->fw.ucode_capa, IWL_UCODE_TLV_API_STA_TYPE)) {
        //TODO zxy if use tdls, its type is IWL_STA_TDLS_LINK, but we now ignore it.
        //sta->tdls ? IWL_STA_TDLS_LINK : IWL_STA_LINK;
        add_sta_cmd.station_type = IWL_STA_LINK;
    }
    if (!update || (flags & STA_MODIFY_QUEUES)) {
        IEEE80211_ADDR_COPY(&add_sta_cmd.addr, in->in_ni.ni_bssid);
        
        if (!iwl_mvm_has_new_tx_api(drv->m_pDevice)) {
            add_sta_cmd.tfd_queue_msk =
            cpu_to_le32(0);//mvm_sta->tfd_queue_msk = 0;
            
            if (flags & STA_MODIFY_QUEUES)
                add_sta_cmd.modify_mask |= STA_MODIFY_QUEUES;
        } else {
            WARN_ON(flags & STA_MODIFY_QUEUES);
        }
    }
//    switch (sta->bandwidth) {
//    case IEEE80211_STA_RX_BW_160:
//        add_sta_cmd.station_flags |= cpu_to_le32(STA_FLG_FAT_EN_160MHZ);
//        /* fall through */
//    case IEEE80211_STA_RX_BW_80:
//        add_sta_cmd.station_flags |= cpu_to_le32(STA_FLG_FAT_EN_80MHZ);
//        /* fall through */
//    case IEEE80211_STA_RX_BW_40:
//        add_sta_cmd.station_flags |= cpu_to_le32(STA_FLG_FAT_EN_40MHZ);
//        /* fall through */
//    case IEEE80211_STA_RX_BW_20:
//        if (sta->ht_cap.ht_supported)
//            add_sta_cmd.station_flags |=
//                cpu_to_le32(STA_FLG_FAT_EN_20MHZ);
//        break;
//    }
    if (in->in_ni.ni_flags & IEEE80211_NODE_HT) {
        add_sta_cmd.station_flags_msk
            |= htole32(STA_FLG_MAX_AGG_SIZE_MSK |
            STA_FLG_AGG_MPDU_DENS_MSK);

        add_sta_cmd.station_flags
            |= htole32(STA_FLG_MAX_AGG_SIZE_64K);
        switch (ic->ic_ampdu_params & IEEE80211_AMPDU_PARAM_SS) {
        case IEEE80211_AMPDU_PARAM_SS_2:
            add_sta_cmd.station_flags
                |= htole32(STA_FLG_AGG_MPDU_DENS_2US);
            break;
        case IEEE80211_AMPDU_PARAM_SS_4:
            add_sta_cmd.station_flags
                |= htole32(STA_FLG_AGG_MPDU_DENS_4US);
            break;
        case IEEE80211_AMPDU_PARAM_SS_8:
            add_sta_cmd.station_flags
                |= htole32(STA_FLG_AGG_MPDU_DENS_8US);
            break;
        case IEEE80211_AMPDU_PARAM_SS_16:
            add_sta_cmd.station_flags
                |= htole32(STA_FLG_AGG_MPDU_DENS_16US);
            break;
        default:
            break;
        }
    }

    status = ADD_STA_SUCCESS;
    ret = drv->sendCmdPduStatus(ADD_STA, iwl_mvm_add_sta_cmd_size(drv->m_pDevice),
        &add_sta_cmd, &status);
    if (!ret && (status & IWL_ADD_STA_STATUS_MASK) != ADD_STA_SUCCESS)
        ret = EIO;
    
    return ret;
}
