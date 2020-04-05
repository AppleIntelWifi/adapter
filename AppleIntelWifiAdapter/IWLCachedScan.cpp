//
//  IWLCachedScan.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/31/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#include "IWLCachedScan.hpp"
#include "IWLDebug.h"
#include "apple80211_ioctl.h"
#include <sys/mbuf.h>
#include <sys/kpi_mbuf.h>
#include "endian.h"
#include "rs.h"

#define super OSObject
OSDefineMetaClassAndStructors(IWLCachedScan, OSObject);

#define check_packet() if (!packet) \
{ IWL_ERR(0, "Packet should exist in IWLCachedScan (func: %s)\n", __FUNCTION__); return NULL; }

/*
    The idea is for this class to be as non-copy as possible.
    The point of fixed offsets are to do such
    (but, they are indeed safe as these are fixed offsets defined within the IEEE 802.11 spec)
*/

SInt32 orderCachedScans(const OSMetaClassBase * obj1, const OSMetaClassBase * obj2, void * context) {
    IWLCachedScan* cachedScan_l = OSDynamicCast(IWLCachedScan, obj1);
    if (!cachedScan_l)
        return 0;
    
    IWLCachedScan* cachedScan_r = OSDynamicCast(IWLCachedScan, obj2);
    if (!cachedScan_r)
        return 0;
    
    if (cachedScan_l->getTimestamp() < cachedScan_r->getTimestamp()) // smaller timestamps go before
        return 1;
    else if (cachedScan_l->getTimestamp() == cachedScan_r->getTimestamp())
        return 0;
    else // object 2 is older, put it before
        return -1;
    
}

bool IWLCachedScan::update(iwl_rx_phy_info* phy_info, int rssi, int noise) {
    this->noise = noise;
    this->rssi = rssi;
    
    memcpy(&this->phy_info, phy_info, sizeof(iwl_rx_phy_info)); //necessary
    this->absolute_time = mach_absolute_time();
    
    return true;
}


