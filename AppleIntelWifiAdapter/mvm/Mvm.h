//
//  Mvm.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef Mvm_h
#define Mvm_h

#define MVM_UCODE_ALIVE_TIMEOUT    (HZ * CPTCFG_IWL_TIMEOUT_FACTOR)
#define MVM_UCODE_CALIB_TIMEOUT    (2 * HZ * CPTCFG_IWL_TIMEOUT_FACTOR)

#define UCODE_VALID_OK    cpu_to_le32(0x1)

struct iwl_mvm_alive_data {
    bool valid;
    u32 scd_base_addr;
};

/**
 * enum iwl_mvm_status - MVM status bits
 * @IWL_MVM_STATUS_HW_RFKILL: HW RF-kill is asserted
 * @IWL_MVM_STATUS_HW_CTKILL: CT-kill is active
 * @IWL_MVM_STATUS_ROC_RUNNING: remain-on-channel is running
 * @IWL_MVM_STATUS_HW_RESTART_REQUESTED: HW restart was requested
 * @IWL_MVM_STATUS_IN_HW_RESTART: HW restart is active
 * @IWL_MVM_STATUS_ROC_AUX_RUNNING: AUX remain-on-channel is running
 * @IWL_MVM_STATUS_FIRMWARE_RUNNING: firmware is running
 * @IWL_MVM_STATUS_NEED_FLUSH_P2P: need to flush P2P bcast STA
 */
enum iwl_mvm_status {
    IWL_MVM_STATUS_HW_RFKILL,
    IWL_MVM_STATUS_HW_CTKILL,
    IWL_MVM_STATUS_ROC_RUNNING,
    IWL_MVM_STATUS_HW_RESTART_REQUESTED,
    IWL_MVM_STATUS_IN_HW_RESTART,
    IWL_MVM_STATUS_ROC_AUX_RUNNING,
    IWL_MVM_STATUS_FIRMWARE_RUNNING,
    IWL_MVM_STATUS_NEED_FLUSH_P2P,
};

/* Keep track of completed init configuration */
enum iwl_mvm_init_status {
    IWL_MVM_INIT_STATUS_THERMAL_INIT_COMPLETE = BIT(0),
    IWL_MVM_INIT_STATUS_LEDS_INIT_COMPLETE = BIT(1),
};

#define IWL_MVM_SCAN_STOPPING_SHIFT    8

enum iwl_scan_status {
    IWL_MVM_SCAN_REGULAR        = BIT(0),
    IWL_MVM_SCAN_SCHED        = BIT(1),
    IWL_MVM_SCAN_NETDETECT        = BIT(2),

    IWL_MVM_SCAN_STOPPING_REGULAR    = BIT(8),
    IWL_MVM_SCAN_STOPPING_SCHED    = BIT(9),
    IWL_MVM_SCAN_STOPPING_NETDETECT    = BIT(10),

    IWL_MVM_SCAN_REGULAR_MASK    = IWL_MVM_SCAN_REGULAR |
                      IWL_MVM_SCAN_STOPPING_REGULAR,
    IWL_MVM_SCAN_SCHED_MASK        = IWL_MVM_SCAN_SCHED |
                      IWL_MVM_SCAN_STOPPING_SCHED,
    IWL_MVM_SCAN_NETDETECT_MASK    = IWL_MVM_SCAN_NETDETECT |
                      IWL_MVM_SCAN_STOPPING_NETDETECT,

    IWL_MVM_SCAN_STOPPING_MASK    = 0xff << IWL_MVM_SCAN_STOPPING_SHIFT,
    IWL_MVM_SCAN_MASK        = 0xff,
};

enum iwl_mvm_scan_type {
    IWL_SCAN_TYPE_NOT_SET,
    IWL_SCAN_TYPE_UNASSOC,
    IWL_SCAN_TYPE_WILD,
    IWL_SCAN_TYPE_MILD,
    IWL_SCAN_TYPE_FRAGMENTED,
    IWL_SCAN_TYPE_FAST_BALANCE,
};

enum iwl_mvm_sched_scan_pass_all_states {
    SCHED_SCAN_PASS_ALL_DISABLED,
    SCHED_SCAN_PASS_ALL_ENABLED,
    SCHED_SCAN_PASS_ALL_FOUND,
};

#ifdef CONFIG_THERMAL
/**
 *struct iwl_mvm_thermal_device - thermal zone related data
 * @temp_trips: temperature thresholds for report
 * @fw_trips_index: keep indexes to original array - temp_trips
 * @tzone: thermal zone device data
*/
struct iwl_mvm_thermal_device {
    s16 temp_trips[IWL_MAX_DTS_TRIPS];
    u8 fw_trips_index[IWL_MAX_DTS_TRIPS];
    struct thermal_zone_device *tzone;
};

/*
 * struct iwl_mvm_cooling_device
 * @cur_state: current state
 * @cdev: struct thermal cooling device
 */
