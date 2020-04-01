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
    bool init(IWLMvmDriver* drv);
    bool release();
    
    bool scanDone();
    
    uint8_t address[ETH_ALEN];
    uint32_t flags;
    
    bool scanning;
    bool published;
    
    uint32_t state;
    uint32_t n_chans;
    
    apple80211_scan_result scan_results[256];
    apple80211_channel current_channel;
    apple80211_channel channels[256];
    apple80211_channel channels_inactive[256];
    apple80211_channel channels_scan[256];
    uint32_t n_scan_chans;
    uint32_t scan_max;
    uint32_t scan_index;

    OSOrderedSet* scanCache;
    IOLock* scanCacheLock;

private:
    IO80211Interface* iface;
    IWLMvmDriver* fDrv;
    
};
#endif /* IWLApple80211_hpp */
