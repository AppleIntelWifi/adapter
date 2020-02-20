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
#include "IWLFH.h"
#include "paging.h"
#include "fw/api/cmdhdr.h"
#include "IWLeeprom.h"
#include "txq.h"
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include "IWLConstants.h"
#include "fw/NotificationWait.hpp"
#include "Mvm.h"

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

static inline int
iwl_trans_get_rb_size_order(enum iwl_amsdu_size rb_size)
{
    
    // TODO: Implement get_order
    switch (rb_size) {
    case IWL_AMSDU_4K:
            return __get_order(4 * 1024);
    case IWL_AMSDU_8K:
            return __get_order(8 * 1024);
    case IWL_AMSDU_12K:
            return __get_order(12 * 1024);
    default:
        //WARN_ON(1);
        return -1;
    }
}


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

/**
 * struct iwl_mvm_tvqm_txq_info - maps TVQM hw queue to tid
 *
 * @sta_id: sta id
 * @txq_tid: txq tid
 */
struct iwl_mvm_tvqm_txq_info {
    u8 sta_id;
    u8 txq_tid;
};

struct iwl_mvm_dqa_txq_info {
    u8 ra_sta_id; /* The RA this queue is mapped to, if exists */
    bool reserved; /* Is this the TXQ reserved for a STA */
    u8 mac80211_ac; /* The mac80211 AC this queue is mapped to */
    u8 txq_tid; /* The TID "owner" of this queue*/
    u16 tid_bitmap; /* Bitmap of the TIDs mapped to this queue */
    /* Timestamp for inactivation per TID of this queue */
    unsigned long last_frame_time[IWL_MAX_TID_COUNT + 1];
    enum iwl_mvm_queue_status status;
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
    
    // MARK: 80211 support
    ieee80211_state ie_state;
    ieee80211_amrr ie_amrr;
    ieee80211com ie_ic;
    IOEthernetInterface* interface;
    IOEthernetController* controller;
    
    // MARK: BT Coex
    int btForceAntMode;
    iwl_bt_coex_profile_notif lastBtNotif;
    iwl_bt_coex_ci_cmd lastBtCiCmd;
    
    // MARK: firmware
    bool firmwareLoadToBuf;
    iwl_fw fw;
    iwl_firmware_pieces pieces;
    bool usniffer_images;
    
    // MARK: NVM Sections
    iwl_nvm_section nvm_sections[NVM_MAX_NUM_SECTIONS];
    iwl_fw_paging fw_paging_db[NUM_OF_FW_PAGING_BLOCKS];
    u16 num_of_paging_blk;
    u16 num_of_pages_in_last_blk;
    enum iwl_ucode_type cur_fw_img;
    iwl_nvm_data *nvm_data;
    
    // MARK: device config
    const struct iwl_cfg *cfg;
    const char *name;
    
    // MARK: mvm
    unsigned long status;
    bool rfkill_safe_init_done;
    iwl_notif_wait_data notif_wait;
    IOLock *rx_sync_waitq;
    iwl_phy_ctx phy_ctx[NUM_PHY_CTX];

    // MARK: queue info
    union {
        struct iwl_mvm_dqa_txq_info queue_info[IWL_MAX_HW_QUEUES];
        struct iwl_mvm_tvqm_txq_info tvqm_info[IWL_MAX_TVQM_QUEUES];
    };
    
    /* MARK: shared module parameters */
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
        .disable_msix = true,
        .remove_when_gone = false,
    };
    
    // MARK: pci device info
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

static inline bool iwl_mvm_is_radio_killed(IWLDevice *mvm)
{
    return test_bit(IWL_MVM_STATUS_HW_RFKILL, &mvm->status) ||
           test_bit(IWL_MVM_STATUS_HW_CTKILL, &mvm->status);
}

static inline bool iwl_mvm_is_radio_hw_killed(IWLDevice *mvm)
{
    return test_bit(IWL_MVM_STATUS_HW_RFKILL, &mvm->status);
}

static inline bool iwl_mvm_firmware_running(IWLDevice *mvm)
{
    return test_bit(IWL_MVM_STATUS_FIRMWARE_RUNNING, &mvm->status);
}

