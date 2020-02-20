//
//  IWLMvmSta.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/19/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmSta_hpp
#define IWLMvmSta_hpp

#include "../trans/IWLTransport.hpp"
#include "../mvm/IWLMvmDriver.hpp"

int iwl_mvm_add_aux_sta(IWLMvmDriver* drv);

#endif /* IWLMvmSta_hpp */
