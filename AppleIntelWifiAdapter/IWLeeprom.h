//
//  IWLeeprom.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/1.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLeeprom_h
#define IWLeeprom_h

//#include <linux/mac80211.h>
#include <net80211/ieee80211_var.h>
#include <linux/types.h>
#include <linux/netdevice.h>

struct iwl_nvm_data {
    int n_hw_addrs;
    u8 hw_addr[ETH_ALEN];

    u8 calib_version;
    __le16 calib_voltage;

    __le16 raw_temperature;
    __le16 kelvin_temperature;
    __le16 kelvin_voltage;
    __le16 xtal_calib[2];

    bool sku_cap_band_24ghz_enable;
    bool sku_cap_band_52ghz_enable;
    bool sku_cap_11n_enable;
    bool sku_cap_11ac_enable;
    bool sku_cap_11ax_enable;
    bool sku_cap_amt_enable;
    bool sku_cap_ipan_enable;
    bool sku_cap_mimo_disabled;

    u16 radio_cfg_type;
    u8 radio_cfg_step;
    u8 radio_cfg_dash;
    u8 radio_cfg_pnum;
    u8 valid_tx_ant, valid_rx_ant;

    u32 nvm_version;
    s8 max_tx_pwr_half_dbm;

    bool lar_enabled;
    bool vht160_supported;
    //TODO
//    struct ieee80211_supported_band bands[NUM_NL80211_BANDS];
    struct ieee80211_channel channels[];
};

/**
 * struct iwl_nvm_section - describes an NVM section in memory.
 *
 * This struct holds an NVM section read from the NIC using NVM_ACCESS_CMD,
 * and saved for later use by the driver. Not all NVM sections are saved
 * this way, only the needed ones.
 */
struct iwl_nvm_section {
    u16 length;
    const u8 *data;
};

#endif /* IWLeeprom_h */
