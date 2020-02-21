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
    struct iwl_scan_config cfg;
    iwl_host_cmd hcmd = {
        .id = iwl_cmd_id(SCAN_CFG_CMD, IWL_ALWAYS_LONG_GROUP, 0),
        .len = { sizeof(cfg), },
        .data = { &cfg, },
        .dataflags = { IWL_HCMD_DFL_NOCOPY }
    };
    
    if(!iwl_mvm_is_reduced_config_scan_supported(drv->m_pDevice))
        return iwl_legacy_config_umac_scan(drv);
    
    memset(&cfg, 0, sizeof(cfg));;
    
    
    cfg.bcast_sta_id = 1;
    cfg.tx_chains = cpu_to_le32(iwl_mvm_get_valid_tx_ant(drv->m_pDevice));
    cfg.rx_chains = cpu_to_le32(iwl_mvm_scan_rx_ant(drv->m_pDevice));
    
    return drv->sendCmd(&hcmd);
}
