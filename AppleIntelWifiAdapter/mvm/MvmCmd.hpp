//
//  MvmCmd.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef MvmCmd_hpp
#define MvmCmd_hpp

#include "IWLTransport.hpp"
#include "fw-api.h"
#include "debug.h"
#include "nan.h"

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */

/**
 * enum iwl_rx_handler_context context for Rx handler
 * @RX_HANDLER_SYNC : this means that it will be called in the Rx path
 *    which can't acquire mvm->mutex.
 * @RX_HANDLER_ASYNC_LOCKED : If the handler needs to hold mvm->mutex
 *    (and only in this case!), it should be set as ASYNC. In that case,
 *    it will be called from a worker with mvm->mutex held.
 * @RX_HANDLER_ASYNC_UNLOCKED : in case the handler needs to lock the
 *    mutex itself, it will be called from a worker without mvm->mutex held.
 */
enum iwl_rx_handler_context {
    RX_HANDLER_SYNC,
    RX_HANDLER_ASYNC_LOCKED,
    RX_HANDLER_ASYNC_UNLOCKED,
};

struct iwl_rx_handlers {
    u16 cmd_id;
    enum iwl_rx_handler_context context;
    void (*fn)(struct iwl_mvm *mvm, struct iwl_rx_cmd_buffer *rxb);
};

#define RX_HANDLER(_cmd_id, _fn, _context)    \
{ .cmd_id = _cmd_id, .fn = _fn, .context = _context }
#define RX_HANDLER_GRP(_grp, _cmd, _fn, _context)    \
{ .cmd_id = WIDE_ID(_grp, _cmd), .fn = _fn, .context = _context }

/*
 * Handlers for fw notifications
 * Convention: RX_HANDLER(CMD_NAME, iwl_mvm_rx_CMD_NAME
 * This list should be in order of frequency for performance purposes.
 *
 * The handler can be one from three contexts, see &iwl_rx_handler_context
 */
static const struct iwl_rx_handlers iwl_mvm_rx_handlers[] = {
   // RX_HANDLER(TX_CMD, iwl_mvm_rx_tx_cmd, RX_HANDLER_SYNC),
   // RX_HANDLER(BA_NOTIF, iwl_mvm_rx_ba_notif, RX_HANDLER_SYNC),
    
   // RX_HANDLER_GRP(DATA_PATH_GROUP, TLC_MNG_UPDATE_NOTIF,
   //                iwl_mvm_tlc_update_notif, RX_HANDLER_SYNC),
    
    RX_HANDLER(BT_PROFILE_NOTIFICATION, IWLMvmDriver::btCoexNotif,
               RX_HANDLER_ASYNC_LOCKED),
  //  RX_HANDLER(BEACON_NOTIFICATION, iwl_mvm_rx_beacon_notif,
  //               RX_HANDLER_ASYNC_LOCKED),
  //  RX_HANDLER(STATISTICS_NOTIFICATION, iwl_mvm_rx_statistics,
  //             RX_HANDLER_ASYNC_LOCKED),
    
  //  RX_HANDLER(BA_WINDOW_STATUS_NOTIFICATION_ID,
  //             iwl_mvm_window_status_notif, RX_HANDLER_SYNC),
    
  //  RX_HANDLER(TIME_EVENT_NOTIFICATION, iwl_mvm_rx_time_event_notif,
  //             RX_HANDLER_SYNC),
  //  RX_HANDLER(MCC_CHUB_UPDATE_CMD, iwl_mvm_rx_chub_update_mcc,
  //             RX_HANDLER_ASYNC_LOCKED),
    
  //  RX_HANDLER(EOSP_NOTIFICATION, iwl_mvm_rx_eosp_notif, RX_HANDLER_SYNC),
    
  //  RX_HANDLER(SCAN_ITERATION_COMPLETE,
  //             iwl_mvm_rx_lmac_scan_iter_complete_notif, RX_HANDLER_SYNC),
  //  RX_HANDLER(SCAN_OFFLOAD_COMPLETE,
  //             iwl_mvm_rx_lmac_scan_complete_notif,
  //             RX_HANDLER_ASYNC_LOCKED),
  //  RX_HANDLER(MATCH_FOUND_NOTIFICATION, iwl_mvm_rx_scan_match_found,
    //           RX_HANDLER_SYNC),
    //RX_HANDLER(SCAN_COMPLETE_UMAC, iwl_mvm_rx_umac_scan_complete_notif,
    //           RX_HANDLER_ASYNC_LOCKED),
    //RX_HANDLER(SCAN_ITERATION_COMPLETE_UMAC,
    //           iwl_mvm_rx_umac_scan_iter_complete_notif, RX_HANDLER_SYNC),
    
    //RX_HANDLER(CARD_STATE_NOTIFICATION, iwl_mvm_rx_card_state_notif,
    //           RX_HANDLER_SYNC),
    
    //RX_HANDLER(MISSED_BEACONS_NOTIFICATION, iwl_mvm_rx_missed_beacons_notif,
    //           RX_HANDLER_SYNC),
    
    RX_HANDLER(REPLY_ERROR, IWLMvmDriver::rxFwErrorNotif, RX_HANDLER_SYNC),
    //RX_HANDLER(PSM_UAPSD_AP_MISBEHAVING_NOTIFICATION,
    //           iwl_mvm_power_uapsd_misbehaving_ap_notif, RX_HANDLER_SYNC),
    //RX_HANDLER(DTS_MEASUREMENT_NOTIFICATION, iwl_mvm_temp_notif,
    //           RX_HANDLER_ASYNC_LOCKED),
    //RX_HANDLER_GRP(PHY_OPS_GROUP, DTS_MEASUREMENT_NOTIF_WIDE,
    //               iwl_mvm_temp_notif, RX_HANDLER_ASYNC_UNLOCKED),
    //RX_HANDLER_GRP(PHY_OPS_GROUP, CT_KILL_NOTIFICATION,
    //               iwl_mvm_ct_kill_notif, RX_HANDLER_SYNC),
    
    //RX_HANDLER(TDLS_CHANNEL_SWITCH_NOTIFICATION, iwl_mvm_rx_tdls_notif,
    //           RX_HANDLER_ASYNC_LOCKED),
    RX_HANDLER(MFUART_LOAD_NOTIFICATION, IWLMvmDriver::rxMfuartNotif,
               RX_HANDLER_SYNC),
    //RX_HANDLER(TOF_NOTIFICATION, iwl_mvm_tof_resp_handler,
    //           RX_HANDLER_ASYNC_LOCKED),
    //RX_HANDLER_GRP(DEBUG_GROUP, MFU_ASSERT_DUMP_NTF,
    //               iwl_mvm_mfu_assert_dump_notif, RX_HANDLER_SYNC),
    //RX_HANDLER_GRP(PROT_OFFLOAD_GROUP, STORED_BEACON_NTF,
    //               iwl_mvm_rx_stored_beacon_notif, RX_HANDLER_SYNC),
    //RX_HANDLER_GRP(DATA_PATH_GROUP, MU_GROUP_MGMT_NOTIF,
    //               iwl_mvm_mu_mimo_grp_notif, RX_HANDLER_SYNC),
    //RX_HANDLER_GRP(DATA_PATH_GROUP, STA_PM_NOTIF,
    //               iwl_mvm_sta_pm_notif, RX_HANDLER_SYNC),
};
#undef RX_HANDLER
#undef RX_HANDLER_GRP

