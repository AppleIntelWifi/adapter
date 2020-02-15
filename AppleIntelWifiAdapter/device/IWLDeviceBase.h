//
//  IWLDeviceBase.h
//  AppleIntelWifiAdapter
//
//  Created by qcwap on 2020/1/5.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLDeviceBase_h
#define IWLDeviceBase_h

#include <libkern/OSTypes.h>
#include <IOKit/IOLib.h>
#include <linux/types.h>
#include <linux/nl80211.h>
#include <linux/ieee80211.h>
#include <linux/kernel.h>
#include "IWLPRPH.h"
#include "IWLCSR.h"
#include "FWFile.h"

#define CPTCFG_IWL_TIMEOUT_FACTOR 1
#define HZ 1000

#define IWL_MAX_PLCP_ERR_LONG_THRESHOLD_DEF    100
#define IWL_MAX_PLCP_ERR_EXT_LONG_THRESHOLD_DEF    200
#define IWL_MAX_PLCP_ERR_THRESHOLD_MAX        255
#define IWL_MAX_PLCP_ERR_THRESHOLD_DISABLE    0

/* TX queue watchdog timeouts in mSecs */
#define IWL_WATCHDOG_DISABLED    0
#define IWL_DEF_WD_TIMEOUT    (2500 * CPTCFG_IWL_TIMEOUT_FACTOR)
#define IWL_LONG_WD_TIMEOUT    (10000 * CPTCFG_IWL_TIMEOUT_FACTOR)
#define IWL_MAX_WD_TIMEOUT    (120000 * CPTCFG_IWL_TIMEOUT_FACTOR)

#define IWL_DEFAULT_MAX_TX_POWER 22
#define IWL_TX_CSUM_NETIF_FLAGS (NETIF_F_IPV6_CSUM | NETIF_F_IP_CSUM |\
                 NETIF_F_TSO | NETIF_F_TSO6)

/* Antenna presence definitions */
#define    ANT_NONE    0x0
#define    ANT_INVALID    0xff
#define    ANT_A        BIT(0)
#define    ANT_B        BIT(1)
#define ANT_C        BIT(2)
#define    ANT_AB        (ANT_A | ANT_B)
#define    ANT_AC        (ANT_A | ANT_C)
#define ANT_BC        (ANT_B | ANT_C)
#define ANT_ABC        (ANT_A | ANT_B | ANT_C)
#define MAX_ANT_NUM 3

static inline u8 num_of_ant(u8 mask)
{
    return  !!((mask) & ANT_A) +
        !!((mask) & ANT_B) +
        !!((mask) & ANT_C);
}

/*
 * information on how to parse the EEPROM
 */
#define EEPROM_REG_BAND_1_CHANNELS        0x08
#define EEPROM_REG_BAND_2_CHANNELS        0x26
#define EEPROM_REG_BAND_3_CHANNELS        0x42
#define EEPROM_REG_BAND_4_CHANNELS        0x5C
#define EEPROM_REG_BAND_5_CHANNELS        0x74
#define EEPROM_REG_BAND_24_HT40_CHANNELS    0x82
#define EEPROM_REG_BAND_52_HT40_CHANNELS    0x92
#define EEPROM_6000_REG_BAND_24_HT40_CHANNELS    0x80
#define EEPROM_REGULATORY_BAND_NO_HT40        0

/* lower blocks contain EEPROM image and calibration data */
#define OTP_LOW_IMAGE_SIZE_2K        (2 * 512 * sizeof(uint16_t))  /*  2 KB */
#define OTP_LOW_IMAGE_SIZE_16K        (16 * 512 * sizeof(uint16_t)) /* 16 KB */
#define OTP_LOW_IMAGE_SIZE_32K        (32 * 512 * sizeof(uint16_t)) /* 32 KB */

