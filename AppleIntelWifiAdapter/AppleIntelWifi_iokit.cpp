//
//  AppleIntelWifi_iokit.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/12/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "AppleIntelWifiAdapterV2.hpp"
#include "ioctl_dbg.h"
#include "IWLApple80211.hpp"
#include "IWLMvmMac.hpp"

const char *fake_hw_version = "Hardware 1.0";
const char *fake_drv_version = "Driver 1.0";
const char *country_code = "US";


SInt32 AppleIntelWifiAdapterV2::apple80211Request(unsigned int request_type,
                                            int request_number,
                                            IO80211Interface* interface,
                                            void* data) {
    if (request_type != SIOCGA80211 && request_type != SIOCSA80211) {
        IWL_ERR(0, "Invalid IOCTL request type: %u", request_type);
        IWL_ERR(0, "Expected either %lu or %lu", SIOCGA80211, SIOCSA80211);
        return kIOReturnError;
    }

    IOReturn ret = 0;
    
    bool isGet = (request_type == SIOCGA80211);
    
#define IOCTL(REQ_TYPE, REQ, DATA_TYPE) \
if (REQ_TYPE == SIOCGA80211) { \
ret = get##REQ(interface, (struct DATA_TYPE* )data); \
} else { \
ret = set##REQ(interface, (struct DATA_TYPE* )data); \
}
    
#define IOCTL_GET(REQ_TYPE, REQ, DATA_TYPE) \
if (REQ_TYPE == SIOCGA80211) { \
    ret = get##REQ(interface, (struct DATA_TYPE* )data); \
}
#define IOCTL_SET(REQ_TYPE, REQ, DATA_TYPE) \
if (REQ_TYPE == SIOCSA80211) { \
    ret = set##REQ(interface, (struct DATA_TYPE* )data); \
}
    
    IWL_INFO(0, "IOCTL %s(%d) %s\n",
          isGet ? "get" : "set",
          request_number,
          IOCTL_NAMES[request_number]);
    
    switch(request_number) {
        case APPLE80211_IOC_SSID: // 1
            IOCTL(request_type, SSID, apple80211_ssid_data);
            break;
        case APPLE80211_IOC_AUTH_TYPE: // 2
            IOCTL_GET(request_type, AUTH_TYPE, apple80211_authtype_data);
            break;
        case APPLE80211_IOC_CHANNEL: // 4
            IOCTL_GET(request_type, CHANNEL, apple80211_channel_data);
            break;
        case APPLE80211_IOC_PROTMODE:
            IOCTL(request_type, PROTMODE, apple80211_protmode_data);
            break;
        case APPLE80211_IOC_TXPOWER: // 7
            IOCTL_GET(request_type, TXPOWER, apple80211_txpower_data);
            break;
        case APPLE80211_IOC_RATE: // 8
            IOCTL_GET(request_type, RATE, apple80211_rate_data);
            break;
        case APPLE80211_IOC_BSSID: // 9
            IOCTL_GET(request_type, BSSID, apple80211_bssid_data);
            break;
        case APPLE80211_IOC_SCAN_REQ: // 10
            IOCTL_SET(request_type, SCAN_REQ, apple80211_scan_data);
            break;
        case APPLE80211_IOC_SCAN_RESULT: // 11
            IOCTL_GET(request_type, SCAN_RESULT, apple80211_scan_result*);
            break;
        case APPLE80211_IOC_CARD_CAPABILITIES: // 12
            IOCTL_GET(request_type, CARD_CAPABILITIES, apple80211_capability_data);
            break;
        case APPLE80211_IOC_STATE: // 13
            IOCTL_GET(request_type, STATE, apple80211_state_data);
            break;
        case APPLE80211_IOC_PHY_MODE: // 14
            IOCTL_GET(request_type, PHY_MODE, apple80211_phymode_data);
            break;
        case APPLE80211_IOC_OP_MODE: // 15
            IOCTL_GET(request_type, OP_MODE, apple80211_opmode_data);
            break;
        case APPLE80211_IOC_RSSI: // 16
            IOCTL_GET(request_type, RSSI, apple80211_rssi_data);
            break;
        case APPLE80211_IOC_NOISE: // 17
            IOCTL_GET(request_type, NOISE, apple80211_noise_data);
            break;
        case APPLE80211_IOC_INT_MIT: // 18
            IOCTL_GET(request_type, INT_MIT, apple80211_intmit_data);
            break;
        case APPLE80211_IOC_POWER: // 19
            IOCTL(request_type, POWER, apple80211_power_data);
            break;
        case APPLE80211_IOC_ASSOCIATE: // 20
            IOCTL_SET(request_type, ASSOCIATE, apple80211_assoc_data);
            break;
        case APPLE80211_IOC_SUPPORTED_CHANNELS: // 27
            IOCTL_GET(request_type, SUPPORTED_CHANNELS, apple80211_sup_channel_data);
            break;
        case APPLE80211_IOC_LOCALE: // 28
            IOCTL_GET(request_type, LOCALE, apple80211_locale_data);
            break;
        case APPLE80211_IOC_TX_ANTENNA: // 37
            IOCTL_GET(request_type, TX_ANTENNA, apple80211_antenna_data);
            break;
        case APPLE80211_IOC_ANTENNA_DIVERSITY: // 39
            IOCTL_GET(request_type, ANTENNA_DIVERSITY, apple80211_antenna_data);
            break;
        case APPLE80211_IOC_DRIVER_VERSION: // 43
            IOCTL_GET(request_type, DRIVER_VERSION, apple80211_version_data);
            break;
        case APPLE80211_IOC_HARDWARE_VERSION: // 44
            IOCTL_GET(request_type, HARDWARE_VERSION, apple80211_version_data);
            break;
        case APPLE80211_IOC_ASSOCIATION_STATUS: //50
            IOCTL_GET(request_type, ASSOCIATION_STATUS, apple80211_assoc_status_data);
            break;
        case APPLE80211_IOC_COUNTRY_CODE: // 51
            IOCTL_GET(request_type, COUNTRY_CODE, apple80211_country_code_data);
            break;
        case APPLE80211_IOC_RADIO_INFO:
            IOCTL_GET(request_type, RADIO_INFO, apple80211_radio_info_data);
            break;
        case APPLE80211_IOC_MCS: // 57
            IOCTL_GET(request_type, MCS, apple80211_mcs_data);
            break;
        case APPLE80211_IOC_WOW_PARAMETERS: // 69
            break;
        case APPLE80211_IOC_ROAM_THRESH:
            IOCTL_GET(request_type, ROAM_THRESH, apple80211_roam_threshold_data);
            break;
        case APPLE80211_IOC_TX_CHAIN_POWER: // 108
            break;
        case APPLE80211_IOC_THERMAL_THROTTLING: // 111
            break;
        case APPLE80211_IOC_POWERSAVE:
            break;
        case APPLE80211_IOC_IE:
            ret = 6;
            break;
        case 353:
            ret = 6;
            break;
        default:
            IWL_ERR(0, "Unhandled IOCTL %s (%d)\n", IOCTL_NAMES[request_number], request_number);
            ret = kIOReturnError;
            break;
    }
    
    return ret;
}