struct iwl_mvm_cooling_device {
    u32 cur_state;
    struct thermal_cooling_device *cdev;
};
#endif

#define IWL_MVM_NUM_LAST_FRAMES_UCODE_RATES 8

struct iwl_mvm_frame_stats {
    u32 legacy_frames;
    u32 ht_frames;
    u32 vht_frames;
    u32 bw_20_frames;
    u32 bw_40_frames;
    u32 bw_80_frames;
    u32 bw_160_frames;
    u32 sgi_frames;
    u32 ngi_frames;
    u32 siso_frames;
    u32 mimo2_frames;
    u32 agg_frames;
    u32 ampdu_count;
    u32 success_frames;
    u32 fail_frames;
    u32 last_rates[IWL_MVM_NUM_LAST_FRAMES_UCODE_RATES];
    int last_frame_idx;
};

#define IWL_MVM_DEBUG_SET_TEMPERATURE_DISABLE 0xff
#define IWL_MVM_DEBUG_SET_TEMPERATURE_MIN -100
#define IWL_MVM_DEBUG_SET_TEMPERATURE_MAX 200

enum iwl_mvm_tdls_cs_state {
    IWL_MVM_TDLS_SW_IDLE = 0,
    IWL_MVM_TDLS_SW_REQ_SENT,
    IWL_MVM_TDLS_SW_RESP_RCVD,
    IWL_MVM_TDLS_SW_REQ_RCVD,
    IWL_MVM_TDLS_SW_ACTIVE,
};

enum iwl_mvm_traffic_load {
    IWL_MVM_TRAFFIC_LOW,
    IWL_MVM_TRAFFIC_MEDIUM,
    IWL_MVM_TRAFFIC_HIGH,
};

/*
 * enum iwl_mvm_queue_status - queue status
 * @IWL_MVM_QUEUE_FREE: the queue is not allocated nor reserved
 *    Basically, this means that this queue can be used for any purpose
 * @IWL_MVM_QUEUE_RESERVED: queue is reserved but not yet in use
 *    This is the state of a queue that has been dedicated for some RATID
 *    (agg'd or not), but that hasn't yet gone through the actual enablement
 *    of iwl_mvm_enable_txq(), and therefore no traffic can go through it yet.
 *    Note that in this state there is no requirement to already know what TID
 *    should be used with this queue, it is just marked as a queue that will
 *    be used, and shouldn't be allocated to anyone else.
 * @IWL_MVM_QUEUE_READY: queue is ready to be used
 *    This is the state of a queue that has been fully configured (including
 *    SCD pointers, etc), has a specific RA/TID assigned to it, and can be
 *    used to send traffic.
 * @IWL_MVM_QUEUE_SHARED: queue is shared, or in a process of becoming shared
 *    This is a state in which a single queue serves more than one TID, all of
 *    which are not aggregated. Note that the queue is only associated to one
 *    RA.
 */
enum iwl_mvm_queue_status {
    IWL_MVM_QUEUE_FREE,
    IWL_MVM_QUEUE_RESERVED,
    IWL_MVM_QUEUE_READY,
    IWL_MVM_QUEUE_SHARED,
};

enum iwl_bt_force_ant_mode {
    BT_FORCE_ANT_DIS = 0,
    BT_FORCE_ANT_AUTO,
    BT_FORCE_ANT_BT,
    BT_FORCE_ANT_WIFI,

    BT_FORCE_ANT_MAX,
};

/**
 * struct iwl_phy_cfg_cmd - Phy configuration command
 * @phy_cfg: PHY configuration value, uses &enum iwl_fw_phy_cfg
 * @calib_control: calibration control data
 */
struct iwl_phy_cfg_cmd {
    __le32    phy_cfg;
    struct iwl_calib_ctrl calib_control;
} __packed;

#define PHY_CFG_RADIO_TYPE    (BIT(0) | BIT(1))
#define PHY_CFG_RADIO_STEP    (BIT(2) | BIT(3))
#define PHY_CFG_RADIO_DASH    (BIT(4) | BIT(5))
#define PHY_CFG_PRODUCT_NUMBER    (BIT(6) | BIT(7))
#define PHY_CFG_TX_CHAIN_A    BIT(8)
#define PHY_CFG_TX_CHAIN_B    BIT(9)
#define PHY_CFG_TX_CHAIN_C    BIT(10)
#define PHY_CFG_RX_CHAIN_A    BIT(12)
#define PHY_CFG_RX_CHAIN_B    BIT(13)
#define PHY_CFG_RX_CHAIN_C    BIT(14)

#define IWL_MVM_DQA_QUEUE_TIMEOUT    (5 * HZ)
#define IWL_MVM_INVALID_QUEUE        0xFFFF

#define IWL_MVM_NUM_CIPHERS             10

#endif /* Mvm_h */