/**
 * struct iwl_base_params - params not likely to change within a device family
 * @max_ll_items: max number of OTP blocks
 * @shadow_ram_support: shadow support for OTP memory
 * @led_compensation: compensate on the led on/off time per HW according
 *    to the deviation to achieve the desired led frequency.
 *    The detail algorithm is described in iwl-led.c
 * @wd_timeout: TX queues watchdog timeout
 * @max_event_log_size: size of event log buffer size for ucode event logging
 * @shadow_reg_enable: HW shadow register support
 * @apmg_wake_up_wa: should the MAC access REQ be asserted when a command
 *    is in flight. This is due to a HW bug in 7260, 3160 and 7265.
 * @scd_chain_ext_wa: should the chain extension feature in SCD be disabled.
 * @max_tfd_queue_size: max number of entries in tfd queue.
 */
struct iwl_base_params {
    unsigned int wd_timeout;

    u16 eeprom_size;
    u16 max_event_log_size;

    u8 pll_cfg:1, /* for iwl_pcie_apm_init() */
       shadow_ram_support:1,
       shadow_reg_enable:1,
       pcie_l1_allowed:1,
       apmg_wake_up_wa:1,
       scd_chain_ext_wa:1;

    u16 num_of_queues;    /* def: HW dependent */
    u32 max_tfd_queue_size;    /* def: HW dependent */

    u8 max_ll_items;
    u8 led_compensation;
};

#define TT_TX_BACKOFF_SIZE 6

/*
 * Tx-backoff threshold
 * @temperature: The threshold in Celsius
 * @backoff: The tx-backoff in uSec
 */
struct iwl_tt_tx_backoff {
    s32 temperature;
    u32 backoff;
};

/**
 * struct iwl_tt_params - thermal throttling parameters
 * @ct_kill_entry: CT Kill entry threshold
 * @ct_kill_exit: CT Kill exit threshold
 * @ct_kill_duration: The time  intervals (in uSec) in which the driver needs
 *    to checks whether to exit CT Kill.
 * @dynamic_smps_entry: Dynamic SMPS entry threshold
 * @dynamic_smps_exit: Dynamic SMPS exit threshold
 * @tx_protection_entry: TX protection entry threshold
 * @tx_protection_exit: TX protection exit threshold
 * @tx_backoff: Array of thresholds for tx-backoff , in ascending order.
 * @support_ct_kill: Support CT Kill?
 * @support_dynamic_smps: Support dynamic SMPS?
 * @support_tx_protection: Support tx protection?
 * @support_tx_backoff: Support tx-backoff?
 */
struct iwl_tt_params {
    u32 ct_kill_entry;
    u32 ct_kill_exit;
    u32 ct_kill_duration;
    u32 dynamic_smps_entry;
    u32 dynamic_smps_exit;
    u32 tx_protection_entry;
    u32 tx_protection_exit;
    struct iwl_tt_tx_backoff tx_backoff[TT_TX_BACKOFF_SIZE];
    u8 support_ct_kill:1,
       support_dynamic_smps:1,
       support_tx_protection:1,
       support_tx_backoff:1;
};

/*
 * @stbc: support Tx STBC and 1*SS Rx STBC
 * @ldpc: support Tx/Rx with LDPC
 * @use_rts_for_aggregation: use rts/cts protection for HT traffic
 * @ht40_bands: bitmap of bands (using %NL80211_BAND_*) that support HT40
 */
struct iwl_ht_params {
    u8 ht_greenfield_support:1,
       stbc:1,
       ldpc:1,
       use_rts_for_aggregation:1;
    u8 ht40_bands;
};

/*
 * LED mode
 *    IWL_LED_DEFAULT:  use device default
 *    IWL_LED_RF_STATE: turn LED on/off based on RF state
 *            LED ON  = RF ON
 *            LED OFF = RF OFF
 *    IWL_LED_BLINK:    adjust led blink rate based on blink table
 *    IWL_LED_DISABLE:    led disabled
 */
enum iwl_led_mode {
    IWL_LED_DEFAULT,
    IWL_LED_RF_STATE,
    IWL_LED_BLINK,
    IWL_LED_DISABLE,
};

/**
 * enum iwl_nvm_type - nvm formats
 * @IWL_NVM: the regular format
 * @IWL_NVM_EXT: extended NVM format
 * @IWL_NVM_SDP: NVM format used by 3168 series
 */