static const struct iwl_hcmd_names iwl_mvm_legacy_names[] = {
    HCMD_NAME(MVM_ALIVE),
    HCMD_NAME(REPLY_ERROR),
    HCMD_NAME(ECHO_CMD),
    HCMD_NAME(INIT_COMPLETE_NOTIF),
    HCMD_NAME(PHY_CONTEXT_CMD),
    HCMD_NAME(DBG_CFG),
    HCMD_NAME(SCAN_CFG_CMD),
    HCMD_NAME(SCAN_REQ_UMAC),
    HCMD_NAME(SCAN_ABORT_UMAC),
    HCMD_NAME(SCAN_COMPLETE_UMAC),
    HCMD_NAME(BA_WINDOW_STATUS_NOTIFICATION_ID),
    HCMD_NAME(ADD_STA_KEY),
    HCMD_NAME(ADD_STA),
    HCMD_NAME(REMOVE_STA),
    HCMD_NAME(FW_GET_ITEM_CMD),
    HCMD_NAME(TX_CMD),
    HCMD_NAME(SCD_QUEUE_CFG),
    HCMD_NAME(TXPATH_FLUSH),
    HCMD_NAME(MGMT_MCAST_KEY),
    HCMD_NAME(WEP_KEY),
    HCMD_NAME(SHARED_MEM_CFG),
    HCMD_NAME(TDLS_CHANNEL_SWITCH_CMD),
    HCMD_NAME(MAC_CONTEXT_CMD),
    HCMD_NAME(TIME_EVENT_CMD),
    HCMD_NAME(TIME_EVENT_NOTIFICATION),
    HCMD_NAME(BINDING_CONTEXT_CMD),
    HCMD_NAME(TIME_QUOTA_CMD),
    HCMD_NAME(NON_QOS_TX_COUNTER_CMD),
    HCMD_NAME(LEDS_CMD),
    HCMD_NAME(LQ_CMD),
    HCMD_NAME(FW_PAGING_BLOCK_CMD),
    HCMD_NAME(SCAN_OFFLOAD_REQUEST_CMD),
    HCMD_NAME(SCAN_OFFLOAD_ABORT_CMD),
    HCMD_NAME(HOT_SPOT_CMD),
    HCMD_NAME(SCAN_OFFLOAD_PROFILES_QUERY_CMD),
    HCMD_NAME(BT_COEX_UPDATE_REDUCED_TXP),
    HCMD_NAME(BT_COEX_CI),
    HCMD_NAME(PHY_CONFIGURATION_CMD),
    HCMD_NAME(CALIB_RES_NOTIF_PHY_DB),
    HCMD_NAME(PHY_DB_CMD),
    HCMD_NAME(SCAN_OFFLOAD_COMPLETE),
    HCMD_NAME(SCAN_OFFLOAD_UPDATE_PROFILES_CMD),
    HCMD_NAME(CONFIG_2G_COEX_CMD),
    HCMD_NAME(POWER_TABLE_CMD),
    HCMD_NAME(PSM_UAPSD_AP_MISBEHAVING_NOTIFICATION),
    HCMD_NAME(REPLY_THERMAL_MNG_BACKOFF),
    HCMD_NAME(DC2DC_CONFIG_CMD),
    HCMD_NAME(NVM_ACCESS_CMD),
    HCMD_NAME(BEACON_NOTIFICATION),
    HCMD_NAME(BEACON_TEMPLATE_CMD),
    HCMD_NAME(TX_ANT_CONFIGURATION_CMD),
    HCMD_NAME(BT_CONFIG),
    HCMD_NAME(STATISTICS_CMD),
    HCMD_NAME(STATISTICS_NOTIFICATION),
    HCMD_NAME(EOSP_NOTIFICATION),
    HCMD_NAME(REDUCE_TX_POWER_CMD),
    HCMD_NAME(CARD_STATE_NOTIFICATION),
    HCMD_NAME(MISSED_BEACONS_NOTIFICATION),
    HCMD_NAME(TDLS_CONFIG_CMD),
    HCMD_NAME(MAC_PM_POWER_TABLE),
    HCMD_NAME(TDLS_CHANNEL_SWITCH_NOTIFICATION),
    HCMD_NAME(MFUART_LOAD_NOTIFICATION),
    HCMD_NAME(RSS_CONFIG_CMD),
    HCMD_NAME(SCAN_ITERATION_COMPLETE_UMAC),
    HCMD_NAME(REPLY_RX_PHY_CMD),
    HCMD_NAME(REPLY_RX_MPDU_CMD),
    HCMD_NAME(BAR_FRAME_RELEASE),
    HCMD_NAME(FRAME_RELEASE),
    HCMD_NAME(BA_NOTIF),
    HCMD_NAME(MCC_UPDATE_CMD),
    HCMD_NAME(MCC_CHUB_UPDATE_CMD),
    HCMD_NAME(MARKER_CMD),
    HCMD_NAME(BT_PROFILE_NOTIFICATION),
    HCMD_NAME(BCAST_FILTER_CMD),
    HCMD_NAME(MCAST_FILTER_CMD),
    HCMD_NAME(REPLY_SF_CFG_CMD),
    HCMD_NAME(REPLY_BEACON_FILTERING_CMD),
    HCMD_NAME(D3_CONFIG_CMD),
    HCMD_NAME(PROT_OFFLOAD_CONFIG_CMD),
    HCMD_NAME(OFFLOADS_QUERY_CMD),
    HCMD_NAME(REMOTE_WAKE_CONFIG_CMD),
    HCMD_NAME(MATCH_FOUND_NOTIFICATION),
    HCMD_NAME(DTS_MEASUREMENT_NOTIFICATION),
    HCMD_NAME(WOWLAN_PATTERNS),
    HCMD_NAME(WOWLAN_CONFIGURATION),
    HCMD_NAME(WOWLAN_TSC_RSC_PARAM),
    HCMD_NAME(WOWLAN_TKIP_PARAM),
    HCMD_NAME(WOWLAN_KEK_KCK_MATERIAL),
    HCMD_NAME(WOWLAN_GET_STATUSES),
    HCMD_NAME(SCAN_ITERATION_COMPLETE),
    HCMD_NAME(D0I3_END_CMD),
    HCMD_NAME(LTR_CONFIG),
    HCMD_NAME(LDBG_CONFIG_CMD),
    HCMD_NAME(DEBUG_LOG_MSG),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_system_names[] = {
    HCMD_NAME(SHARED_MEM_CFG_CMD),
    HCMD_NAME(SOC_CONFIGURATION_CMD),
    HCMD_NAME(INIT_EXTENDED_CFG_CMD),
    HCMD_NAME(FW_ERROR_RECOVERY_CMD),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_mac_conf_names[] = {
    HCMD_NAME(CHANNEL_SWITCH_TIME_EVENT_CMD),
    HCMD_NAME(SESSION_PROTECTION_CMD),
    HCMD_NAME(SESSION_PROTECTION_NOTIF),
    HCMD_NAME(CHANNEL_SWITCH_NOA_NOTIF),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_phy_names[] = {
    HCMD_NAME(CMD_DTS_MEASUREMENT_TRIGGER_WIDE),
    HCMD_NAME(CTDP_CONFIG_CMD),
    HCMD_NAME(TEMP_REPORTING_THRESHOLDS_CMD),
    HCMD_NAME(GEO_TX_POWER_LIMIT),
    HCMD_NAME(CT_KILL_NOTIFICATION),
    HCMD_NAME(DTS_MEASUREMENT_NOTIF_WIDE),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_data_path_names[] = {
    HCMD_NAME(DQA_ENABLE_CMD),
    HCMD_NAME(UPDATE_MU_GROUPS_CMD),
    HCMD_NAME(TRIGGER_RX_QUEUES_NOTIF_CMD),
    HCMD_NAME(STA_HE_CTXT_CMD),
    HCMD_NAME(RFH_QUEUE_CONFIG_CMD),
    HCMD_NAME(TLC_MNG_CONFIG_CMD),
    HCMD_NAME(CHEST_COLLECTOR_FILTER_CONFIG_CMD),
    HCMD_NAME(STA_PM_NOTIF),
    HCMD_NAME(MU_GROUP_MGMT_NOTIF),
    HCMD_NAME(RX_QUEUES_NOTIFICATION),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_debug_names[] = {
    HCMD_NAME(DBGC_SUSPEND_RESUME),
    HCMD_NAME(BUFFER_ALLOCATION),
    HCMD_NAME(MFU_ASSERT_DUMP_NTF),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_nan_names[] = {
    HCMD_NAME(NAN_CONFIG_CMD),
    HCMD_NAME(NAN_DISCOVERY_FUNC_CMD),
    HCMD_NAME(NAN_FAW_CONFIG_CMD),
    HCMD_NAME(NAN_DISCOVERY_EVENT_NOTIF),
    HCMD_NAME(NAN_DISCOVERY_TERMINATE_NOTIF),
    HCMD_NAME(NAN_FAW_START_NOTIF),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_location_names[] = {
    HCMD_NAME(TOF_RANGE_REQ_CMD),
    HCMD_NAME(TOF_CONFIG_CMD),
    HCMD_NAME(TOF_RANGE_ABORT_CMD),
    HCMD_NAME(TOF_RANGE_REQ_EXT_CMD),
    HCMD_NAME(TOF_RESPONDER_CONFIG_CMD),
    HCMD_NAME(TOF_RESPONDER_DYN_CONFIG_CMD),
    HCMD_NAME(TOF_LC_NOTIF),
    HCMD_NAME(TOF_RESPONDER_STATS),
    HCMD_NAME(TOF_MCSI_DEBUG_NOTIF),
    HCMD_NAME(TOF_RANGE_RESPONSE_NOTIF),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_prot_offload_names[] = {
    HCMD_NAME(STORED_BEACON_NTF),
};

/* Please keep this array *SORTED* by hex value.
 * Access is done through binary search
 */
static const struct iwl_hcmd_names iwl_mvm_regulatory_and_nvm_names[] = {
    HCMD_NAME(NVM_ACCESS_COMPLETE),
    HCMD_NAME(NVM_GET_INFO),
};

static const struct iwl_hcmd_arr iwl_mvm_groups[] = {
    [LEGACY_GROUP] = HCMD_ARR(iwl_mvm_legacy_names),
    [LONG_GROUP] = HCMD_ARR(iwl_mvm_legacy_names),
    [SYSTEM_GROUP] = HCMD_ARR(iwl_mvm_system_names),
    [MAC_CONF_GROUP] = HCMD_ARR(iwl_mvm_mac_conf_names),
    [PHY_OPS_GROUP] = HCMD_ARR(iwl_mvm_phy_names),
    [DATA_PATH_GROUP] = HCMD_ARR(iwl_mvm_data_path_names),
    [NAN_GROUP] = HCMD_ARR(iwl_mvm_nan_names),
    [LOCATION_GROUP] = HCMD_ARR(iwl_mvm_location_names),
    [PROT_OFFLOAD_GROUP] = HCMD_ARR(iwl_mvm_prot_offload_names),
    [REGULATORY_AND_NVM_GROUP] =
        HCMD_ARR(iwl_mvm_regulatory_and_nvm_names),
};

#endif /* MvmCmd_hpp */
