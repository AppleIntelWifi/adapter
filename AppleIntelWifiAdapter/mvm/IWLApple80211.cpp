//
//  IWLApple80211.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/18/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#include "IWLApple80211.hpp"

bool IWL80211Device::init(IWLMvmDriver* drv) {
  fDrv = drv;

  scanCacheLock = IOLockAlloc();
  scanCache = OSOrderedSet::withCapacity(50);

  iface = fDrv->controller->getNetworkInterface();

  auth_lower = APPLE80211_AUTHTYPE_OPEN;
  ssid = NULL;

  return true;
}

bool IWL80211Device::release() {
  if (scanCacheLock) {
    IOLockFree(scanCacheLock);
    scanCacheLock = NULL;
  }

  if (scanCache) {
    scanCache->release();
    scanCache = NULL;
  }

  return true;
}

bool IWL80211Device::scanDone() {
  if (iface != NULL) {
    // fDrv->m_pDevice->interface->postMessage(APPLE80211_M_SCAN_DONE);
    fDrv->controller->getNetworkInterface()->postMessage(
        APPLE80211_M_SCAN_DONE);
    return true;
  } else {
    return false;
  }
}

bool IWL80211Device::initChannelMap(size_t channel_map_size) {
  if (this->channels != NULL) return false;

  if (channel_map_size > 256) return false;

  this->channels = reinterpret_cast<apple80211_channel*>(
      kzalloc(sizeof(apple80211_channel) * channel_map_size));
  this->n_chans = channel_map_size;

  return (this->channels != NULL);
}

void IWL80211Device::setChannel(size_t ch_idx, apple80211_channel* channel) {
  if (channel == NULL) return;

  if (this->channels == NULL) return;

  if (ch_idx > this->n_chans) return;

  memcpy(&this->channels[ch_idx], channel, sizeof(apple80211_channel));
}

void IWL80211Device::setSSID(size_t ssid_len, const char* ssid) {
  if (ssid_len > 32) ssid_len = 32;

  this->ssid = reinterpret_cast<const char*>(kzalloc(ssid_len + 1));
  if (this->ssid == NULL) return;

  memcpy((void*)this->ssid, ssid, ssid_len);  // NOLINT(readability/casting)
}

bool IWL80211Device::initScanChannelMap(size_t map_size) {
  if (map_size > 256) return false;
  // clang-format off
  if (this->channels_scan)
    IOFree((void*)this->channels_scan, // NOLINT(readability/casting)
           sizeof(apple80211_channel) * this->n_scan_chans);
  // clang-format on

  this->channels_scan = reinterpret_cast<apple80211_channel*>(
      kzalloc(sizeof(apple80211_channel) * map_size));
  this->n_scan_chans = map_size;

  return (this->channels_scan != NULL);
}

bool IWL80211Device::initScanChannelMap(apple80211_channel* channel_map,
                                        size_t map_size) {
  if (map_size > 256) map_size = 256;

  if (!channel_map) return false;

  // clang-format off
  if (this->channels_scan)
    IOFree((void*)this->channels_scan, // NOLINT(readability/casting)
           sizeof(apple80211_channel) * this->n_scan_chans);
  // clang-format on

  this->channels_scan = reinterpret_cast<apple80211_channel*>(
      kzalloc(sizeof(apple80211_channel) * map_size));
  this->n_scan_chans = map_size;

  if (!this->channels_scan) return false;

  memcpy(this->channels_scan, channel_map,
         sizeof(apple80211_channel) * map_size);
  return (this->channels_scan != NULL);
}

void IWL80211Device::resetScanChannelMap() {
  if (!this->channels_scan) return;

  memset(this->channels_scan, 0,
         sizeof(apple80211_channel) * this->n_scan_chans);
}
