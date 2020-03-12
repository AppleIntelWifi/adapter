//
//  IWLMvmMac.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/20/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmMac.hpp"

int iwl_legacy_config_umac_scan(IWLMvmDriver* drv) {
    IWL_ERR(0, "legacy config not implemented");
    return -1;
}

int iwl_config_umac_scan(IWLMvmDriver* drv) {
    struct iwl_scan_config_v1* cfg;
    int nchan, err;
    size_t len = sizeof(iwl_scan_config_v1) + drv->m_pDevice->fw.ucode_capa.n_scan_channels;
    
    cfg = (iwl_scan_config_v1*)IOMalloc(len);
    
    iwl_host_cmd hcmd = {
        .id = iwl_cmd_id(SCAN_CFG_CMD, IWL_ALWAYS_LONG_GROUP, 0),
        .len = { len, },
        .data = { &cfg, },
        .dataflags = {IWL_HCMD_DFL_DUP},
        .flags = 0
    };
    
    /*
    if(!iwl_mvm_is_reduced_config_scan_supported(drv->m_pDevice))
        return iwl_legacy_config_umac_scan(drv); */
    static const uint32_t rates = (SCAN_CONFIG_RATE_1M |
        SCAN_CONFIG_RATE_2M | SCAN_CONFIG_RATE_5M |
        SCAN_CONFIG_RATE_11M | SCAN_CONFIG_RATE_6M |
        SCAN_CONFIG_RATE_9M | SCAN_CONFIG_RATE_12M |
        SCAN_CONFIG_RATE_18M | SCAN_CONFIG_RATE_24M |
        SCAN_CONFIG_RATE_36M | SCAN_CONFIG_RATE_48M |
        SCAN_CONFIG_RATE_54M);

    
    
    cfg->bcast_sta_id = 1;
    cfg->tx_chains = cpu_to_le32(iwl_mvm_get_valid_tx_ant(drv->m_pDevice));
    cfg->rx_chains = cpu_to_le32(iwl_mvm_scan_rx_ant(drv->m_pDevice));
    cfg->legacy_rates = cpu_to_le32(rates | SCAN_CONFIG_SUPPORTED_RATE(rates));
    
    cfg->dwell.active = 10;
    cfg->dwell.passive = 110;
    cfg->dwell.fragmented = 44;
    cfg->dwell.extended = 90;
    cfg->out_of_channel_time = cpu_to_le32(0);
    cfg->suspend_time = cpu_to_le32(0);
    
    memcpy(&cfg->mac_addr[0], &drv->m_pDevice->nvm_data->hw_addr[0], sizeof(u8) * ETH_ALEN);
    
    cfg->channel_flags = IWL_CHANNEL_FLAG_EBS |
    IWL_CHANNEL_FLAG_ACCURATE_EBS | IWL_CHANNEL_FLAG_EBS_ADD |
    IWL_CHANNEL_FLAG_PRE_SCAN_PASSIVE2ACTIVE;
    
    
    struct ieee80211com *ic = &drv->m_pDevice->ie_ic;
    
    if(!ic) {
        IWL_ERR(0, "Missing ie_ic?\n");
        return -1;
    }
    
    struct ieee80211_channel *c;
    
    nchan = 0;
    
    int num_channels = drv->m_pDevice->fw.ucode_capa.n_scan_channels;
    
    if(drv->m_pDevice->fw.ucode_capa.n_scan_channels > IEEE80211_CHAN_MAX) {
        IWL_ERR(0, "ucode asked for %d channels\n", drv->m_pDevice->fw.ucode_capa.n_scan_channels);
        return -1;
    }
    
    
    for (nchan = 1; nchan < num_channels; nchan++) {
        c = &ic->ic_channels[nchan];
        
        if(!c)
            continue;
        if (c->ic_flags == 0)
            continue;
        
        //IWL_INFO(0, "Adding channel %d to scan", c->hw_value);
        cfg->channel_array[nchan++] =
            ieee80211_mhz2ieee(c->ic_freq, 0);
    }
    
    cfg->flags = cpu_to_le32(SCAN_CONFIG_FLAG_ACTIVATE |
        SCAN_CONFIG_FLAG_ALLOW_CHUB_REQS |
        SCAN_CONFIG_FLAG_SET_TX_CHAINS |
        SCAN_CONFIG_FLAG_SET_RX_CHAINS |
        SCAN_CONFIG_FLAG_SET_AUX_STA_ID |
        SCAN_CONFIG_FLAG_SET_ALL_TIMES |
        SCAN_CONFIG_FLAG_SET_LEGACY_RATES |
        SCAN_CONFIG_FLAG_SET_MAC_ADDR |
        SCAN_CONFIG_FLAG_SET_CHANNEL_FLAGS|
        SCAN_CONFIG_N_CHANNELS(nchan) |
        SCAN_CONFIG_FLAG_CLEAR_FRAGMENTED);
    
        
    err = drv->sendCmd(&hcmd);
    
    
    //drv->trans->freeResp(&hcmd);
    return err;
}

int iwl_disable_beacon_filter(IWLMvmDriver* drv) {
    struct iwl_beacon_filter_cmd cmd;
    int err;
    
    memset(&cmd, 0, sizeof(cmd));
    
    return drv->sendCmdPdu(REPLY_BEACON_FILTERING_CMD, 0, sizeof(cmd), &cmd);
}
