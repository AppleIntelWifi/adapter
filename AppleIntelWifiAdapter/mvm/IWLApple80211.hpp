//
//  IWLApple80211.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/18/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#ifndef APPLEINTELWIFIADAPTER_MVM_IWLAPPLE80211_HPP_
#define APPLEINTELWIFIADAPTER_MVM_IWLAPPLE80211_HPP_

#include "IWLCachedScan.hpp"
#include "IWLMvmDriver.hpp"
#include "IWLNode.hpp"

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

  void saveState() { this->prev_state = this->state; }

  void restoreState() { this->state = this->prev_state; }

  inline uint32_t getFlags() { return this->flags; }

  inline uint8_t* getBSSID() { return reinterpret_cast<uint8_t*>(this->bssid); }
  inline bool setBSSID(uint8_t* bssid, size_t bssid_len) {
    if (bssid_len != 6) return false;

    memcpy(&this->bssid, bssid, 6);
    return true;
  }
  inline void resetBSSID() { memset(&this->bssid, 0, ETH_ALEN); }

  inline bool getScanning() { return this->scanning; }
  inline void setScanning(bool status) { this->scanning = status; }

  inline bool getPublished() { return this->published; }
  inline void setPublished(bool published) { this->published = published; }

  inline const char* getSSID() { return this->ssid; }

  void setSSID(size_t ssid_len, const char* ssid);
  // clang-format off
  inline void resetSSID() {
    if (this->ssid) {
      IOFree((void*)this->ssid,  // NOLINT(readability/casting)
             this->ssid_len + 1);
      this->ssid = NULL;
    }
  }
  // clang-format on
  inline size_t getSSIDLen() {
    if (this->ssid_len > 32) this->ssid_len = 31;

    return this->ssid_len;
  }

  inline apple80211_scan_data* getScanData() { return this->scan_data; }

  inline void setScanData(apple80211_scan_data* scan) {
    this->scan_data = scan;
  }

  inline void resetScanData() { this->scan_data = NULL; }

  inline apple80211_scan_multiple_data* getScanMultipleData() {
    return this->scan_multiple_data;
  }

  inline void setScanMultipleData(apple80211_scan_multiple_data* datum) {
    this->scan_multiple_data = datum;
  }

  inline void resetScanMultipleData() { this->scan_multiple_data = NULL; }

  inline apple80211_channel getCurrentChannel() {
    if (this->current_channel.channel == 0) {
      return this->channels[0];
    }

    return this->current_channel;
  }

  inline bool setCurrentChannel(uint32_t ch_idx) {
    if (!this->channels) return false;

    if (ch_idx == 0) return false;

    for (int i = 0; i < this->n_chans; i++) {
      apple80211_channel ch = this->channels[i];
      if (ch.channel == ch_idx) {
        this->current_channel.channel = ch_idx;
        this->current_channel.flags = ch.flags;
        this->current_channel.version = APPLE80211_VERSION;
        return true;
      }
    }

    return false;
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

  inline void setCipherKey(apple80211_key* cipher_key) {
    if (cipher_key) {
      if (!cipher_key->version) return;

      // clang-format off
      if (this->key) IOFree((void*)this->key, sizeof(apple80211_key)); // NOLINT(readability/casting)
      // clang-format on
      this->key =
          reinterpret_cast<apple80211_key*>(kzalloc(sizeof(apple80211_key)));
      memcpy(this->key, cipher_key, sizeof(apple80211_key));
    }
  }

  inline void resetCipherKey() {
    // clang-format off
    if (this->key) IOFree((void*)this->key, sizeof(apple80211_key)); // NOLINT(readability/casting)
    // clang-format on
  }

  inline apple80211_key* getCipherKey() { return this->key; }

  inline uint8_t* getRSN_IE() { return reinterpret_cast<uint8_t*>(&rsn_ie); }

  inline uint32_t getRSN_IELen() { return this->rsn_ie_len; }

  inline void resetRSN_IE() { memset(&rsn_ie, 0, APPLE80211_MAX_RSN_IE_LEN); }

  inline void setRSN_IE(uint8_t* ie, uint32_t ie_len) {
    if (ie_len > APPLE80211_MAX_RSN_IE_LEN) return;

    memcpy(&rsn_ie, ie, ie_len);
  }

  inline uint32_t getAuthUpper() { return this->auth_upper; }

  inline void setAuthUpper(uint32_t upper) { this->auth_upper = upper; }

  inline uint32_t getAuthLower() { return this->auth_lower; }

  inline void setAuthLower(uint32_t lower) { this->auth_lower = lower; }

  inline uint32_t getAPMode() { return ap_mode; }

  inline void setAPMode(uint32_t mode) { this->ap_mode = mode; }

  inline uint32_t getPhyMode() { return phy_mode; }

  inline void setPhyMode(uint32_t phy) { this->phy_mode = phy; }

  inline uint32_t getOPMode() { return this->op_mode; }

  inline void setOPMode(uint32_t op) { this->op_mode = op; }

  inline IWLNode* getBSS() { return this->bss; }

  inline void resetBSS() {
    if (this->bss) this->bss->release();

    this->bss = NULL;
  }

  inline void setBSS(IWLNode* _bss) {
    if (!_bss) return;

    _bss->retain();
    this->bss = _bss;
  }

  inline IO80211Interface* getInterface() { return iface; }

  void inputFrame(mbuf_t m);

 private:
  uint8_t address[ETH_ALEN];
  uint32_t flags;

  bool scanning;
  bool published;

  const char* ssid;
  size_t ssid_len;

  uint8_t bssid[ETH_ALEN];

  uint32_t prev_state;
  uint32_t state;
  size_t n_chans;

  apple80211_scan_data* scan_data;
  apple80211_scan_multiple_data* scan_multiple_data;
  apple80211_channel current_channel;
  apple80211_channel* channels;
  apple80211_channel* channels_scan;

  uint32_t auth_lower;
  uint32_t auth_upper;
  uint32_t ap_mode;
  uint32_t phy_mode;
  uint32_t op_mode;

  uint32_t n_scan_chans;
  uint32_t scan_max;
  uint32_t scan_index;

  IWLNode* bss;

  OSOrderedSet* scanCache;
  IOLock* scanCacheLock;

  apple80211_key* key;
  uint8_t rsn_ie[APPLE80211_MAX_RSN_IE_LEN];
  uint32_t rsn_ie_len;

  IO80211Interface* iface;
  IWLMvmDriver* fDrv;
};
#endif  // APPLEINTELWIFIADAPTER_MVM_IWLAPPLE80211_HPP_
