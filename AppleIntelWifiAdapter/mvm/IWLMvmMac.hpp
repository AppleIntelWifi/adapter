//
//  IWLMvmMac.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/21/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#ifndef APPLEINTELWIFIADAPTER_MVM_IWLMVMMAC_HPP_
#define APPLEINTELWIFIADAPTER_MVM_IWLMVMMAC_HPP_

#include "IWLMvmDriver.hpp"

int iwl_legacy_config_umac_scan(IWLMvmDriver* drv);
int iwl_config_umac_scan(IWLMvmDriver* drv);
int iwl_umac_scan(IWLMvmDriver* drv);
int iwl_lmac_scan(IWLMvmDriver* drv, apple80211_scan_data* req);
int iwl_enable_beacon_filter(IWLMvmDriver* drv);
int iwl_disable_beacon_filter(IWLMvmDriver* drv);
int iwl_mac_ctxt_cmd(IWLMvmDriver* drv, uint32_t action, int assoc);
int iwl_binding_cmd(IWLMvmDriver* drv, uint32_t action);

void iwl_protect_session(IWLMvmDriver* drv, uint32_t duration,
                         uint32_t max_delay);

#endif  // APPLEINTELWIFIADAPTER_MVM_IWLMVMMAC_HPP_