IOReturn AppleIntelWifiAdapterV2::getSSID(IO80211Interface *interface,
                                    struct apple80211_ssid_data *sd) {
    
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
        return APPLE80211_REASON_NOT_AUTHED;
    }
    
     
    bzero(sd, sizeof(*sd));
    sd->version = APPLE80211_VERSION;
    //strncpy((char*)sd->ssid_bytes, fake_ssid, sizeof(sd->ssid_bytes));
    //sd->ssid_len = (uint32_t)strlen(fake_ssid);

    return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setSSID(IO80211Interface *interface,
                                    struct apple80211_ssid_data *sd) {
    
    //fInterface->postMessage(APPLE80211_M_SSID_CHANGED);
    return kIOReturnSuccess;
}

//
// MARK: 2 - AUTH_TYPE
//

IOReturn AppleIntelWifiAdapterV2::getAUTH_TYPE(IO80211Interface *interface,
                                         struct apple80211_authtype_data *ad) {
    ad->version = APPLE80211_VERSION;
    ad->authtype_lower = APPLE80211_AUTHTYPE_OPEN;
    ad->authtype_upper = APPLE80211_AUTHTYPE_NONE;
    return kIOReturnSuccess;
}

//
// MARK: 4 - CHANNEL
//

IOReturn AppleIntelWifiAdapterV2::getCHANNEL(IO80211Interface *interface,
                                       struct apple80211_channel_data *cd) {
    //return kIOReturnError;
    memset(cd, 0, sizeof(apple80211_channel_data));
    /*
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
        return APPLE80211_REASON_NOT_AUTHED;
    }
     */
    //bzero(cd, sizeof(apple80211_channel_data));
    
    cd->version = APPLE80211_VERSION;
    cd->channel.channel = 1;
    cd->channel.flags = APPLE80211_C_FLAG_2GHZ | APPLE80211_C_FLAG_ACTIVE | APPLE80211_C_FLAG_20MHZ;
    //memcpy(&cd->channel, &drv->m_pDevice->ie_dev->channels[0], sizeof(apple80211_channel));
    return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::getPROTMODE(IO80211Interface* interface, struct apple80211_protmode_data* pd) {
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
        return APPLE80211_REASON_NOT_AUTHED;
    }
    
    return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setPROTMODE(IO80211Interface* interface, struct apple80211_protmode_data* pd) {
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
        return APPLE80211_REASON_NOT_AUTHED;
    }
    
    return kIOReturnSuccess;
}

