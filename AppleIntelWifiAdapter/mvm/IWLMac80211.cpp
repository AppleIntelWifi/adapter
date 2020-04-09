//
//  IWLMac80211.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/21.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#include "IWLMvmDriver.hpp"

bool IWLMvmDriver::ieee80211Init() { return true; }

bool IWLMvmDriver::ieee80211Run() { return true; }

void IWLMvmDriver::ieee80211Release() { return; }

int IWLMvmDriver::iwm_bgscan(struct ieee80211com *ic) {
  IWL_INFO(0, "bg scan\n");
  return 0;
}

struct ieee80211_node *IWLMvmDriver::iwm_node_alloc(struct ieee80211com *ic) {
  IWL_INFO(0, "iwm_node_alloc\n");
  void *buf = IOMalloc(sizeof(struct iwm_node));
  if (buf) {
    bzero(buf, sizeof(struct iwm_node));
  }
  return (struct ieee80211_node *)buf;
}

int IWLMvmDriver::iwm_newstate(struct ieee80211com *ic,
                               enum ieee80211_state nstate, int arg) {
  struct ifnet *ifp = &ic->ic_if;

  if (!ifp) {
    IWL_ERR(0, "got null ifp\n");
    return -1;
  }
  IWLMvmDriver *sc = reinterpret_cast<IWLMvmDriver *>(ifp->if_softc);
  struct iwm_node *in = (struct iwm_node *)ic->ic_bss;

  if (!in) {
    IWL_ERR(0, "could not get bss\n");
    return -1;
  }

  if (ic->ic_state == IEEE80211_S_RUN) {
    //        timeout_del(&sc->sc_calib_to);
    ieee80211_mira_cancel_timeouts(&in->in_mn);
    //        iwm_del_task(sc, systq, &sc->ba_task);
    //        iwm_del_task(sc, systq, &sc->htprot_task);
  }

  //    sc->ns_nstate = nstate;
  //    sc->ns_arg = arg;

  //    iwm_add_task(sc, sc->sc_nswq, &sc->newstate_task);

  return 0;
}

void IWLMvmDriver::iwm_setup_ht_rates() {
  struct ieee80211com *ic = &m_pDevice->ie_ic;
  uint8_t rx_ant;

  /* TX is supported with the same MCS as RX. */
  ic->ic_tx_mcs_set = IEEE80211_TX_MCS_SET_DEFINED;

  ic->ic_sup_mcs[0] = 0xff; /* MCS 0-7 */

  if (m_pDevice->nvm_data->sku_cap_mimo_disabled) return;

  rx_ant = iwl_mvm_get_valid_rx_ant(m_pDevice);
  if ((rx_ant & ANT_AB) == ANT_AB || (rx_ant & ANT_BC) == ANT_BC)
    ic->ic_sup_mcs[1] = 0xff; /* MCS 8-15 */
}

int IWLMvmDriver::iwm_binding_cmd(struct iwm_node *in, uint32_t action) {
  struct iwl_binding_cmd cmd;
  struct iwl_phy_ctx *phyctxt = in->in_phyctxt;
  uint32_t mac_id = FW_CMD_ID_AND_COLOR(in->in_id, in->in_color);
  int i, err;
  uint32_t status;
  int size;

  memset(&cmd, 0, sizeof(cmd));

  if (fw_has_capa(&m_pDevice->fw.ucode_capa,
                  IWL_UCODE_TLV_CAPA_BINDING_CDB_SUPPORT)) {
    size = sizeof(cmd);
    if (phyctxt->channel->flags & APPLE80211_C_FLAG_2GHZ ||
        !iwl_mvm_is_cdb_supported(m_pDevice))
      cmd.lmac_id = cpu_to_le32(IWL_LMAC_24G_INDEX);
    else
      cmd.lmac_id = cpu_to_le32(IWL_LMAC_5G_INDEX);
  } else {
    size = IWL_BINDING_CMD_SIZE_V1;
  }

  cmd.id_and_color =
      cpu_to_le32(FW_CMD_ID_AND_COLOR(phyctxt->id, phyctxt->color));
  cmd.action = cpu_to_le32(action);
  cmd.phy = cpu_to_le32(FW_CMD_ID_AND_COLOR(phyctxt->id, phyctxt->color));

  cmd.macs[0] = cpu_to_le32(mac_id);
  for (i = 1; i < MAX_MACS_IN_BINDING; i++)
    cmd.macs[i] = cpu_to_le32(FW_CTXT_INVALID);

  status = 0;
  err = sendCmdPduStatus(BINDING_CONTEXT_CMD, size, &cmd, &status);
  if (err) {
    IWL_ERR(mvm, "Failed to send binding (action:%d): %d\n", action, err);
    return err;
  }
  if (status) {
    IWL_ERR(mvm, "Binding command failed: %u\n", status);
    err = -EIO;
  }

  return err;
}

/* This file is a disaster */
