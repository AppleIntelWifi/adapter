//
//  IWLApple80211.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/18/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#ifndef APPLEINTELWIFIADAPTER_MVM_IWLAPPLE80211_HPP_
#define APPLEINTELWIFIADAPTER_MVM_IWLAPPLE80211_HPP_

#include "IWLMvmDriver.hpp"

class IWL80211Device {
 public:
  bool init(IWLMvmDriver* drv);
  bool release();
  bool scanDone();
  bool initChannelMap(size_t channel_map_size);
  void setChannel(size_t ch_idx, apple80211_channel* channel);
  bool initScanChannelMap(size_t channel_map_size);
  bool initScanChannelMap(apple80211_channel* channel_map,
                          size_t channel_map_size);
  void resetScanChannelMap();

  inline uint8_t* getMAC() { return this->address; }

  inline bool setMAC(size_t mac_size, uint8_t* mac) {
    if (mac_size != 6) return false;

    memcpy(this->address, mac, 6);
    return true;
  }

  inline uint32_t getState() { return this->state; }
  void setState(uint32_t new_state) { this->state = new_state; }

  inline uint32_t getFlags() { return this->flags; }

  inline uint8_t* getBSSID() { return reinterpret_cast<uint8_t*>(this->bssid); }

  inline bool getScanning() { return this->scanning; }
  inline void setScanning(bool status) { this->scanning = status; }

  inline bool getPublished() { return this->published; }
  inline void setPublished(bool published) { this->published = published; }

  inline const char* getSSID() { return this->ssid; }

  void setSSID(size_t ssid_len, const char* ssid);

  inline size_t getSSIDLen() {
    if (this->ssid_len > 32) this->ssid_len = 32;

    return this->ssid_len;
  }

  inline apple80211_scan_data* getScanData() { return this->scan_data; }

  inline void setScanData(apple80211_scan_data* scan) {
    this->scan_data = scan;
  }

  inline apple80211_channel getCurrentChannel() {
    if (this->current_channel.channel == 0) {
      return this->channels[0];
    }

    return this->current_channel;
  }

  inline apple80211_channel* getChannelMap() { return this->channels; }

  inline size_t getChannelMapSize() { return this->n_chans; }

  inline apple80211_channel* getScanChannelMap() { return this->channels_scan; }
  inline size_t getScanChannelMapSize() { return this->n_scan_chans; }

  inline OSOrderedSet* getScanCache() { return this->scanCache; }
  inline bool lockScanCache() {
    if (this->scanCacheLock == NULL) return false;

    if (!IOLockTryLock(this->scanCacheLock)) return false;

    this->scanCache->retain();
    return true;
  }
  inline bool unlockScanCache() {
    if (this->scanCacheLock == NULL) return false;

    IOLockUnlock(this->scanCacheLock);

    this->scanCache->release();

    return true;
  }

  inline void resetScanIndex() { this->scan_index = 0; }

  inline uint32_t getScanIndex() { return this->scan_index; }

  inline void setScanIndex(uint32_t new_index) { this->scan_index = new_index; }

 private:
  uint8_t address[ETH_ALEN];
  uint32_t flags;

  bool scanning;
  bool published;

  const char* ssid;
  size_t ssid_len;

  uint8_t bssid[ETH_ALEN];

  uint32_t state;
  size_t n_chans;

  apple80211_scan_data* scan_data;
  apple80211_channel current_channel;
  apple80211_channel* channels;
  apple80211_channel* channels_scan;

  uint32_t n_scan_chans;
  uint32_t scan_max;
  uint32_t scan_index;

  OSOrderedSet* scanCache;
  IOLock* scanCacheLock;

  IO80211Interface* iface;
  IWLMvmDriver* fDrv;
};
#endif  // APPLEINTELWIFIADAPTER_MVM_IWLAPPLE80211_HPP_
