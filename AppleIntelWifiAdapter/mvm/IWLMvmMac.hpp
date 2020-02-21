//
//  IWLMvmMac.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/21/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmMac_hpp
#define IWLMvmMac_hpp

#include "IWLMvmDriver.hpp"

int iwl_legacy_config_umac_scan(IWLMvmDriver* drv);
int iwl_config_umac_scan(IWLMvmDriver* drv);
#endif /* IWLMvmMac_h */