/* Find TFD CB base pointer for given queue */
static inline unsigned int FH_MEM_CBBC_QUEUE(IWLDevice *trans,
                         unsigned int chnl)
{
    if (trans->cfg->trans.use_tfh) {
        WARN_ON_ONCE(chnl >= 64);
        return TFH_TFDQ_CBB_TABLE + 8 * chnl;
    }
    if (chnl < 16)
        return FH_MEM_CBBC_0_15_LOWER_BOUND + 4 * chnl;
    if (chnl < 20)
        return FH_MEM_CBBC_16_19_LOWER_BOUND + 4 * (chnl - 16);
    WARN_ON_ONCE(chnl >= 32);
    return FH_MEM_CBBC_20_31_LOWER_BOUND + 4 * (chnl - 20);
}

static inline u8 iwl_mvm_get_valid_tx_ant(IWLDevice *mvm)
{
    return mvm->nvm_data && mvm->nvm_data->valid_tx_ant ?
           mvm->fw.valid_tx_ant & mvm->nvm_data->valid_tx_ant :
           mvm->fw.valid_tx_ant;
}

static inline u8 iwl_mvm_get_valid_rx_ant(IWLDevice *mvm)
{
    return mvm->nvm_data && mvm->nvm_data->valid_rx_ant ?
           mvm->fw.valid_rx_ant & mvm->nvm_data->valid_rx_ant :
           mvm->fw.valid_rx_ant;
}

static inline u32 iwl_mvm_get_phy_config(IWLDevice *mvm)
{
    u32 phy_config = ~(FW_PHY_CFG_TX_CHAIN |
               FW_PHY_CFG_RX_CHAIN);
    u32 valid_rx_ant = iwl_mvm_get_valid_rx_ant(mvm);
    u32 valid_tx_ant = iwl_mvm_get_valid_tx_ant(mvm);
    
    IWL_INFO(0, "valid_rx_ant=%d, valid_tx_ant=%d\n", valid_rx_ant, valid_tx_ant);

    phy_config |= valid_tx_ant << FW_PHY_CFG_TX_CHAIN_POS |
              valid_rx_ant << FW_PHY_CFG_RX_CHAIN_POS;
    
    IWL_INFO(0, "phy_config=%u\n", phy_config);

    return mvm->fw.phy_config & phy_config;
}

static inline bool iwl_mvm_is_adaptive_dwell_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_ADAPTIVE_DWELL);
}

static inline bool iwl_mvm_is_adaptive_dwell_v2_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_ADAPTIVE_DWELL_V2);
}

static inline bool iwl_mvm_is_adwell_hb_ap_num_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_ADWELL_HB_DEF_N_AP);
}

static inline bool iwl_mvm_is_oce_supported(IWLDevice *mvm)
{
    /* OCE should never be enabled for LMAC scan FWs */
    return fw_has_api(&mvm->fw.ucode_capa, IWL_UCODE_TLV_API_OCE);
}

static inline bool iwl_mvm_is_frag_ebs_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa, IWL_UCODE_TLV_API_FRAG_EBS);
}

static inline bool iwl_mvm_is_short_beacon_notif_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_SHORT_BEACON_NOTIF);
}

static inline bool iwl_mvm_is_dqa_data_queue(IWLDevice *mvm, u8 queue)
{
    return (queue >= IWL_MVM_DQA_MIN_DATA_QUEUE) &&
           (queue <= IWL_MVM_DQA_MAX_DATA_QUEUE);
}

static inline bool iwl_mvm_is_dqa_mgmt_queue(IWLDevice *mvm, u8 queue)
{
    return (queue >= IWL_MVM_DQA_MIN_MGMT_QUEUE) &&
           (queue <= IWL_MVM_DQA_MAX_MGMT_QUEUE);
}

static inline bool iwl_mvm_is_lar_supported(IWLDevice *mvm)
{
    bool nvm_lar = mvm->nvm_data->lar_enabled;
    bool tlv_lar = fw_has_capa(&mvm->fw.ucode_capa,
                   IWL_UCODE_TLV_CAPA_LAR_SUPPORT);

    /*
     * Enable LAR only if it is supported by the FW (TLV) &&
     * enabled in the NVM
     */
    if (mvm->cfg->nvm_type == IWL_NVM_EXT)
        return nvm_lar && tlv_lar;
    else
        return tlv_lar;
}

static inline bool iwl_mvm_is_wifi_mcc_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_WIFI_MCC_UPDATE) ||
           fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_LAR_MULTI_MCC);
}

static inline bool iwl_mvm_bt_is_rrc_supported(IWLDevice *mvm)
{
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_BT_COEX_RRC) &&
        IWL_MVM_BT_COEX_RRC;
}

