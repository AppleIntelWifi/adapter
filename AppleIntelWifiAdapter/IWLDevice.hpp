//
//  IWLDevice.hpp
//  AppleIntelWifiAdapter
//
//  Created by qcwap on 2020/1/5.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLDevice_hpp
#define IWLDevice_hpp

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>

#include "IWLDeviceList.h"
#include "fw/FWImg.h"

enum iwl_power_level {
    IWL_POWER_INDEX_1,
    IWL_POWER_INDEX_2,
    IWL_POWER_INDEX_3,
    IWL_POWER_INDEX_4,
    IWL_POWER_INDEX_5,
    IWL_POWER_NUM
};

enum iwl_disable_11n {
    IWL_DISABLE_HT_ALL     = BIT(0),
    IWL_DISABLE_HT_TXAGG     = BIT(1),
    IWL_DISABLE_HT_RXAGG     = BIT(2),
    IWL_ENABLE_HT_TXAGG     = BIT(3),
};

enum iwl_amsdu_size {
    IWL_AMSDU_DEF = 0,
    IWL_AMSDU_4K = 1,
    IWL_AMSDU_8K = 2,
    IWL_AMSDU_12K = 3,
    /* Add 2K at the end to avoid breaking current API */
    IWL_AMSDU_2K = 4,
};

enum iwl_uapsd_disable {
    IWL_DISABLE_UAPSD_BSS        = BIT(0),
    IWL_DISABLE_UAPSD_P2P_CLIENT    = BIT(1),
};

/**
 * struct iwl_mod_params
 *
 * Holds the module parameters
 *
 * @swcrypto: using hardware encryption, default = 0
 * @disable_11n: disable 11n capabilities, default = 0,
 *    use IWL_[DIS,EN]ABLE_HT_* constants
 * @amsdu_size: See &enum iwl_amsdu_size.
 * @fw_restart: restart firmware, default = 1
 * @bt_coex_active: enable bt coex, default = true
 * @led_mode: system default, default = 0
 * @power_save: enable power save, default = false
 * @power_level: power level, default = 1
 * @debug_level: levels are IWL_DL_*
 * @antenna_coupling: antenna coupling in dB, default = 0
 * @nvm_file: specifies a external NVM file
 * @uapsd_disable: disable U-APSD, see &enum iwl_uapsd_disable, default =
 *    IWL_DISABLE_UAPSD_BSS | IWL_DISABLE_UAPSD_P2P_CLIENT
 * @xvt_default_mode: xVT is the default operation mode, default = false
 * @fw_monitor: allow to use firmware monitor
 * @disable_11ac: disable VHT capabilities, default = false.
 * @disable_msix: disable MSI-X and fall back to MSI on PCIe, default = false.
 * @remove_when_gone: remove an inaccessible device from the PCIe bus.
 * @enable_ini: enable new FW debug infratructure (INI TLVs)
 */
struct iwl_mod_params {
    int swcrypto;
    unsigned int disable_11n;
    int amsdu_size;
    bool fw_restart;
    bool bt_coex_active;
    int led_mode;
    bool power_save;
    int power_level;
    u32 debug_level;
    int antenna_coupling;
    bool xvt_default_mode;
    char *nvm_file;
    u32 uapsd_disable;
    bool fw_monitor;
    bool disable_11ac;
    /**
     * @disable_11ax: disable HE capabilities, default = false
     */
    bool disable_11ax;
    bool disable_msix;
    bool remove_when_gone;
    bool enable_ini;
};

class IWLDevice
{
public:
    
    bool init(IOPCIDevice *pciDevice);
    
    void release();
    
    void enablePCI();
    
    bool iwl_enable_rx_ampdu(void)
    {
        if (iwlwifi_mod_params.disable_11n & IWL_DISABLE_HT_RXAGG)
            return false;
        return true;
    }

    bool iwl_enable_tx_ampdu()
    {
        if (iwlwifi_mod_params.disable_11n & IWL_DISABLE_HT_TXAGG)
            return false;
        if (iwlwifi_mod_params.disable_11n & IWL_ENABLE_HT_TXAGG)
            return true;

        /* enabled by default */
        return true;
    }
    
public:
    
    IOPCIDevice *pciDevice;
    bool firmwareLoadToBuf;
    iwl_fw fw;
    iwl_firmware_pieces pieces;
    bool usniffer_images;
    
    const struct iwl_cfg *cfg;
    const char *name;
    /* shared module parameters */
    struct iwl_mod_params iwlwifi_mod_params = {
        .fw_restart = true,
        .bt_coex_active = true,
        .power_level = IWL_POWER_INDEX_1,
        .uapsd_disable = IWL_DISABLE_UAPSD_BSS | IWL_DISABLE_UAPSD_P2P_CLIENT,
        .enable_ini = true,
        /* the rest are 0 by default */
        
        //come from iwl-drv.c module_param_named
        .debug_level = 0644,
        .swcrypto = 0,
        .xvt_default_mode= false,
        .disable_11n = 1,
        .amsdu_size = 0,
        .antenna_coupling = 0,
        .led_mode = 0,
        .power_save = false,
        .fw_monitor = false,
        .disable_11ac = false,
        .disable_11ax = false,
        .disable_msix = false,
        .remove_when_gone = false,
    };
    
    uint32_t hw_rf_id;
    uint32_t hw_rev;
    UInt16 deviceID;
    uint16_t hw_id;
    uint16_t subSystemDeviceID;
    char hw_id_str[64];
    
    IOSimpleLock *registerRWLock;
    
    bool holdNICWake;
    
private:
    
    
};


#endif /* IWLDevice_hpp */
