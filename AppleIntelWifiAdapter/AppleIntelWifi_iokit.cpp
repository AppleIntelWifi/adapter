//
//  AppleIntelWifi_iokit.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/12/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#include "AppleIntelWifiAdapterV2.hpp"
#include "IWLApple80211.hpp"
#include "IWLCachedScan.hpp"
#include "IWLMvmMac.hpp"
#include "apple80211/ioctl_dbg.h"

const char *fake_hw_version = "Hardware 1.0";
const char *fake_drv_version = "Driver 1.0";
const char *country_code = "US";

SInt32 AppleIntelWifiAdapterV2::apple80211Request(unsigned int request_type,
                                                  int request_number,
                                                  IO80211Interface *interface,
                                                  void *data) {
  if (request_type != SIOCGA80211 && request_type != SIOCSA80211) {
    IWL_ERR(0, "Invalid IOCTL request type: %u", request_type);
    IWL_ERR(0, "Expected either %lu or %lu", SIOCGA80211, SIOCSA80211);
    return kIOReturnError;
  }

  IOReturn ret = 0;
  bool isGet = (request_type == SIOCGA80211);
#define IOCTL(REQ_TYPE, REQ, DATA_TYPE)                  \
  if (REQ_TYPE == SIOCGA80211) {                         \
    ret = get##REQ(interface, (struct DATA_TYPE *)data); \
  } else {                                               \
    ret = set##REQ(interface, (struct DATA_TYPE *)data); \
  }

#define IOCTL_GET(REQ_TYPE, REQ, DATA_TYPE)              \
  if (REQ_TYPE == SIOCGA80211) {                         \
    ret = get##REQ(interface, (struct DATA_TYPE *)data); \
  }

#define IOCTL_SET(REQ_TYPE, REQ, DATA_TYPE)              \
  if (REQ_TYPE == SIOCSA80211) {                         \
    ret = set##REQ(interface, (struct DATA_TYPE *)data); \
  }

  IWL_INFO(0, "IOCTL %s(%d) %s\n", isGet ? "get" : "set", request_number,
           IOCTL_NAMES[request_number]);

  switch (request_number) {
    case APPLE80211_IOC_SSID:  // 1
      IOCTL(request_type, SSID, apple80211_ssid_data);
      break;
    case APPLE80211_IOC_AUTH_TYPE:  // 2
      IOCTL_GET(request_type, AUTH_TYPE, apple80211_authtype_data);
      break;
    case APPLE80211_IOC_CHANNEL:  // 4
      IOCTL_GET(request_type, CHANNEL, apple80211_channel_data);
      break;
    case APPLE80211_IOC_PROTMODE:
      IOCTL(request_type, PROTMODE, apple80211_protmode_data);
      break;
    case APPLE80211_IOC_TXPOWER:  // 7
      IOCTL_GET(request_type, TXPOWER, apple80211_txpower_data);
      break;
    case APPLE80211_IOC_RATE:  // 8
      IOCTL_GET(request_type, RATE, apple80211_rate_data);
      break;
    case APPLE80211_IOC_BSSID:  // 9
      IOCTL_GET(request_type, BSSID, apple80211_bssid_data);
      break;
    case APPLE80211_IOC_SCAN_REQ:  // 10
      IOCTL_SET(request_type, SCAN_REQ, apple80211_scan_data);
      break;
    case APPLE80211_IOC_SCAN_RESULT:  // 11
      IOCTL_GET(request_type, SCAN_RESULT, apple80211_scan_result *);
      break;
    case APPLE80211_IOC_CARD_CAPABILITIES:  // 12
      IOCTL_GET(request_type, CARD_CAPABILITIES, apple80211_capability_data);
      break;
    case APPLE80211_IOC_STATE:  // 13
      IOCTL_GET(request_type, STATE, apple80211_state_data);
      break;
    case APPLE80211_IOC_PHY_MODE:  // 14
      IOCTL_GET(request_type, PHY_MODE, apple80211_phymode_data);
      break;
    case APPLE80211_IOC_OP_MODE:  // 15
      IOCTL_GET(request_type, OP_MODE, apple80211_opmode_data);
      break;
    case APPLE80211_IOC_RSSI:  // 16
      IOCTL_GET(request_type, RSSI, apple80211_rssi_data);
      break;
    case APPLE80211_IOC_NOISE:  // 17
      IOCTL_GET(request_type, NOISE, apple80211_noise_data);
      break;
    case APPLE80211_IOC_INT_MIT:  // 18
      IOCTL_GET(request_type, INT_MIT, apple80211_intmit_data);
      break;
    case APPLE80211_IOC_POWER:  // 19
      IOCTL(request_type, POWER, apple80211_power_data);
      break;
    case APPLE80211_IOC_ASSOCIATE:  // 20
      IOCTL_SET(request_type, ASSOCIATE, apple80211_assoc_data);
      break;
    case APPLE80211_IOC_DISASSOCIATE:
      ret = setDISASSOCIATE(interface);
      break;
    case APPLE80211_IOC_SUPPORTED_CHANNELS:  // 27
      IOCTL_GET(request_type, SUPPORTED_CHANNELS, apple80211_sup_channel_data);
      break;
    case APPLE80211_IOC_LOCALE:  // 28
      IOCTL_GET(request_type, LOCALE, apple80211_locale_data);
      break;
    case APPLE80211_IOC_TX_ANTENNA:  // 37
      IOCTL_GET(request_type, TX_ANTENNA, apple80211_antenna_data);
      break;
    case APPLE80211_IOC_ANTENNA_DIVERSITY:  // 39
      IOCTL_GET(request_type, ANTENNA_DIVERSITY, apple80211_antenna_data);
      break;
    case APPLE80211_IOC_DRIVER_VERSION:  // 43
      IOCTL_GET(request_type, DRIVER_VERSION, apple80211_version_data);
      break;
    case APPLE80211_IOC_HARDWARE_VERSION:  // 44
      IOCTL_GET(request_type, HARDWARE_VERSION, apple80211_version_data);
      break;
    case APPLE80211_IOC_ASSOCIATION_STATUS:  // 50
      IOCTL_GET(request_type, ASSOCIATION_STATUS, apple80211_assoc_status_data);
      break;
    case APPLE80211_IOC_COUNTRY_CODE:  // 51
      IOCTL_GET(request_type, COUNTRY_CODE, apple80211_country_code_data);
      break;
    case APPLE80211_IOC_RADIO_INFO:
      IOCTL_GET(request_type, RADIO_INFO, apple80211_radio_info_data);
      break;
    case APPLE80211_IOC_MCS:  // 57
      IOCTL_GET(request_type, MCS, apple80211_mcs_data);
      break;
    case APPLE80211_IOC_WOW_PARAMETERS:  // 69
      break;
    case APPLE80211_IOC_ROAM_THRESH:
      IOCTL_GET(request_type, ROAM_THRESH, apple80211_roam_threshold_data);
      break;
    case APPLE80211_IOC_TX_CHAIN_POWER:  // 108
      break;
    case APPLE80211_IOC_THERMAL_THROTTLING:  // 111
      break;
    case APPLE80211_IOC_POWERSAVE:
      break;
    case APPLE80211_IOC_IE:
      ret = 0;
      break;
    case 353:
      ret = 6;
      break;
    case APPLE80211_IOC_SCANCACHE_CLEAR:
      ret = setSCANCACHE_CLEAR(interface);
      break;
    default:
      IWL_ERR(0, "Unhandled IOCTL %s (%d)\n", IOCTL_NAMES[request_number],
              request_number);
      ret = kIOReturnError;
      break;
  }

  return ret;
}

