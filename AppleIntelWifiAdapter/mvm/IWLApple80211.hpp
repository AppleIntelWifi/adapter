//
//  IWLApple80211.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/18/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLApple80211_hpp
#define IWLApple80211_hpp

#include "IWLMvmDriver.hpp"

class IWL80211Device {
public:
    bool init();
    bool release();
    
    bool startScan();
    
    uint8_t address[ETH_ALEN];
    uint32_t flags;
    
    uint32_t n_chans;
    apple80211_channel channels[256];
    apple80211_channel channels_inactive[256];
    apple80211_channel channels_scan[256];
    

private:
    IWLMvmDriver* fDrv;
    
};
#endif /* IWLApple80211_hpp */
