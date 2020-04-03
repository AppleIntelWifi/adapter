//
//  IWLMvmMac.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/21/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#ifndef IWLMvmMac_hpp
#define IWLMvmMac_hpp

#include "IWLMvmDriver.hpp"

int iwl_legacy_config_umac_scan(IWLMvmDriver* drv);
int iwl_config_umac_scan(IWLMvmDriver* drv);
int iwl_umac_scan(IWLMvmDriver* drv, apple80211_scan_data* req);
int iwl_lmac_scan(IWLMvmDriver* drv, apple80211_scan_data* req);
int iwl_enable_beacon_filter(IWLMvmDriver* drv);
int iwl_disable_beacon_filter(IWLMvmDriver* drv);

#endif /* IWLMvmMac_h */
