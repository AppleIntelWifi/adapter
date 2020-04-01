//
//  IWLCachedScan.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/31/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLCachedScan_hpp
#define IWLCachedScan_hpp

#include <apple80211_var.h>
#include <libkern/c++/OSObject.h>
#include <TransHdr.h>
#include "rx.h"

SInt32 orderCachedScans(const OSMetaClassBase * obj1, const OSMetaClassBase * obj2, void * context);

class IWLCachedScan : public OSObject {
    OSDeclareDefaultStructors(IWLCachedScan)
public:
    bool init(mbuf_t mbuf, int offset, iwl_rx_phy_info* phy_info, int rssi, int noise);
    void free() override;
    
    apple80211_channel getChannel();
    
    uint64_t getTimestamp();
    uint64_t getSysTimestamp(); // stored as absolute time
    
    const char* getSSID();
    uint32_t getSSIDLen();
    uint32_t getRSSI();
    uint32_t getNoise();
    uint16_t getCapabilities();

    uint8_t* getBSSID(); // BSSID len is always 6, no need for the corresponding len getter
    
    uint8_t* getRates();
    //uint32_t getNumRates(); always 8
    
    void* getIE();
    uint32_t getIELen();
    
    apple80211_scan_result* getNativeType();
    
    mbuf_t getMbuf();
private:
    uint8_t* ie;
    uint32_t ie_len;
    
    uint32_t rssi;
    uint32_t noise;
    
    apple80211_channel channel;
    uint64_t absolute_time;
    
    apple80211_scan_result* result; // probably will get upset if we don't store this
    
    struct iwl_rx_phy_info phy_info;
    struct iwl_rx_packet* packet; // as long buf is valid, this is also valid
    mbuf_t buf; // copied in from the ISR
};

#endif /* IWLCachedScan_hpp */