IOReturn AppleIntelWifiAdapterV2::getSSID(IO80211Interface *interface,
                                          struct apple80211_ssid_data *sd) {
  if (drv->m_pDevice->ie_dev->getState() == APPLE80211_S_INIT ||
      drv->m_pDevice->ie_dev->getState() == APPLE80211_S_SCAN) {
    return APPLE80211_REASON_NOT_AUTHED;
  }

  bzero(sd, sizeof(*sd));
  sd->version = APPLE80211_VERSION;
  uint32_t ssid_len = drv->m_pDevice->ie_dev->getSSIDLen();
  const char *ssid = drv->m_pDevice->ie_dev->getSSID();

  if (!ssid) {
    sd->ssid_len = ssid_len;
    memcpy(&sd->ssid_bytes, drv->m_pDevice->ie_dev->getSSID(),
           drv->m_pDevice->ie_dev->getSSIDLen());
  } else {
    sd->ssid_len = 0;
  }
  // strncpy((char*)sd->ssid_bytes, fake_ssid, sizeof(sd->ssid_bytes));
  // sd->ssid_len = (uint32_t)strlen(fake_ssid);

  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setSSID(IO80211Interface *interface,
                                          struct apple80211_ssid_data *sd) {
  drv->m_pDevice->ie_dev->setState(APPLE80211_S_AUTH);
  interface->postMessage(APPLE80211_M_SSID_CHANGED);

  drv->m_pDevice->ie_dev->setSSID(
      sd->ssid_len, reinterpret_cast<const char *>(&sd->ssid_bytes));

  return kIOReturnSuccess;
}

//
// MARK: 2 - AUTH_TYPE
//

IOReturn AppleIntelWifiAdapterV2::getAUTH_TYPE(
    IO80211Interface *interface, struct apple80211_authtype_data *ad) {
  ad->version = APPLE80211_VERSION;

  ad->authtype_lower = drv->m_pDevice->ie_dev->getAuthLower();
  ad->authtype_upper = drv->m_pDevice->ie_dev->getAuthUpper();

  return kIOReturnSuccess;
}

//
// MARK: 4 - CHANNEL
//

IOReturn AppleIntelWifiAdapterV2::getCHANNEL(
    IO80211Interface *interface, struct apple80211_channel_data *cd) {
  memset(cd, 0, sizeof(apple80211_channel_data));
  cd->version = APPLE80211_VERSION;
  cd->channel = drv->m_pDevice->ie_dev->getCurrentChannel();
  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::getPROTMODE(
    IO80211Interface *interface, struct apple80211_protmode_data *pd) {
  if (drv->m_pDevice->ie_dev->getState() == APPLE80211_S_INIT ||
      drv->m_pDevice->ie_dev->getState() == APPLE80211_S_SCAN) {
    return APPLE80211_REASON_NOT_AUTHED;
  }

  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setPROTMODE(
    IO80211Interface *interface, struct apple80211_protmode_data *pd) {
  if (drv->m_pDevice->ie_dev->getState() == APPLE80211_S_INIT ||
      drv->m_pDevice->ie_dev->getState() == APPLE80211_S_SCAN) {
    return APPLE80211_REASON_NOT_AUTHED;
  }

  return kIOReturnSuccess;
}

//
// MARK: 7 - TXPOWER
//

IOReturn AppleIntelWifiAdapterV2::getTXPOWER(
    IO80211Interface *interface, struct apple80211_txpower_data *txd) {
  txd->version = APPLE80211_VERSION;
  txd->txpower = 100;
  txd->txpower_unit = APPLE80211_UNIT_PERCENT;
  return kIOReturnSuccess;
}

//
// MARK: 8 - RATE
//

IOReturn AppleIntelWifiAdapterV2::getRATE(IO80211Interface *interface,
                                          struct apple80211_rate_data *rd) {
  if (drv->m_pDevice->ie_dev->getState() == APPLE80211_S_INIT ||
      drv->m_pDevice->ie_dev->getState() == APPLE80211_S_SCAN) {
    return APPLE80211_REASON_NOT_AUTHED;
  }

  rd->version = APPLE80211_VERSION;
  rd->num_radios = 1;
  rd->rate[0] = 54;
  return kIOReturnSuccess;
}

//
// MARK: 9 - BSSID
//

IOReturn AppleIntelWifiAdapterV2::getBSSID(IO80211Interface *interface,
                                           struct apple80211_bssid_data *bd) {
  if (drv->m_pDevice->ie_dev->getState() == APPLE80211_S_INIT ||
      drv->m_pDevice->ie_dev->getState() == APPLE80211_S_SCAN) {
    return APPLE80211_REASON_NOT_AUTHED;
  }

  bzero(bd, sizeof(*bd));
  bd->version = APPLE80211_VERSION;
  memcpy(bd->bssid.octet, drv->m_pDevice->ie_dev->getBSSID(), ETH_ALEN);

  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::scanAction(OSObject *target, void *arg0,
                                             void *arg1, void *arg2,
                                             void *arg3) {
  IOReturn ret = kIOReturnSuccess;

  AppleIntelWifiAdapterV2 *that =
      reinterpret_cast<AppleIntelWifiAdapterV2 *>(target);
  IO80211Interface *iface = reinterpret_cast<IO80211Interface *>(arg0);
  IWLMvmDriver *dev = reinterpret_cast<IWLMvmDriver *>(that->drv);
  apple80211_scan_data *sd = reinterpret_cast<apple80211_scan_data *>(arg1);

  dev->m_pDevice->ie_dev->setScanning(true);
  for (int i = 0; i < sd->num_channels; i++) {
    IWL_INFO(0, "%d: ch %d\n", i, sd->channels[i].channel);
  }

  // IWL_INFO(0, "device: %s\n", dev->m_pDevice->cfg->name);
  IWL_INFO(0, "scanning (ptr: %x, dev_ptr: %x, dev: %s, umac: %s)\n", dev,
           dev->m_pDevice, dev->m_pDevice->name,
           dev->m_pDevice->umac_scanning ? "yes" : "no");

  if (dev->m_pDevice->ie_dev->getScanCache() == NULL)
    IWL_ERR(0, "scan cache was null?\n");

#ifdef notyet
  if (fw_has_capa(&dev->m_pDevice->fw.ucode_capa,
                  IWL_UCODE_TLV_CAPA_UMAC_SCAN)) {
#else
  if (1) {
#endif
    if (iwl_umac_scan(dev, sd) != 0) {
      IWL_ERR(0, "umac scan failed\n");
      ret = kIOReturnError;
    }
  } else {
    // IWL_ERR(0, "lmac scanning not implemented yet (device: %s)\n",
    // &dev->m_pDevice->name);
    if (iwl_lmac_scan(dev, sd) != 0) {
      IWL_ERR(0, "lmac scan failed\n");
      ret = kIOReturnError;
    }
  }
  // IOSleep(4000);
  // IOFree(sd, sizeof(apple80211_scan_data));

  return ret;
}

//
// MARK: 10 - SCAN_REQ
//

IOReturn AppleIntelWifiAdapterV2::setSCAN_REQ(IO80211Interface *interface,
                                              struct apple80211_scan_data *sd) {
  if (drv->m_pDevice->ie_dev->getScanning()) {
    IWL_ERR(0, "Denying scan because device is already scanning\n");
    return 0xe0820446;
  }

  /*
  if(drv->m_pDevice->ie_dev->scan_max != 0) {
      for(int k = 0; k < drv->m_pDevice->ie_dev->scan_max; k++) {
          IWL_INFO(0, "Freeing old scan IE data\n");
          //IOFree(drv->m_pDevice->ie_dev->scan_results[k].asr_ie_data,
          //    drv->m_pDevice->ie_dev->scan_results[k].asr_ie_len);
          memset(&drv->m_pDevice->ie_dev->scan_results[k], 0,
  sizeof(apple80211_scan_result));
      }
  }
  */

  drv->m_pDevice->ie_dev->setState(APPLE80211_S_SCAN);
  IWL_INFO(0,
           "Apple80211. Scan requested. Type: %u\n"
           "BSS Type: %u\n"
           "PHY Mode: %u\n"
           "Dwell time: %u\n"
           "Rest time: %u\n"
           "Num channels: %u\n",
           sd->scan_type, sd->bss_type, sd->phy_mode, sd->dwell_time,
           sd->rest_time, sd->num_channels);

  if (sd->num_channels == 0 && sd->ssid_len == 0) {
    apple80211_channel *channel_map = drv->m_pDevice->ie_dev->getChannelMap();

    if (!channel_map) return kIOReturnError;

    if (!drv->m_pDevice->ie_dev->initScanChannelMap(
            channel_map, drv->m_pDevice->fw.ucode_capa.n_scan_channels))
      return kIOReturnError;

  } else {
    if (!drv->m_pDevice->ie_dev->initScanChannelMap(sd->channels,
                                                    sd->num_channels))
      return kIOReturnError;
  }

  drv->m_pDevice->ie_dev->resetScanIndex();

  if (sd->scan_type == 3) {
    interface->postMessage(APPLE80211_M_SCAN_DONE);
    return kIOReturnSuccess;
  }

  if (interface) {
    apple80211_scan_data *request = reinterpret_cast<apple80211_scan_data *>(
        IOMalloc(sizeof(apple80211_scan_data)));
    memcpy(request, sd, sizeof(apple80211_scan_data));

    drv->m_pDevice->ie_dev->setScanData(request);

    IOReturn ret = getCommandGate()->runAction(&scanAction, interface, request);
    return ret;
  }

  return kIOReturnError;
}

//
// MARK: 11 - SCAN_RESULT
//

/*
const char beacon_ie[] =
"\x00\x0a\x55\x50\x43\x35\x34\x32\x34\x32\x39\x37" \ // ssid
"\x01\x08\x82\x84\x8b\x96\x0c\x12\x18\x24" \ // supported rates
"\x03\x01\x01" \ // current channel
"\x05\x04\x00\x01\x00\x00" \ // Traffic Indiciation Map
"\x07\x06\x43\x5a\x20\x01\x0d\x14" \ // country info
"\x2a\x01\x04" \ // ERP info
"\x32\x04\x30\x48\x60\x6c" \ // extended supported rates
"\x2d\x1a\xad\x01\x1b\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x06\xe6\xe7\x0d\x00"
\
// HT capas
"\x3d\x16\x01\x00\x17\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
\ // HT info
"\x4a\x0e\x14\x00\x0a\x00\x2c\x01\xc8\x00\x14\x00\x05\x00\x19\x00"\
"\x7f\x01\x01" \
"\xdd\x18\x00\x50\xf2\x02\x01\x01\x80\x00\x03\xa4" \
"\x00\x00\x27\xa4\x00\x00\x42\x43\x5e\x00\x62\x32\x2f\x00\xdd\x09" \
"\x00\x03\x7f\x01\x01\x00\x00\xff\x7f\x30\x18\x01\x00\x00\x0f\xac" \
"\x02\x02\x00\x00\x0f\xac\x04\x00\x0f\xac\x02\x01\x00\x00\x0f\xac" \
"\x02\x00\x00\xdd\x1a\x00\x50\xf2\x01\x01\x00\x00\x50\xf2\x02\x02" \
"\x00\x00\x50\xf2\x04\x00\x50\xf2\x02\x01\x00\x00\x50\xf2\x02\xdd" \
"\x22\x00\x50\xf2\x04\x10\x4a\x00\x01\x10\x10\x44\x00\x01\x02\x10" \
"\x57\x00\x01\x01\x10\x3c\x00\x01\x01\x10\x49\x00\x06\x00\x37\x2a" \
"\x00\x01\x20";
 */

const char *fake_ssid = "UPC5424297";

IOReturn AppleIntelWifiAdapterV2::getSCAN_RESULT(
    IO80211Interface *interface, struct apple80211_scan_result **sr) {
  OSOrderedSet *scanCache = drv->m_pDevice->ie_dev->getScanCache();

  if (!scanCache) return 5;

  IWL_DEBUG(0, "Locking mutex\n");

  if (!drv->m_pDevice->ie_dev->lockScanCache()) {
    IWL_INFO(0, "Could not lock mutex\n");
    return -1;
  }

  OSObject *obj = NULL;

  apple80211_scan_data *scan_data = drv->m_pDevice->ie_dev->getScanData();

  if (!scan_data) {
    drv->m_pDevice->ie_dev->unlockScanCache();
    return 5;
  }

  if (scan_data->ssid_len != 0) {
    for (int i = drv->m_pDevice->ie_dev->getScanIndex();
         i < scanCache->getCount(); i++) {
      OSObject *_can = scanCache->getObject(i);
      if (_can == NULL) continue;

      IWLCachedScan *candidate = OSDynamicCast(IWLCachedScan, _can);
      if (candidate == NULL) continue;

      if (candidate->getSSIDLen() == scan_data->ssid_len) {
        const char *candidate_ssid = candidate->getSSID();
        if (memcmp(candidate_ssid, scan_data->ssid, candidate->getSSIDLen()) ==
            0) {
          if (scan_data->num_channels != 0) {
            for (int chan = 0; chan < scan_data->num_channels; chan++) {
              if (scan_data->channels[chan].channel ==
                  candidate->getChannel().channel) {
                IWL_INFO(0, "Scan result (SSID: %s, channel: %d, RSSI: %d)\n",
                         candidate_ssid, candidate->getChannel().channel,
                         candidate->getRSSI());

                // clang-format off
                IOFree((void *)candidate_ssid, candidate->getSSIDLen() + 1);  // NOLINT(readability/casting)
                // clang-format on
                *sr = candidate->getNativeType();
                drv->m_pDevice->ie_dev->setScanIndex(i + 1);
                drv->m_pDevice->ie_dev->unlockScanCache();
                return kIOReturnSuccess;
              }
            }
          } else {
            IWL_INFO(0, "Scan result (SSID: %s, channel: %d, RSSI: %d)\n",
                     candidate_ssid, candidate->getChannel().channel,
                     candidate->getRSSI());
            // clang-format off
            IOFree((void *)candidate_ssid, candidate->getSSIDLen() + 1);  // NOLINT(readability/casting)
            // clang-format on
            *sr = candidate->getNativeType();
            drv->m_pDevice->ie_dev->setScanIndex(i + 1);
            drv->m_pDevice->ie_dev->unlockScanCache();
            return kIOReturnSuccess;
          }
        }
      }
    }
    IOFree(scan_data, sizeof(apple80211_scan_data));
    drv->m_pDevice->ie_dev->setScanData(NULL);
    drv->m_pDevice->ie_dev->unlockScanCache();
    return 5;
  } else {
    int scan_index = drv->m_pDevice->ie_dev->getScanIndex();
    obj = scanCache->getObject(scan_index);
    drv->m_pDevice->ie_dev->setScanIndex(scan_index + 1);
  }

  if (obj == NULL) {
    IWL_INFO(0, "Reached end of scan\n");
    IOFree(scan_data, sizeof(apple80211_scan_data));
    drv->m_pDevice->ie_dev->setScanData(NULL);
    drv->m_pDevice->ie_dev->unlockScanCache();
    return 5;
  }

  IWLCachedScan *scan = OSDynamicCast(IWLCachedScan, obj);
  if (!scan) {
    IWL_INFO(0, "Could not cast OSObject to cached scan...\n");
    drv->m_pDevice->ie_dev->unlockScanCache();
    return -1;
  }

  apple80211_scan_result *result = scan->getNativeType();
  if (result != NULL) {
    *sr = result;  // valid until the next scan request, cannot guarantee
                   // whether IE will remain
    IWL_INFO(0, "Scan result (SSID: %s, channel: %d, RSSI: %d)\n",
             result->asr_ssid, result->asr_channel.channel, result->asr_rssi);

  } else {
    IWL_ERR(0, "Scan result was bad\n");
  }

  drv->m_pDevice->ie_dev->unlockScanCache();

  return kIOReturnSuccess;
}

//
// MARK: 12 - CARD_CAPABILITIES
//

IOReturn AppleIntelWifiAdapterV2::getCARD_CAPABILITIES(
    IO80211Interface *interface, struct apple80211_capability_data *cd) {
  cd->version = APPLE80211_VERSION;

  /// ic->ic_caps =
  /// IEEE80211_C_WEP |        /* WEP */
  /// IEEE80211_C_RSN |        /* WPA/RSN */
  /// IEEE80211_C_SCANALL |    /* device scans all channels at once */
  /// IEEE80211_C_SCANALLBAND |    /* device scans all bands at once */
  /// IEEE80211_C_SHSLOT |    /* short slot time supported */
  /// IEEE80211_C_SHPREAMBLE;    /* short preamble supported */

  uint32_t caps = 0;

  cd->capabilities[0] = 0xeb;
  cd->capabilities[1] = 0x7e;
  // cd->capabilities[5] |= 4;
  cd->capabilities[2] |= 0xC0;
  cd->capabilities[6] |= 0x84;
  /* Airport-related flags
  cd->capabilities[3] |= 8;
  cd->capabilities[6] |= 1;
  cd->capabilities[5] |= 0x80;
  */

  /*
   Needs further documentation,
   as setting all of these flags enables AirDrop + IO80211VirtualInterface

  cd->capabilities[2] = 3;
  cd->capabilities[2] |= 0x13;
  cd->capabilities[2] |= 0x20;
  cd->capabilities[2] |= 0x28;
  cd->capabilities[2] |= 4;
  cd->capabilities[5] |= 8;
  cd->capabilities[3] |= 2;
  cd->capabilities[4] |= 1;
  cd->capabilities[6] |= 8;

  cd->capabilities[3] |= 0x23;
  cd->capabilities[2] |= 0x80;
  cd->capabilities[5] |= 4;
  cd->capabilities[2] |= 0xC0;
  cd->capabilities[6] |= 0x84;
  cd->capabilities[3] |= 8;
  cd->capabilities[6] |= 1;
  cd->capabilities[5] |= 0x80;

  */

  /*
  cd->capabilities[0] = (caps & 0xff);
  cd->capabilities[1] = (caps & 0xff00) >> 8;
  cd->capabilities[2] = (caps & 0xff0000) >> 16;
  */

  return kIOReturnSuccess;
}

//
// MARK: 13 - STATE
//

IOReturn AppleIntelWifiAdapterV2::getSTATE(IO80211Interface *interface,
                                           struct apple80211_state_data *sd) {
  sd->version = APPLE80211_VERSION;
  sd->state = drv->m_pDevice->ie_dev->getState();
  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setSTATE(IO80211Interface *interface,
                                           struct apple80211_state_data *sd) {
  IWL_INFO(0, "Setting state: %u\n", sd->state);
  drv->m_pDevice->ie_dev->setState(sd->state);
  return kIOReturnSuccess;
}

//
// MARK: 14 - PHY_MODE
//

IOReturn AppleIntelWifiAdapterV2::getPHY_MODE(
    IO80211Interface *interface, struct apple80211_phymode_data *pd) {
  pd->version = APPLE80211_VERSION;
  pd->phy_mode = APPLE80211_MODE_11A | APPLE80211_MODE_11B |
                 APPLE80211_MODE_11G | APPLE80211_MODE_11N |
                 APPLE80211_MODE_11AC;

  pd->active_phy_mode = drv->m_pDevice->ie_dev->getPhyMode();
  return kIOReturnSuccess;
}

//
// MARK: 15 - OP_MODE
//

IOReturn AppleIntelWifiAdapterV2::getOP_MODE(
    IO80211Interface *interface, struct apple80211_opmode_data *od) {
  od->version = APPLE80211_VERSION;
  od->op_mode = this->drv->m_pDevice->ie_dev->getOPMode();
  return kIOReturnSuccess;
}

//
// MARK: 16 - RSSI
//

IOReturn AppleIntelWifiAdapterV2::getRSSI(IO80211Interface *interface,
                                          struct apple80211_rssi_data *rd) {
  if (drv->m_pDevice->ie_dev->getState() == APPLE80211_S_INIT ||
      drv->m_pDevice->ie_dev->getState() == APPLE80211_S_SCAN)
    return APPLE80211_REASON_NOT_AUTHED;
  // bzero(rd, sizeof(*rd));
  rd->version = APPLE80211_VERSION;
  rd->num_radios = 1;
  rd->rssi[0] = -42;
  rd->aggregate_rssi = -42;
  rd->rssi_unit = APPLE80211_UNIT_DBM;
  return kIOReturnSuccess;
}

//
// MARK: 17 - NOISE
//

IOReturn AppleIntelWifiAdapterV2::getNOISE(IO80211Interface *interface,
                                           struct apple80211_noise_data *nd) {
  if (drv->m_pDevice->ie_dev->getState() == APPLE80211_S_INIT ||
      drv->m_pDevice->ie_dev->getState() == APPLE80211_S_SCAN)
    return APPLE80211_REASON_NOT_AUTHED;
  bzero(nd, sizeof(*nd));

  nd->version = APPLE80211_VERSION;
  nd->num_radios = 1;
  nd->noise[0] = -101;
  nd->aggregate_noise = -101;
  nd->noise_unit = APPLE80211_UNIT_DBM;

  return kIOReturnSuccess;
}

//
// MARK: 18 - INT_MIT
//
IOReturn AppleIntelWifiAdapterV2::getINT_MIT(
    IO80211Interface *interface, struct apple80211_intmit_data *imd) {
  imd->version = APPLE80211_VERSION;
  imd->int_mit = APPLE80211_INT_MIT_AUTO;
  return kIOReturnSuccess;
}

//
// MARK: 19 - POWER
//

IOReturn AppleIntelWifiAdapterV2::getPOWER(IO80211Interface *interface,
                                           struct apple80211_power_data *pd) {
  pd->version = APPLE80211_VERSION;
  pd->num_radios = 1;
  pd->power_state[0] = drv->m_pDevice->power_state;

  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setPOWER(IO80211Interface *interface,
                                           struct apple80211_power_data *pd) {
  if (pd->num_radios > 0) {
    drv->m_pDevice->power_state = pd->power_state[0];
    IWL_INFO(0, "Setting power to %u\n", pd->power_state[0]);
    // dev->setPowerState(pd->power_state[0]);
  }
  interface->postMessage(APPLE80211_M_POWER_CHANGED);

  return kIOReturnSuccess;
}

//
// MARK: 20 - ASSOCIATE
//

IOReturn AppleIntelWifiAdapterV2::setASSOCIATE(
    IO80211Interface *interface, struct apple80211_assoc_data *ad) {
  IWL_INFO(0, "ASSOCIATE TO %s\n", ad->ad_ssid);

  if (ad->ad_mode == 1) {
    IWL_ERR(0, "p2p-gc and ibss cannot exist\n");
  } else {
    if (ad->ad_key.key_len != 0)
      drv->m_pDevice->ie_dev->setCipherKey(&ad->ad_key);

    drv->m_pDevice->ie_dev->setRSN_IE(
        reinterpret_cast<uint8_t *>(&ad->ad_rsn_ie), ad->ad_rsn_ie_len);

    drv->m_pDevice->ie_dev->setSSID(
        ad->ad_ssid_len, reinterpret_cast<const char *>(&ad->ad_ssid));
    drv->m_pDevice->ie_dev->setAPMode(ad->ad_mode);
    drv->m_pDevice->ie_dev->setBSSID(
        reinterpret_cast<uint8_t *>(&ad->ad_bssid.octet), ETH_ALEN);
    drv->m_pDevice->ie_dev->setState(APPLE80211_S_AUTH);
  }

  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setDISASSOCIATE(IO80211Interface *interface) {
  drv->m_pDevice->ie_dev->setState(APPLE80211_S_INIT);
  drv->m_pDevice->ie_dev->resetBSSID();
  drv->m_pDevice->ie_dev->resetSSID();
  drv->m_pDevice->ie_dev->resetCipherKey();
  drv->m_pDevice->ie_dev->resetRSN_IE();
  drv->m_pDevice->ie_dev->setAPMode(0);
  drv->m_pDevice->ie_dev->setPhyMode(0);
  drv->m_pDevice->ie_dev->setOPMode(0);
  return kIOReturnSuccess;
}

//
// MARK: 27 - SUPPORTED_CHANNELS
//

enum iwl_nvm_channel_flags {
  NVM_CHANNEL_VALID = BIT(0),
  NVM_CHANNEL_IBSS = BIT(1),
  NVM_CHANNEL_ACTIVE = BIT(3),
  NVM_CHANNEL_RADAR = BIT(4),
  NVM_CHANNEL_INDOOR_ONLY = BIT(5),
  NVM_CHANNEL_GO_CONCURRENT = BIT(6),
  NVM_CHANNEL_UNIFORM = BIT(7),
  NVM_CHANNEL_20MHZ = BIT(8),
  NVM_CHANNEL_40MHZ = BIT(9),
  NVM_CHANNEL_80MHZ = BIT(10),
  NVM_CHANNEL_160MHZ = BIT(11),
  NVM_CHANNEL_DC_HIGH = BIT(12),
};

IOReturn AppleIntelWifiAdapterV2::getSUPPORTED_CHANNELS(
    IO80211Interface *interface, struct apple80211_sup_channel_data *ad) {
  ad->version = APPLE80211_VERSION;
  ad->num_channels = this->drv->m_pDevice->ie_dev->getChannelMapSize();

  apple80211_channel *channel_map =
      this->drv->m_pDevice->ie_dev->getChannelMap();

  if (!channel_map) return kIOReturnError;

  memcpy(&ad->supported_channels, channel_map,
         sizeof(apple80211_channel) * ad->num_channels);

  return kIOReturnSuccess;
}

//
// MARK: 28 - LOCALE
//

IOReturn AppleIntelWifiAdapterV2::getLOCALE(IO80211Interface *interface,
                                            struct apple80211_locale_data *ld) {
  ld->version = APPLE80211_VERSION;
  ld->locale = APPLE80211_LOCALE_FCC;

  return kIOReturnSuccess;
}

//
// MARK: 37 - TX_ANTENNA
//
IOReturn AppleIntelWifiAdapterV2::getTX_ANTENNA(IO80211Interface *interface,
                                                apple80211_antenna_data *ad) {
  ad->version = APPLE80211_VERSION;
  ad->num_radios = 1;
  ad->antenna_index[0] = 1;
  return kIOReturnSuccess;
}

//
// MARK: 39 - ANTENNA_DIVERSITY
//

IOReturn AppleIntelWifiAdapterV2::getANTENNA_DIVERSITY(
    IO80211Interface *interface, apple80211_antenna_data *ad) {
  ad->version = APPLE80211_VERSION;
  ad->num_radios = 1;
  ad->antenna_index[0] = 1;
  return kIOReturnSuccess;
}

//
// MARK: 43 - DRIVER_VERSION
//

IOReturn AppleIntelWifiAdapterV2::getDRIVER_VERSION(
    IO80211Interface *interface, struct apple80211_version_data *hv) {
  hv->version = APPLE80211_VERSION;
  strncpy(hv->string, fake_drv_version, sizeof(hv->string));
  hv->string_len = strlen(fake_drv_version);
  return kIOReturnSuccess;
}

//
// MARK: 44 - HARDWARE_VERSION
//

IOReturn AppleIntelWifiAdapterV2::getHARDWARE_VERSION(
    IO80211Interface *interface, struct apple80211_version_data *hv) {
  hv->version = APPLE80211_VERSION;
  strncpy(hv->string, fake_hw_version, sizeof(hv->string));
  hv->string_len = strlen(fake_hw_version);
  return kIOReturnSuccess;
}

//
// MARK: 50 - ASSOCIATION_STATUS
//

IOReturn AppleIntelWifiAdapterV2::getASSOCIATION_STATUS(
    IO80211Interface *interface, struct apple80211_assoc_status_data *hv) {
  hv->version = APPLE80211_VERSION;
  hv->status = 0;
  return kIOReturnSuccess;
}

//
// MARK: 51 - COUNTRY_CODE
//

IOReturn AppleIntelWifiAdapterV2::getCOUNTRY_CODE(
    IO80211Interface *interface, struct apple80211_country_code_data *cd) {
  cd->version = APPLE80211_VERSION;
  strncpy(reinterpret_cast<char *>(cd->cc), country_code, sizeof(cd->cc));
  return kIOReturnSuccess;
}

//
// MARK: 57 - MCS
//
IOReturn AppleIntelWifiAdapterV2::getMCS(IO80211Interface *interface,
                                         struct apple80211_mcs_data *md) {
  if (drv->m_pDevice->ie_dev->getState() == APPLE80211_S_INIT ||
      drv->m_pDevice->ie_dev->getState() == APPLE80211_S_SCAN)
    return APPLE80211_REASON_NOT_AUTHED;
  md->version = APPLE80211_VERSION;
  md->index = APPLE80211_MCS_INDEX_AUTO;
  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::getROAM_THRESH(
    IO80211Interface *interface, struct apple80211_roam_threshold_data *md) {
  md->threshold = 1000;
  md->count = 0;
  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::getRADIO_INFO(
    IO80211Interface *interface, struct apple80211_radio_info_data *md) {
  md->version = 1;
  md->count = 1;
  return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setSCANCACHE_CLEAR(
    IO80211Interface *interface) {
  OSOrderedSet *scanCache = drv->m_pDevice->ie_dev->getScanCache();

  if (scanCache != NULL) {
    if (!drv->m_pDevice->ie_dev->lockScanCache()) return kIOReturnError;

    scanCache->flushCollection();

    drv->m_pDevice->ie_dev->unlockScanCache();
  }
  return kIOReturnSuccess;
}