//
// MARK: 7 - TXPOWER
//

IOReturn AppleIntelWifiAdapterV2::getTXPOWER(IO80211Interface *interface,
                                       struct apple80211_txpower_data *txd) {

    txd->version = APPLE80211_VERSION;
    txd->txpower = 100;
    txd->txpower_unit = APPLE80211_UNIT_PERCENT;
    return kIOReturnSuccess;
}

//
// MARK: 8 - RATE
//

IOReturn AppleIntelWifiAdapterV2::getRATE(IO80211Interface *interface, struct apple80211_rate_data *rd) {
    
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
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
    
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
        return APPLE80211_REASON_NOT_AUTHED;
    }
    
    
    bzero(bd, sizeof(*bd));
    bd->version = APPLE80211_VERSION;
    //memcpy(bd->bssid.octet, fake_bssid, sizeof(fake_bssid));
    return kIOReturnSuccess;
}

int iwl_config_runtime_scan(IWLMvmDriver* drv, apple80211_scan_data* appleReq) {
    struct iwl_scan_config_v1* cfg;
    int nchan, err;
    size_t len = sizeof(iwl_scan_config_v1) + appleReq->num_channels;
    
    cfg = (iwl_scan_config_v1*)kzalloc(len);
    
    iwl_host_cmd hcmd = {
        .id = iwl_cmd_id(SCAN_CFG_CMD, IWL_ALWAYS_LONG_GROUP, 0),
        .dataflags = {IWL_HCMD_DFL_NOCOPY},
        .flags = 0
    };
    
    hcmd.data[0] = cfg;
    hcmd.len[0] = len;
    
    struct apple80211_channel *c;
    
    nchan = 0;
    
    int num_channels = appleReq->num_channels;
    
    if(num_channels > IEEE80211_CHAN_MAX) {
        IWL_ERR(0, "ucode asked for %d channels\n", num_channels);
        IOFree(cfg, len);
        return -1;
    }
    
    
    for (nchan = 0; nchan < num_channels; nchan++) {
        c = &appleReq->channels[nchan];
        
        if(!c)
            continue;
        
        IWL_INFO(0, "Adding channel %d to scan", c->channel);
        cfg->channel_array[nchan] = c->channel;
    }
    
    cfg->flags = cpu_to_le32(SCAN_CONFIG_FLAG_ACTIVATE |
        SCAN_CONFIG_FLAG_SET_CHANNEL_FLAGS |
        SCAN_CONFIG_N_CHANNELS(nchan) |
        SCAN_CONFIG_FLAG_CLEAR_FRAGMENTED);
    
        
    err = drv->sendCmd(&hcmd);
    
    IOFree(cfg, len);
    //drv->trans->freeResp(&hcmd);
    return err;
}