bool IWLCachedScan::init(mbuf_t mbuf, int offset, int whOffset, iwl_rx_phy_info* phy_info, int rssi, int noise) {
    
    if (!super::init())
        return false;
    
    errno_t err = mbuf_dup(mbuf, MBUF_DONTWAIT, &buf);
    
    if (err != 0) {
        IWL_ERR(0, "mbuf dup complained\n");
        return false;
    }
    
    if (!buf)
        return false;
    
    packet = (iwl_rx_packet*)((u8*)mbuf_data((mbuf_t)buf) + (offset));
    
    if (!packet)
        return false;
    
    wh = (ieee80211_frame*)(packet->data + whOffset);
    iwl_rx_mpdu_res_start* rx_res = (iwl_rx_mpdu_res_start*)packet->data;
    
    this->ie_len = rx_res->byte_count - 36;
    
    if (this->ie_len <= 0)
        return false;
    
    if (le16toh(rx_res->byte_count) <= 36)
        return false;
        
    this->ie = ((uint8_t*)wh + 36);
    
    this->noise = noise;
    this->rssi = rssi;
    
    memcpy(&this->phy_info, phy_info, sizeof(iwl_rx_phy_info)); //necessary
    this->absolute_time = mach_absolute_time();
    
    
    if (this->ie[0] != 0x00) {
        IWL_ERR(0, "potentially uncompliant frame\n");
        IWL_INFO(0, "wh: %x, ie: %x\n", ((uint8_t*)wh)[39], ie[5]);
        IWL_INFO(0, "wh: %x, ie: %x\n", ((uint8_t*)wh)[38], ie[4]); // first byte
        IWL_INFO(0, "wh: %x, ie: %x\n", ((uint8_t*)wh)[37], ie[3]); // len
        IWL_INFO(0, "wh: %x, ie: %x\n", ((uint8_t*)wh)[36], ie[2]); // indicator
        IWL_INFO(0, "wh: %x, ie: %x\n", ((uint8_t*)wh)[35], ie[1]);
        IWL_INFO(0, "wh: %x, ie: %x\n", ((uint8_t*)wh)[34], ie[0]);
        IWL_INFO(0, "wh: %x\n", ((uint8_t*)wh)[34]);
        IWL_INFO(0, "wh: %x\n", ((uint8_t*)wh)[33]);
        IWL_INFO(0, "wh: %x\n", ((uint8_t*)wh)[32]);
        IWL_INFO(0, "wh: %x\n", ((uint8_t*)wh)[31]);
        IWL_INFO(0, "wh: %x\n", ((uint8_t*)wh)[30]);
        IWL_INFO(0, "wh: %x\n", ((uint8_t*)wh)[29]);
        return false;
    }
    
       
       channel.version = APPLE80211_VERSION;
       channel.channel = le16toh(this->phy_info.channel);
       

       /*
       if(this->phy_info.phy_flags & RX_RES_PHY_FLAGS_BAND_24) {
           channel.flags |= APPLE80211_C_FLAG_2GHZ;
       } else {
           channel.flags |= APPLE80211_C_FLAG_5GHZ;
       }
        */
    
        if (channel.channel < 15)
            channel.flags |= APPLE80211_C_FLAG_2GHZ;
        else
            channel.flags |= APPLE80211_C_FLAG_5GHZ;
        
    
       switch (this->phy_info.rate_n_flags & RATE_MCS_CHAN_WIDTH_MSK) {
           case RATE_MCS_CHAN_WIDTH_20:
               IWL_INFO(0, "Chan width 20mhz\n");
               channel.flags |= APPLE80211_C_FLAG_20MHZ;
               break;
           
           case RATE_MCS_CHAN_WIDTH_40:
               IWL_INFO(0, "Chan width 40mhz\n");
               channel.flags |= APPLE80211_C_FLAG_40MHZ;
               break;
               
           case RATE_MCS_CHAN_WIDTH_80:
               IWL_INFO(0, "Chan width 80mhz\n");
               channel.flags |= APPLE80211_C_FLAG_EXT_ABV | APPLE80211_C_FLAG_40MHZ;
               break;
           
           case RATE_MCS_CHAN_WIDTH_160:
               IWL_INFO(0, "Chan width 160mhz\n");
               channel.flags |= 0x400;
               break;
       }
    
    
    
    return true;
}

void IWLCachedScan::free() {
    super::free();
    
    if (buf) {
        mbuf_freem(buf);
    }
    packet = NULL;
}

apple80211_channel IWLCachedScan::getChannel() {
    return channel;
}

uint64_t IWLCachedScan::getTimestamp() {
    return phy_info.timestamp;
}

uint64_t IWLCachedScan::getSysTimestamp() {
    return absolute_time;
}

const char* IWLCachedScan::getSSID() { // ensure to free this resulting buffer
    check_packet()
    
    if (ie[0] != 0x00) {
        IWL_ERR(0, "haven't handled this yet\n");
        return NULL;
    }
    
    uint8_t ssid_len = getSSIDLen();
    
    const char* ssid = (const char*)kzalloc(ssid_len + 1); // include null terminator
    
    if (!ssid)
        return NULL;
    
    memcpy((void*)ssid, &ie[2], ssid_len); // 0x00 == type, 0x01 == size, 0x02 onwards == data
    
    return ssid;
}

uint32_t IWLCachedScan::getSSIDLen() {
    check_packet()
    
    if (ie[0] != 0x00) {
        IWL_ERR(0, "haven't handled this yet\n");
        return NULL;
    }
    
    uint8_t ssid_len = ie[1];
    
    if (ssid_len > 32) {
        IWL_ERR(0, "ssid length too large, clamping to 32\n");
        ssid_len = 32;
    }
    
    return ssid_len;
}

uint32_t IWLCachedScan::getRSSI() {
    return rssi;
}

uint32_t IWLCachedScan::getNoise() {
    return noise;
}