enum iwl_nvm_type {
    IWL_NVM,
    IWL_NVM_EXT,
    IWL_NVM_SDP,
};

/**
 * struct iwl_csr_params
 *
 * @flag_sw_reset: reset the device
 * @flag_mac_clock_ready:
 *    Indicates MAC (ucode processor, etc.) is powered up and can run.
 *    Internal resources are accessible.
 *    NOTE:  This does not indicate that the processor is actually running.
 *    NOTE:  This does not indicate that device has completed
 *           init or post-power-down restore of internal SRAM memory.
 *           Use CSR_UCODE_DRV_GP1_BIT_MAC_SLEEP as indication that
 *           SRAM is restored and uCode is in normal operation mode.
 *           This note is relevant only for pre 5xxx devices.
 *    NOTE:  After device reset, this bit remains "0" until host sets
 *           INIT_DONE
 * @flag_init_done: Host sets this to put device into fully operational
 *    D0 power mode. Host resets this after SW_RESET to put device into
 *    low power mode.
 * @flag_mac_access_req: Host sets this to request and maintain MAC wakeup,
 *    to allow host access to device-internal resources. Host must wait for
 *    mac_clock_ready (and !GOING_TO_SLEEP) before accessing non-CSR device
 *    registers.
 * @flag_val_mac_access_en: mac access is enabled
 * @flag_master_dis: disable master
 * @flag_stop_master: stop master
 * @addr_sw_reset: address for resetting the device
 * @mac_addr0_otp: first part of MAC address from OTP
 * @mac_addr1_otp: second part of MAC address from OTP
 * @mac_addr0_strap: first part of MAC address from strap
 * @mac_addr1_strap: second part of MAC address from strap
 */
struct iwl_csr_params {
    uint8_t flag_sw_reset;
    uint8_t flag_mac_clock_ready;
    uint8_t flag_init_done;
    uint8_t flag_mac_access_req;
    uint8_t flag_val_mac_access_en;
    uint8_t flag_master_dis;
    uint8_t flag_stop_master;
    uint8_t addr_sw_reset;
    uint32_t mac_addr0_otp;
    uint32_t mac_addr1_otp;
    uint32_t mac_addr0_strap;
    uint32_t mac_addr1_strap;
};

const struct iwl_csr_params iwl_csr_v1 = {
    .flag_mac_clock_ready = 0,
    .flag_val_mac_access_en = 0,
    .flag_init_done = 2,
    .flag_mac_access_req = 3,
    .flag_sw_reset = 7,
    .flag_master_dis = 8,
    .flag_stop_master = 9,
    .addr_sw_reset = CSR_BASE + 0x020,
    .mac_addr0_otp = 0x380,
    .mac_addr1_otp = 0x384,
    .mac_addr0_strap = 0x388,
    .mac_addr1_strap = 0x38C
};

const struct iwl_csr_params iwl_csr_v2 = {
    .flag_init_done = 6,
    .flag_mac_clock_ready = 20,
    .flag_val_mac_access_en = 20,
    .flag_mac_access_req = 21,
    .flag_master_dis = 28,
    .flag_stop_master = 29,
    .flag_sw_reset = 31,
    .addr_sw_reset = CSR_BASE + 0x024,
    .mac_addr0_otp = 0x30,
    .mac_addr1_otp = 0x34,
    .mac_addr0_strap = 0x38,
    .mac_addr1_strap = 0x3C
};

enum iwl_device_family {
    IWL_DEVICE_FAMILY_UNDEFINED,
    IWL_DEVICE_FAMILY_1000,
    IWL_DEVICE_FAMILY_100,
    IWL_DEVICE_FAMILY_2000,
    IWL_DEVICE_FAMILY_2030,
    IWL_DEVICE_FAMILY_105,
    IWL_DEVICE_FAMILY_135,
    IWL_DEVICE_FAMILY_5000,
    IWL_DEVICE_FAMILY_5150,
    IWL_DEVICE_FAMILY_6000,
    IWL_DEVICE_FAMILY_6000i,
    IWL_DEVICE_FAMILY_6005,
    IWL_DEVICE_FAMILY_6030,
    IWL_DEVICE_FAMILY_6050,
    IWL_DEVICE_FAMILY_6150,
    IWL_DEVICE_FAMILY_7000,
    IWL_DEVICE_FAMILY_8000,
    IWL_DEVICE_FAMILY_9000,
    IWL_DEVICE_FAMILY_22000,
    IWL_DEVICE_FAMILY_AX210,
};