IOReturn AppleIntelWifiAdapterV2::scanAction(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3) {
    IOReturn ret = kIOReturnSuccess;
    
    AppleIntelWifiAdapterV2* that = (AppleIntelWifiAdapterV2*)target;
    IO80211Interface *iface = (IO80211Interface *)arg0;
    IWLMvmDriver* dev = (IWLMvmDriver*)that->drv;
    apple80211_scan_data* sd = (apple80211_scan_data*)arg1;
    
    dev->m_pDevice->ie_dev->scanning = true;
    for(int i = 0; i < sd->num_channels; i++) {
        IWL_INFO(0, "%d: ch %d\n", i, sd->channels[i].channel);
    }

    //IWL_INFO(0, "device: %s\n", dev->m_pDevice->cfg->name);
    IWL_INFO(0, "scanning (ptr: %x, dev_ptr: %x, dev: %s, umac: %s)\n", dev, dev->m_pDevice, dev->m_pDevice->name, dev->m_pDevice->umac_scanning ? "yes" : "no");
    
    
#ifdef notyet
    if(fw_has_capa(&dev->m_pDevice->fw.ucode_capa, IWL_UCODE_TLV_CAPA_UMAC_SCAN)) {
#else
    if(1) {
#endif
        if(iwl_umac_scan(dev, sd) != 0) {
            IWL_ERR(0, "umac scan failed\n");
            ret = kIOReturnError;
        }
    } else {
        //IWL_ERR(0, "lmac scanning not implemented yet (device: %s)\n", &dev->m_pDevice->name);
        if(iwl_lmac_scan(dev, sd) != 0) {
            IWL_ERR(0, "lmac scan failed\n");
            ret = kIOReturnError;
        }
    }

    IOFree(sd, sizeof(apple80211_scan_data));
        
    //IOSleep(4000);
    
    return ret;
}

//
// MARK: 10 - SCAN_REQ
//

IOReturn AppleIntelWifiAdapterV2::setSCAN_REQ(IO80211Interface *interface,
                                        struct apple80211_scan_data *sd) {
    if(drv->m_pDevice->ie_dev->scanning) {
        IWL_ERR(0, "Denying scan because device is already scanning\n");
        return kIOReturnBusy;
    }
    
    if(drv->m_pDevice->ie_dev->scan_max != 0) {
        for(int k = 0; k < drv->m_pDevice->ie_dev->scan_max; k++) {
            IWL_INFO(0, "Freeing old scan IE data\n");
            IOFree(drv->m_pDevice->ie_dev->scan_results[k].asr_ie_data,
                drv->m_pDevice->ie_dev->scan_results[k].asr_ie_len);
            memset(&drv->m_pDevice->ie_dev->scan_results[k], 0, sizeof(apple80211_scan_result));
        }
    }
    
    
    drv->m_pDevice->ie_dev->state = APPLE80211_S_SCAN;
    IWL_INFO(0, "Apple80211. Scan requested. Type: %u\n"
          "BSS Type: %u\n"
          "PHY Mode: %u\n"
          "Dwell time: %u\n"
          "Rest time: %u\n"
          "Num channels: %u\n",
          sd->scan_type,
          sd->bss_type,
          sd->phy_mode,
          sd->dwell_time,
          sd->rest_time,
          sd->num_channels);
    
    memset(&drv->m_pDevice->ie_dev->channels_scan, 0, 256 * sizeof(apple80211_channel));
    
    memcpy(&drv->m_pDevice->ie_dev->channels_scan, &sd->channels, sd->num_channels * sizeof(apple80211_channel));
    drv->m_pDevice->ie_dev->n_scan_chans = sd->num_channels;
    drv->m_pDevice->ie_dev->scan_max = 0;
    drv->m_pDevice->ie_dev->scan_index = 0;
    
    
    if(interface) {
        apple80211_scan_data* request = (apple80211_scan_data*)IOMalloc(sizeof(apple80211_scan_data));
        memcpy(request, sd, sizeof(apple80211_scan_data));

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
    "\x2d\x1a\xad\x01\x1b\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x06\xe6\xe7\x0d\x00" \
    // HT capas
    "\x3d\x16\x01\x00\x17\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \ // HT info
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

const uint8_t fake_bssid[] = { 0x64, 0x7C, 0x34, 0x5C, 0x1C, 0x40 };
const char *fake_ssid = "UPC5424297";

IOReturn AppleIntelWifiAdapterV2::getSCAN_RESULT(IO80211Interface *interface,
                                           struct apple80211_scan_result **sr) {
    
    if(drv->m_pDevice->ie_dev->state != APPLE80211_S_SCAN) {
        return 0xe0820446;
    }
    
    int index = drv->m_pDevice->ie_dev->scan_index;
    int max = drv->m_pDevice->ie_dev->scan_max;
    
    IWL_INFO(0, "scan index: %d out of %d\n", index, max);

    bool found = false;
    /*
    for(int i = index; i < max; i++) {
        apple80211_scan_result* result = drv->m_pDevice->ie_dev->scan_results[i];
        if(result == NULL) {
            continue;
        }
        
        for(int k = 0; k < drv->m_pDevice->ie_dev->n_scan_chans; k++) {
            //IWL_INFO(0, "wanted channel %d\n", drv->m_pDevice->ie_dev->channels_scan[k].channel);
            IWL_INFO(0, "scan result on chan %d\n", result->asr_channel.channel);
            //if(result->asr_channel.channel == drv->m_pDevice->ie_dev->channels_scan[k].channel) {
                found = true;
            //}
        }
        
        if(found) {
            *sr = result;
            break;
        }
    }
     */
    apple80211_scan_result* result = &drv->m_pDevice->ie_dev->scan_results[index];
    *sr = result;
    
    IWL_INFO(0, "Scan result (SSID: %s, channel: %d, RSSI: %d)\n", result->asr_ssid, result->asr_channel.channel, result->asr_rssi);
     
    drv->m_pDevice->ie_dev->scan_index++;
    if(index + 1 >= drv->m_pDevice->ie_dev->scan_max) {
        drv->m_pDevice->published = false;
        drv->m_pDevice->ie_dev->state = APPLE80211_S_INIT;
    }
    
    return kIOReturnSuccess;
}

//
// MARK: 12 - CARD_CAPABILITIES
//

IOReturn AppleIntelWifiAdapterV2::getCARD_CAPABILITIES(IO80211Interface *interface,
                                                 struct apple80211_capability_data *cd) {
    cd->version = APPLE80211_VERSION;
    cd->capabilities[0] = 0xab;
    cd->capabilities[1] = 0x7e;
    return kIOReturnSuccess;
}

//
// MARK: 13 - STATE
//

IOReturn AppleIntelWifiAdapterV2::getSTATE(IO80211Interface *interface,
                                     struct apple80211_state_data *sd) {
    sd->version = APPLE80211_VERSION;
    sd->state = drv->m_pDevice->ie_dev->state;
    return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setSTATE(IO80211Interface *interface,
                                     struct apple80211_state_data *sd) {
    IWL_INFO(0, "Setting state: %u\n", sd->state);
    drv->m_pDevice->ie_dev->state = sd->state;
    return kIOReturnSuccess;
}

//
// MARK: 14 - PHY_MODE
//

IOReturn AppleIntelWifiAdapterV2::getPHY_MODE(IO80211Interface *interface,
                                        struct apple80211_phymode_data *pd) {
    
    pd->version = APPLE80211_VERSION;
    pd->phy_mode = APPLE80211_MODE_11A
                 | APPLE80211_MODE_11B
                 | APPLE80211_MODE_11G
                 | APPLE80211_MODE_11N
                 | APPLE80211_MODE_11AC;
    
    pd->active_phy_mode = APPLE80211_MODE_AUTO;
    return kIOReturnSuccess;
}

//
// MARK: 15 - OP_MODE
//

IOReturn AppleIntelWifiAdapterV2::getOP_MODE(IO80211Interface *interface,
                                       struct apple80211_opmode_data *od) {
    
    od->version = APPLE80211_VERSION;
    od->op_mode = APPLE80211_M_NONE;
    return kIOReturnSuccess;
}

//
// MARK: 16 - RSSI
//

IOReturn AppleIntelWifiAdapterV2::getRSSI(IO80211Interface *interface,
                                    struct apple80211_rssi_data *rd) {
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
        return APPLE80211_REASON_NOT_AUTHED;
    }
    //bzero(rd, sizeof(*rd));
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
    
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
        return APPLE80211_REASON_NOT_AUTHED;
    }
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
IOReturn AppleIntelWifiAdapterV2::getINT_MIT(IO80211Interface* interface,
                                       struct apple80211_intmit_data* imd) {
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
        //dev->setPowerState(pd->power_state[0]);
    }
    interface->postMessage(APPLE80211_M_POWER_CHANGED);
    
    return kIOReturnSuccess;
}

//
// MARK: 20 - ASSOCIATE
//

IOReturn AppleIntelWifiAdapterV2::setASSOCIATE(IO80211Interface *interface,
                                         struct apple80211_assoc_data *ad) {
    IOLog("AppleIntelWifiAdapterV2::setAssociate %s", ad->ad_ssid);
    netif->setLinkState(IO80211LinkState::kIO80211NetworkLinkUp, 0);
    return kIOReturnSuccess;
}

//
// MARK: 27 - SUPPORTED_CHANNELS
//

enum iwl_nvm_channel_flags {
    NVM_CHANNEL_VALID        = BIT(0),
    NVM_CHANNEL_IBSS        = BIT(1),
    NVM_CHANNEL_ACTIVE        = BIT(3),
    NVM_CHANNEL_RADAR        = BIT(4),
    NVM_CHANNEL_INDOOR_ONLY        = BIT(5),
    NVM_CHANNEL_GO_CONCURRENT    = BIT(6),
    NVM_CHANNEL_UNIFORM        = BIT(7),
    NVM_CHANNEL_20MHZ        = BIT(8),
    NVM_CHANNEL_40MHZ        = BIT(9),
    NVM_CHANNEL_80MHZ        = BIT(10),
    NVM_CHANNEL_160MHZ        = BIT(11),
    NVM_CHANNEL_DC_HIGH        = BIT(12),
};

IOReturn AppleIntelWifiAdapterV2::getSUPPORTED_CHANNELS(IO80211Interface *interface,
                                                  struct apple80211_sup_channel_data *ad) {
    ad->version = APPLE80211_VERSION;
    ad->num_channels = this->drv->m_pDevice->ie_dev->n_chans;
    
    if(ad->num_channels > 64) {
        ad->num_channels = 64;
    }
    memcpy(&ad->supported_channels, this->drv->m_pDevice->ie_dev->channels, sizeof(apple80211_channel) * ad->num_channels);
    
    return kIOReturnSuccess;
}

//
// MARK: 28 - LOCALE
//

IOReturn AppleIntelWifiAdapterV2::getLOCALE(IO80211Interface *interface,
                                      struct apple80211_locale_data *ld) {
    ld->version = APPLE80211_VERSION;
    ld->locale  = APPLE80211_LOCALE_FCC;
    
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

IOReturn AppleIntelWifiAdapterV2::getANTENNA_DIVERSITY(IO80211Interface *interface,
                                                 apple80211_antenna_data *ad) {
    ad->version = APPLE80211_VERSION;
    ad->num_radios = 1;
    ad->antenna_index[0] = 1;
    return kIOReturnSuccess;
}

//
// MARK: 43 - DRIVER_VERSION
//

IOReturn AppleIntelWifiAdapterV2::getDRIVER_VERSION(IO80211Interface *interface,
                                              struct apple80211_version_data *hv) {
    hv->version = APPLE80211_VERSION;
    strncpy(hv->string, fake_drv_version, sizeof(hv->string));
    hv->string_len = strlen(fake_drv_version);
    return kIOReturnSuccess;
}

//
// MARK: 44 - HARDWARE_VERSION
//

IOReturn AppleIntelWifiAdapterV2::getHARDWARE_VERSION(IO80211Interface *interface,
                                                struct apple80211_version_data *hv) {
    hv->version = APPLE80211_VERSION;
    strncpy(hv->string, fake_hw_version, sizeof(hv->string));
    hv->string_len = strlen(fake_hw_version);
    return kIOReturnSuccess;
}

//
// MARK: 50 - ASSOCIATION_STATUS
//

IOReturn AppleIntelWifiAdapterV2::getASSOCIATION_STATUS(IO80211Interface* interface,
                                                        struct apple80211_assoc_status_data* hv) {
    hv->version = APPLE80211_VERSION;
    hv->status = APPLE80211_RESULT_UNKNOWN;
    return kIOReturnSuccess;
}

//
// MARK: 51 - COUNTRY_CODE
//

IOReturn AppleIntelWifiAdapterV2::getCOUNTRY_CODE(IO80211Interface *interface,
                                            struct apple80211_country_code_data *cd) {
    cd->version = APPLE80211_VERSION;
    strncpy((char*)cd->cc, country_code, sizeof(cd->cc));
    return kIOReturnSuccess;
}

//
// MARK: 57 - MCS
//
IOReturn AppleIntelWifiAdapterV2::getMCS(IO80211Interface* interface, struct apple80211_mcs_data* md) {
    if(drv->m_pDevice->ie_dev->state == APPLE80211_S_INIT || drv->m_pDevice->ie_dev->state == APPLE80211_S_SCAN) {
        return APPLE80211_REASON_NOT_AUTHED;
    }
    md->version = APPLE80211_VERSION;
    md->index = APPLE80211_MCS_INDEX_AUTO;
    return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::getROAM_THRESH(IO80211Interface* interface, struct apple80211_roam_threshold_data* md) {
    md->threshold = 1000;
    md->count = 0;
    return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::getRADIO_INFO(IO80211Interface* interface, struct apple80211_radio_info_data* md)
{
    md->version = 1;
    md->count = 1;
    return kIOReturnSuccess;
}