static inline bool iwl_mvm_is_csum_supported(IWLDevice *mvm)
{
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_CSUM_SUPPORT) &&
               !IWL_MVM_HW_CSUM_DISABLE;
}

static inline bool iwl_mvm_is_mplut_supported(IWLDevice *mvm)
{
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_BT_MPLUT_SUPPORT) &&
        IWL_MVM_BT_COEX_MPLUT;
}

static inline
bool iwl_mvm_is_p2p_scm_uapsd_supported(IWLDevice *mvm)
{
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_P2P_SCM_UAPSD) &&
        !(mvm->iwlwifi_mod_params.uapsd_disable &
          IWL_DISABLE_UAPSD_P2P_CLIENT);
}

static inline bool iwl_mvm_has_new_rx_api(IWLDevice *mvm)
{
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_MULTI_QUEUE_RX_SUPPORT);
}

static inline bool iwl_mvm_has_new_tx_api(IWLDevice *mvm)
{
    /* TODO - replace with TLV once defined */
    return mvm->cfg->trans.use_tfh;
}

static inline bool iwl_mvm_has_unified_ucode(IWLDevice *mvm)
{
    /* TODO - better define this */
    return mvm->cfg->trans.device_family >= IWL_DEVICE_FAMILY_22000;
}

static inline bool iwl_mvm_is_cdb_supported(IWLDevice *mvm)
{
    /*
     * TODO:
     * The issue of how to determine CDB APIs and usage is still not fully
     * defined.
     * There is a compilation for CDB and non-CDB FW, but there may
     * be also runtime check.
     * For now there is a TLV for checking compilation mode, but a
     * runtime check will also have to be here - once defined.
     */
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_CDB_SUPPORT);
}

static inline bool iwl_mvm_cdb_scan_api(IWLDevice *mvm)
{
    /*
     * TODO: should this be the same as iwl_mvm_is_cdb_supported()?
     * but then there's a little bit of code in scan that won't make
     * any sense...
     */
    return mvm->cfg->trans.device_family >= IWL_DEVICE_FAMILY_22000;
}

static inline bool iwl_mvm_is_reduced_config_scan_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_REDUCED_SCAN_CONFIG);
}

static inline bool iwl_mvm_is_scan_ext_chan_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_SCAN_EXT_CHAN_VER);
}

static inline bool iwl_mvm_is_band_in_rx_supported(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_API_BAND_IN_RX_DATA);
}

static inline bool iwl_mvm_has_new_rx_stats_api(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_NEW_RX_STATS);
}

static inline bool iwl_mvm_has_quota_low_latency(IWLDevice *mvm)
{
    return fw_has_api(&mvm->fw.ucode_capa,
              IWL_UCODE_TLV_API_QUOTA_LOW_LATENCY);
}

static inline bool iwl_mvm_has_tlc_offload(const IWLDevice *mvm)
{
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_TLC_OFFLOAD);
}

static inline struct agg_tx_status *
iwl_mvm_get_agg_status(IWLDevice *mvm, void *tx_resp)
{
    if (iwl_mvm_has_new_tx_api(mvm))
        return &((struct iwl_mvm_tx_resp *)tx_resp)->status;
    else
        return ((struct iwl_mvm_tx_resp_v3 *)tx_resp)->status;
}

static inline bool iwl_mvm_is_tt_in_fw(IWLDevice *mvm)
{
    /* these two TLV are redundant since the responsibility to CT-kill by
     * FW happens only after we send at least one command of
     * temperature THs report.
     */
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_CT_KILL_BY_FW) &&
           fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_TEMP_THS_REPORT_SUPPORT);
}

static inline bool iwl_mvm_is_ctdp_supported(IWLDevice *mvm)
{
    return fw_has_capa(&mvm->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_CTDP_SUPPORT);
}

struct iwl_rate_info {
    u8 plcp;    /* uCode API:  IWL_RATE_6M_PLCP, etc. */
    u8 plcp_siso;    /* uCode API:  IWL_RATE_SISO_6M_PLCP, etc. */
    u8 plcp_mimo2;    /* uCode API:  IWL_RATE_MIMO2_6M_PLCP, etc. */
    u8 plcp_mimo3;  /* uCode API:  IWL_RATE_MIMO3_6M_PLCP, etc. */
    u8 ieee;    /* MAC header:  IWL_RATE_6M_IEEE, etc. */
};

#endif /* IWLDevice_hpp */
