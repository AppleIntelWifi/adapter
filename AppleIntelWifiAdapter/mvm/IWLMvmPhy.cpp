//
//  IWLMvmPhy.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/19/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmPhy.hpp"

/* Channel info utils */
static inline bool iwl_mvm_has_ultra_hb_channel(IWLMvmDriver *mvm)
{
    return fw_has_capa(&mvm->m_pDevice->fw.ucode_capa,
               IWL_UCODE_TLV_CAPA_ULTRA_HB_CHANNELS);
}

static inline void *iwl_mvm_chan_info_cmd_tail(IWLMvmDriver* mvm,
                           struct iwl_fw_channel_info *ci)
{
    return (u8 *)ci + (iwl_mvm_has_ultra_hb_channel(mvm) ?
               sizeof(struct iwl_fw_channel_info) :
               sizeof(struct iwl_fw_channel_info_v1));
}

static inline size_t iwl_mvm_chan_info_padding(IWLMvmDriver* mvm)
{
    return iwl_mvm_has_ultra_hb_channel(mvm) ? 0 :
        sizeof(struct iwl_fw_channel_info) -
        sizeof(struct iwl_fw_channel_info_v1);
}

static inline void iwl_mvm_set_chan_info(IWLMvmDriver* mvm,
                     struct iwl_fw_channel_info *ci,
                     u32 chan, u8 band, u8 width,
                     u8 ctrl_pos)
{
    if (iwl_mvm_has_ultra_hb_channel(mvm)) {
        ci->channel = cpu_to_le32(chan);
        ci->band = band;
        ci->width = width;
        ci->ctrl_pos = ctrl_pos;
    } else {
        struct iwl_fw_channel_info_v1 *ci_v1 =
                    (struct iwl_fw_channel_info_v1 *)ci;

        ci_v1->channel = chan;
        ci_v1->band = band;
        ci_v1->width = width;
        ci_v1->ctrl_pos = ctrl_pos;
    }
}

int
iwl_phy_ctxt_add(IWLMvmDriver* drv, struct iwl_phy_ctx *ctxt,
    struct ieee80211_channel *chan,
                 uint8_t chains_static, uint8_t chains_dynamic) {
    ctxt->channel = chan;
    
    return iwl_phy_ctxt_apply(drv, ctxt,
        chains_static, chains_dynamic, FW_CTXT_ACTION_ADD, 0);
}

int
iwl_phy_ctxt_changed(IWLMvmDriver* drv,
    struct iwl_phy_ctx *ctxt, struct ieee80211_channel *chan,
                     uint8_t chains_static, uint8_t chains_dynamic) {
    ctxt->channel = chan;

    return iwl_phy_ctxt_apply(drv, ctxt,
        chains_static, chains_dynamic, FW_CTXT_ACTION_MODIFY, 0);
    
}

int
iwl_phy_ctxt_apply(IWLMvmDriver* drv,
    struct iwl_phy_ctx *ctxt,
    uint8_t chains_static, uint8_t chains_dynamic,
                   uint32_t action, uint32_t apply_time) {
    struct iwl_phy_context_cmd cmd;
    int ret;
    
    u16 len = sizeof(cmd) - iwl_mvm_chan_info_padding(drv);
    iwl_phy_ctxt_cmd_hdr(drv, ctxt, &cmd, action, apply_time);
    iwl_phy_ctxt_cmd_data(drv, &cmd, ctxt->channel,
        chains_static, chains_dynamic);
    
    ret = drv->sendCmdPdu(PHY_CONTEXT_CMD, 0, sizeof(cmd), &cmd);
    if(ret) {
        IWL_ERR(0, "Could not send phy context?\n");
    }
    
    return ret;
}

void
iwl_phy_ctxt_cmd_data(IWLMvmDriver* drv,
    struct iwl_phy_context_cmd *cmd, struct ieee80211_channel *chan,
                      uint8_t chains_static, uint8_t chains_dynamic) {
    struct ieee80211com *ic = &drv->m_pDevice->ie_ic;
    uint8_t active_cnt, idle_cnt;
    
    iwl_mvm_set_chan_info(drv, &cmd->ci, ieee80211_chan2ieee(ic, chan),
                                            IEEE80211_IS_CHAN_2GHZ(chan) ?
                                            PHY_BAND_24 : PHY_BAND_5,
                                            PHY_VHT_CHANNEL_MODE20,
                          PHY_VHT_CTRL_POS_1_BELOW);

    /* Set rx the chains */
    idle_cnt = chains_static;
    active_cnt = chains_dynamic;

    IWL_INFO(0,
        "%s: 2ghz=%d, channel=%d, chains static=0x%x, dynamic=0x%x, "
        "rx_ant=0x%x, tx_ant=0x%x\n",
        __func__,
        !! IEEE80211_IS_CHAN_2GHZ(chan),
        ieee80211_chan2ieee(ic, chan),
        chains_static,
        chains_dynamic,
        iwl_mvm_get_valid_rx_ant(drv->m_pDevice),
        iwl_mvm_get_valid_tx_ant(drv->m_pDevice));
    
    /* In scenarios where we only ever use a single-stream rates,
     * i.e. legacy 11b/g/a associations, single-stream APs or even
     * static SMPS, enable both chains to get diversity, improving
     * the case where we're far enough from the AP that attenuation
     * between the two antennas is sufficiently different to impact
     * performance.
     */
    if (active_cnt == 1 && (num_of_ant(iwl_mvm_get_valid_rx_ant(drv->m_pDevice)) != 1)) {
        idle_cnt = 2;
        active_cnt = 2;
    }

    iwl_phy_context_cmd_tail* tail = (iwl_phy_context_cmd_tail*)iwl_mvm_chan_info_cmd_tail(drv, &cmd->ci);
    tail->rxchain_info = htole32(iwl_mvm_get_valid_rx_ant(drv->m_pDevice) <<
                    PHY_RX_CHAIN_VALID_POS);
    tail->rxchain_info |= htole32(idle_cnt << PHY_RX_CHAIN_CNT_POS);
    tail->rxchain_info |= htole32(active_cnt <<
        PHY_RX_CHAIN_MIMO_CNT_POS);

    tail->txchain_info = htole32(iwl_mvm_get_valid_tx_ant(drv->m_pDevice));
}

void
iwl_phy_ctxt_cmd_hdr(IWLMvmDriver* drv, struct iwl_phy_ctx *ctxt,
                     struct iwl_phy_context_cmd *cmd, uint32_t action, uint32_t apply_time) {
    memset(cmd, 0, sizeof(iwl_phy_context_cmd));
    
    IWL_INFO(drv, "%s: id=%d, colour=%d, action=%d, apply_time=%d\n",
        __func__,
        ctxt->id,
        ctxt->color,
        action,
        apply_time);
    
    cmd->id_and_color = htole32(FW_CMD_ID_AND_COLOR(ctxt->id,
        ctxt->color));
    cmd->action = htole32(action);
    cmd->apply_time = htole32(apply_time);
}