/**
 * struct iwl_cfg_trans - information needed to start the trans
 *
 * These values are specific to the device ID and do not change when
 * multiple configs are used for a single device ID.  They values are
 * used, among other things, to boot the NIC so that the HW REV or
 * RFID can be read before deciding the remaining parameters to use.
 *
 * @base_params: pointer to basic parameters
 * @csr: csr flags and addresses that are different across devices
 * @device_family: the device family
 * @umac_prph_offset: offset to add to UMAC periphery address
 * @xtal_latency: power up latency to get the xtal stabilized
 * @extra_phy_cfg_flags: extra configuration flags to pass to the PHY
 * @rf_id: need to read rf_id to determine the firmware image
 * @use_tfh: use TFH
 * @gen2: 22000 and on transport operation
 * @mq_rx_supported: multi-queue rx support
 * @integrated: discrete or integrated
 * @low_latency_xtal: use the low latency xtal if supported
 */
struct iwl_cfg_trans_params {
    const struct iwl_base_params *base_params;
    enum iwl_device_family device_family;
    u32 umac_prph_offset;
    u32 xtal_latency;
    u32 extra_phy_cfg_flags;
    u32 rf_id:1,
        use_tfh:1,
        gen2:1,
        mq_rx_supported:1,
        integrated:1,
        low_latency_xtal:1,
        bisr_workaround:1;
};

/**
 * struct iwl_fw_mon_reg - FW monitor register info
 * @addr: register address
 * @mask: register mask
 */
struct iwl_fw_mon_reg {
    u32 addr;
    u32 mask;
};

/**
 * struct iwl_fw_mon_regs - FW monitor registers
 * @write_ptr: write pointer register
 * @cycle_cnt: cycle count register
 * @cur_frag: current fragment in use
 */
struct iwl_fw_mon_regs {
    struct iwl_fw_mon_reg write_ptr;
    struct iwl_fw_mon_reg cycle_cnt;
    struct iwl_fw_mon_reg cur_frag;
};

struct iwl_eeprom_params {
    const uint8_t regulatory_bands[7];
    bool enhanced_txpower;
};

/* Tx-backoff power threshold
 * @pwr: The power limit in mw
 * @backoff: The tx-backoff in uSec
 */
struct iwl_pwr_tx_backoff {
    uint32_t pwr;
    uint32_t backoff;
};