uint16_t IWLCachedScan::getCapabilities() {
    check_packet()
    
    return (*((uint8_t*)wh + 35) << 8) | (*((uint8_t*)wh + 34));
    // these are stored in the fixed parameters, offsets are fine here
}

uint8_t* IWLCachedScan::getBSSID() {
    check_packet()
    
    return &wh->i_addr3[0];
}

uint8_t* IWLCachedScan::getRates() {
    check_packet()
    
    uint8_t* rate_ptr;
    
    size_t index = 0;
    if (ie[index++] == 0x00) { // index is 1 when we enter in the frame
        index += ie[1] + 1; // increment by size of ssid AND skip over length byte
    } else {
        IWL_ERR(0, "still can't handle this\n");
        return NULL;
    }
    
    if (index >= ie_len) {
        IWL_ERR(0, "tried to access too far within IE, buffer overrun\n");
        return NULL;
    }
    
    if (ie[index++] == 0x01) {
        //good, we found it!
        uint32_t rate_size = ie[index++];
        if (rate_size != 8) {
            IWL_ERR(0, "rate set is NOT correct\n");
            return NULL;
        }
        
        rate_ptr = &ie[index];
    } else {
        IWL_ERR(0, "found ssid, but no rateset\n");
        return NULL;
    }
    
    return rate_ptr;
}

void* IWLCachedScan::getIE() {
    check_packet()
    
    return (void*)ie;
}

uint32_t IWLCachedScan::getIELen() {
    return ie_len;
}

apple80211_scan_result* IWLCachedScan::getNativeType() { // be sure to free this too
    check_packet()
    
    result = (apple80211_scan_result*)kzalloc(sizeof(apple80211_scan_result));
    
    if (result == NULL)
        return NULL;

    result->version = APPLE80211_VERSION;
    
    //uint64_t nanosecs;
    //absolutetime_to_nanoseconds(this->getSysTimestamp(), &nanosecs);
    //result->asr_age = (nanosecs * (__int128)0x431BDE82D7B634DBuLL >> 64) >> 18; // MAGIC compiler division..
                                                                                // I don't get this
    
    //result->asr_age = le32toh(this->phy_info.system_timestamp);
    result->asr_ie_len = this->getIELen();
    
    IWL_INFO(0, "IE length: %d\n", result->asr_ie_len);
    if (result->asr_ie_len != 0) {
        result->asr_ie_data = ie;
        
        //uint8_t* buf = (uint8_t*)result->asr_ie_data;
        //size_t out_sz = 0;
        //uint8_t* encode = base64_encode(buf, (size_t)result->asr_ie_len, &out_sz);
        //IWL_INFO(0, "IE: %s", encode);
    }
    
    result->asr_beacon_int = 100;
    
    uint8_t* rates = this->getRates();
    
    if (rates != NULL) {
        for (int i = 0; i < 8; i++) {
            result->asr_rates[i] = (rates[i] >> 1) & 0x3F; // stolen magic
        }
        result->asr_nrates = 8;
    }
    
    result->asr_cap = this->getCapabilities();
    
    result->asr_channel.channel = this->getChannel().channel;
    result->asr_channel.flags = this->getChannel().flags;
    result->asr_channel.version = 1;
    
    result->asr_noise = this->getNoise();
    result->asr_rssi = this->getRSSI();
    
    memcpy(&result->asr_bssid, this->getBSSID(), 6);
    
    result->asr_ssid_len = this->getSSIDLen();
    
    if (this->getSSIDLen() != 0) {
        
        const char* ssid = this->getSSID();
        
        if (ssid) {
            memcpy(&result->asr_ssid, ssid, this->getSSIDLen() + 1);
        
            IOFree((void*)ssid, this->getSSIDLen() + 1);
        }
    }
    
    //result->asr_age = le32toh(this->phy_info.system_timestamp);
    return result;
}

mbuf_t IWLCachedScan::getMbuf() {
    return buf;
}