/**
 * struct iwl_cfg
 * @trans: the trans-specific configuration part
 * @name: Official name of the device
 * @fw_name_pre: Firmware filename prefix. The api version and extension
 *    (.ucode) will be added to filename before loading from disk. The
 *    filename is constructed as fw_name_pre<api>.ucode.
 * @ucode_api_max: Highest version of uCode API supported by driver.
 * @ucode_api_min: Lowest version of uCode API supported by driver.
 * @max_inst_size: The maximal length of the fw inst section (only DVM)
 * @max_data_size: The maximal length of the fw data section (only DVM)
 * @valid_tx_ant: valid transmit antenna
 * @valid_rx_ant: valid receive antenna
 * @non_shared_ant: the antenna that is for WiFi only
 * @nvm_ver: NVM version
 * @nvm_calib_ver: NVM calibration version
 * @lib: pointer to the lib ops
 * @ht_params: point to ht parameters
 * @led_mode: 0=blinking, 1=On(RF On)/Off(RF Off)
 * @rx_with_siso_diversity: 1x1 device with rx antenna diversity
 * @tx_with_siso_diversity: 1x1 device with tx antenna diversity
 * @internal_wimax_coex: internal wifi/wimax combo device
 * @high_temp: Is this NIC is designated to be in high temperature.
 * @host_interrupt_operation_mode: device needs host interrupt operation
 *    mode set
 * @nvm_hw_section_num: the ID of the HW NVM section
 * @mac_addr_from_csr: read HW address from CSR registers
 * @features: hw features, any combination of feature_whitelist
 * @pwr_tx_backoffs: translation table between power limits and backoffs
 * @max_tx_agg_size: max TX aggregation size of the ADDBA request/response
 * @max_ht_ampdu_factor: the exponent of the max length of A-MPDU that the
 *    station can receive in HT
 * @max_vht_ampdu_exponent: the exponent of the max length of A-MPDU that the
 *    station can receive in VHT
 * @dccm_offset: offset from which DCCM begins
 * @dccm_len: length of DCCM (including runtime stack CCM)
 * @dccm2_offset: offset from which the second DCCM begins
 * @dccm2_len: length of the second DCCM
 * @smem_offset: offset from which the SMEM begins
 * @smem_len: the length of SMEM
 * @vht_mu_mimo_supported: VHT MU-MIMO support
 * @cdb: CDB support
 * @nvm_type: see &enum iwl_nvm_type
 * @d3_debug_data_base_addr: base address where D3 debug data is stored
 * @d3_debug_data_length: length of the D3 debug data
 * @bisr_workaround: BISR hardware workaround (for 22260 series devices)
 * @min_txq_size: minimum number of slots required in a TX queue
 * @uhb_supported: ultra high band channels supported
 * @min_256_ba_txq_size: minimum number of slots required in a TX queue which
 *    supports 256 BA aggregation
 * @num_rbds: number of receive buffer descriptors to use
 *    (only used for multi-queue capable devices)
 *
 * We enable the driver to be backward compatible wrt. hardware features.
 * API differences in uCode shouldn't be handled here but through TLVs
 * and/or the uCode API version instead.
 */
struct iwl_cfg {
    struct iwl_cfg_trans_params trans;
    /* params specific to an individual device within a device family */
    const char *name;
    const char *fw_name_pre;
    /* params likely to change within a device family */
    const struct iwl_ht_params *ht_params;
    const struct iwl_eeprom_params *eeprom_params;
    const struct iwl_pwr_tx_backoff *pwr_tx_backoffs;
    const char *default_nvm_file_C_step;
    const struct iwl_tt_params *thermal_params;
    enum iwl_led_mode led_mode;
    enum iwl_nvm_type nvm_type;
    u32 max_data_size;
    u32 max_inst_size;
    netdev_features_t features;
    u32 dccm_offset;
    u32 dccm_len;
    u32 dccm2_offset;
    u32 dccm2_len;
    u32 smem_offset;
    u32 smem_len;
    u16 nvm_ver;
    u16 nvm_calib_ver;
    u32 rx_with_siso_diversity:1,
        tx_with_siso_diversity:1,
        bt_shared_single_ant:1,
        internal_wimax_coex:1,
        host_interrupt_operation_mode:1,
        high_temp:1,
        mac_addr_from_csr:1,
        lp_xtal_workaround:1,
        disable_dummy_notification:1,
        apmg_not_supported:1,
        vht_mu_mimo_supported:1,
        cdb:1,
        dbgc_supported:1,
        uhb_supported:1;
    u8 valid_tx_ant;
    u8 valid_rx_ant;
    u8 non_shared_ant;
    u8 nvm_hw_section_num;
    u8 max_tx_agg_size;
    u8 max_ht_ampdu_exponent;
    u8 max_vht_ampdu_exponent;
    u8 ucode_api_max;
    u8 ucode_api_min;
    u16 num_rbds;
    u32 min_umac_error_event_table;
    u32 d3_debug_data_base_addr;
    u32 d3_debug_data_length;
    u32 min_txq_size;
    u32 gp2_reg_addr;
    u32 min_256_ba_txq_size;
    const struct iwl_fw_mon_regs mon_dram_regs;
    const struct iwl_fw_mon_regs mon_smem_regs;
};

#endif /* IWLDeviceBase_h */
