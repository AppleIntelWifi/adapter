//
//  IWLMvmMac.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/20/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#include "IWLMvmMac.hpp"

#include "IWLApple80211.hpp"

int iwl_legacy_config_umac_scan(IWLMvmDriver* drv) {
  IWL_ERR(0, "legacy config not implemented");
  return -1;
}

int iwl_config_umac_scan(IWLMvmDriver* drv) {
  struct iwl_scan_config_v1* cfg;
  int nchan, err;
  size_t len;
  if (iwl_mvm_cdb_scan_api(drv->m_pDevice)) {
    if (iwl_mvm_is_cdb_supported(drv->m_pDevice)) {
      len = sizeof(iwl_scan_config_v2) +
            (drv->m_pDevice->fw.ucode_capa.n_scan_channels);
    } else {
      len = sizeof(iwl_scan_config_v1) +
            (drv->m_pDevice->fw.ucode_capa.n_scan_channels);
    }
  } else {
    len = sizeof(iwl_scan_config_v1) +
          (drv->m_pDevice->fw.ucode_capa.n_scan_channels);
  }

  cfg = reinterpret_cast<iwl_scan_config_v1*>(kzalloc(len));

  iwl_host_cmd hcmd = {.id = iwl_cmd_id(SCAN_CFG_CMD, IWL_ALWAYS_LONG_GROUP, 0),
                       .dataflags = {IWL_HCMD_DFL_NOCOPY},
                       .flags = 0};

  hcmd.data[0] = cfg;
  hcmd.len[0] = len;

  static const uint32_t rates =
      (SCAN_CONFIG_RATE_1M | SCAN_CONFIG_RATE_2M | SCAN_CONFIG_RATE_5M |
       SCAN_CONFIG_RATE_11M | SCAN_CONFIG_RATE_6M | SCAN_CONFIG_RATE_9M |
       SCAN_CONFIG_RATE_12M | SCAN_CONFIG_RATE_18M | SCAN_CONFIG_RATE_24M |
       SCAN_CONFIG_RATE_36M | SCAN_CONFIG_RATE_48M | SCAN_CONFIG_RATE_54M);

  cfg->flags = cpu_to_le32(
      SCAN_CONFIG_FLAG_ACTIVATE | SCAN_CONFIG_FLAG_ALLOW_CHUB_REQS |
      SCAN_CONFIG_FLAG_SET_TX_CHAINS | SCAN_CONFIG_FLAG_SET_RX_CHAINS |
      SCAN_CONFIG_FLAG_SET_AUX_STA_ID | SCAN_CONFIG_FLAG_SET_ALL_TIMES |
      SCAN_CONFIG_FLAG_SET_LEGACY_RATES | SCAN_CONFIG_FLAG_SET_MAC_ADDR |
      SCAN_CONFIG_FLAG_SET_CHANNEL_FLAGS |
      SCAN_CONFIG_N_CHANNELS(drv->m_pDevice->fw.ucode_capa.n_scan_channels) |
      SCAN_CONFIG_FLAG_CLEAR_FRAGMENTED);

  cfg->bcast_sta_id = IWM_AUX_STA_ID;
  cfg->tx_chains = cpu_to_le32(iwl_mvm_get_valid_tx_ant(drv->m_pDevice));
  cfg->rx_chains = cpu_to_le32(iwl_mvm_get_valid_rx_ant(drv->m_pDevice));
  cfg->legacy_rates = cpu_to_le32(rates | SCAN_CONFIG_SUPPORTED_RATE(rates));

  if (iwl_mvm_cdb_scan_api(drv->m_pDevice) &&
      iwl_mvm_is_cdb_supported(drv->m_pDevice)) {
    IWL_INFO(0, "cfg_v2 scan config being used\n");
    iwl_scan_config_v2* cfg_v2 = reinterpret_cast<iwl_scan_config_v2*>(cfg);
    cfg_v2->dwell.active = 10;
    cfg_v2->dwell.passive = 110;
    cfg_v2->dwell.fragmented = 44;
    cfg_v2->dwell.extended = 90;
    cfg_v2->out_of_channel_time[0] = cpu_to_le32(0);
    cfg_v2->out_of_channel_time[1] = cpu_to_le32(0);
    cfg_v2->suspend_time[0] = cpu_to_le32(0);
    cfg_v2->suspend_time[1] = cpu_to_le32(0);
    memcpy(&cfg_v2->mac_addr, drv->m_pDevice->ie_dev->getMAC(), ETH_ALEN);
    IWL_DEBUG(0, "%02x:%02x:%02x:%02x:%02x:%02x\n", cfg_v2->mac_addr[0],
              cfg_v2->mac_addr[1], cfg_v2->mac_addr[2], cfg_v2->mac_addr[3],
              cfg_v2->mac_addr[4], cfg_v2->mac_addr[5]);

    cfg_v2->channel_flags =
        IWL_CHANNEL_FLAG_EBS | IWL_CHANNEL_FLAG_ACCURATE_EBS |
        IWL_CHANNEL_FLAG_EBS_ADD | IWL_CHANNEL_FLAG_PRE_SCAN_PASSIVE2ACTIVE;

    struct apple80211_channel* c;

    nchan = 0;

    int num_channels = drv->m_pDevice->fw.ucode_capa.n_scan_channels;

    if (num_channels > IEEE80211_CHAN_MAX) {
      IWL_ERR(0, "ucode asked for %d channels\n", num_channels);
      IOFree(cfg, len);
      return -1;
    }

    apple80211_channel* channel_map = drv->m_pDevice->ie_dev->getChannelMap();
    if (channel_map == NULL) {
      IWL_ERR(0, "Channel map was null\n");
      return -1;
    }

    for (nchan = 0; nchan < num_channels; nchan++) {
      c = &channel_map[nchan];

      if (!c) continue;

      if (c->flags == 0) continue;

      IWL_DEBUG(0, "Adding channel %d to scan config\n", c->channel);
      cfg_v2->channel_array[nchan] = c->channel;
    }
  } else {
    cfg->dwell.active = 10;
    cfg->dwell.passive = 110;
    cfg->dwell.fragmented = 44;
    cfg->dwell.extended = 90;
    cfg->out_of_channel_time = cpu_to_le32(0);
    cfg->suspend_time = cpu_to_le32(0);

    memcpy(&cfg->mac_addr, drv->m_pDevice->ie_dev->getMAC(), ETH_ALEN);
    IWL_DEBUG(0, "%02x:%02x:%02x:%02x:%02x:%02x\n", cfg->mac_addr[0],
              cfg->mac_addr[1], cfg->mac_addr[2], cfg->mac_addr[3],
              cfg->mac_addr[4], cfg->mac_addr[5]);

    cfg->channel_flags = IWL_CHANNEL_FLAG_EBS | IWL_CHANNEL_FLAG_ACCURATE_EBS |
                         IWL_CHANNEL_FLAG_EBS_ADD |
                         IWL_CHANNEL_FLAG_PRE_SCAN_PASSIVE2ACTIVE;

    struct apple80211_channel* c;

    nchan = 0;

    int num_channels = drv->m_pDevice->fw.ucode_capa.n_scan_channels;

    if (num_channels > IEEE80211_CHAN_MAX) {
      IWL_ERR(0, "ucode asked for %d channels\n", num_channels);
      IOFree(cfg, len);
      return -1;
    }

    apple80211_channel* channel_map = drv->m_pDevice->ie_dev->getChannelMap();
    if (channel_map == NULL) {
      IWL_ERR(0, "Channel map was null\n");
      return -1;
    }

    for (nchan = 0; nchan < num_channels; nchan++) {
      c = &channel_map[nchan];

      if (!c) continue;

      if (c->flags == 0) continue;

      IWL_DEBUG(0, "Adding channel %d to scan config\n", c->channel);
      cfg->channel_array[nchan] = c->channel;
    }
  }

  err = drv->sendCmd(&hcmd);
  if (!err) IWL_DEBUG(0, "sent umac config successfully\n");

  IOFree(cfg, len);
  // drv->trans->freeResp(&hcmd);
  return err;
}

int iwl_fill_probe_req(IWLMvmDriver* drv, apple80211_scan_data* appleReq,
                       iwl_scan_probe_req_v1* preq) {
  bool ext_chan = fw_has_api(&drv->m_pDevice->fw.ucode_capa,
                             IWL_UCODE_TLV_API_SCAN_EXT_CHAN_VER);

  iwl_scan_probe_req* preq_v2;
  struct ieee80211_frame* wh;
  if (ext_chan) {
    preq_v2 = reinterpret_cast<iwl_scan_probe_req*>(preq);
    memset(preq_v2, 0, sizeof(*preq_v2));
    wh = reinterpret_cast<ieee80211_frame*>(preq_v2->buf);
  } else {
    memset(preq, 0, sizeof(*preq));
    wh = reinterpret_cast<ieee80211_frame*>(preq->buf);
  }

  ieee80211_rateset rs;
  size_t remain = sizeof(preq->buf);
  uint8_t *frm, *pos;

  if (WARN_ON(appleReq->ssid_len > sizeof(appleReq->ssid))) {
    IWL_ERR(0, "ssid_len longer then sizeof?\n");
    return -1;
  }

  if (remain < sizeof(*wh) + 2 + appleReq->ssid_len) {
    IWL_ERR(0, "no space for ssid\n");
    return ENOBUFS;
  }

  wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_MGT |
                IEEE80211_FC0_SUBTYPE_PROBE_REQ;
  wh->i_fc[1] = IEEE80211_FC1_DIR_NODS;

  memcpy(wh->i_addr1, etherbroadcastaddr, ETHER_ADDR_LEN);
  memcpy(wh->i_addr2, drv->m_pDevice->ie_dev->getMAC(), ETHER_ADDR_LEN);
  memcpy(wh->i_addr3, etherbroadcastaddr, ETHER_ADDR_LEN);
  // clang-format off
  *(uint16_t*)&wh->i_dur[0] = 0; /* filled by HW */  // NOLINT(readability/casting)
  *(uint16_t*)&wh->i_seq[0] = 0; /* filled by HW */     // NOLINT(readability/casting)
  frm = (uint8_t*)(wh + 1);  // NOLINT(readability/casting)
  // clang-format on
  frm = ieee80211_add_ssid(frm, appleReq->ssid, appleReq->ssid_len);

  /* Tell the firmware where the MAC header is. */
  preq->mac_header.offset = 0;
  preq->mac_header.len = htole16(frm - reinterpret_cast<uint8_t*>(wh));
  remain -= frm - reinterpret_cast<uint8_t*>(wh);

  /* 2Ghz IEs */
  memset(&rs, 0, sizeof(ieee80211_rateset));
  memcpy(&rs, &ieee80211_std_rateset_11g, sizeof(ieee80211_std_rateset_11g));
  // rs = &ieee80211_std_rateset_11g;
  if (rs.rs_nrates > IEEE80211_RATE_SIZE) {
    if (remain < 4 + rs.rs_nrates) {
      IWL_ERR(0, "no space for nrates\n");
      return ENOBUFS;
    }
  } else if (remain < 2 + rs.rs_nrates) {
    IWL_ERR(0, "no space for nrates (2)\n");
    return ENOBUFS;
  }

  preq->band_data[0].offset = htole16(frm - reinterpret_cast<uint8_t*>(wh));
  pos = frm;
  frm = ieee80211_add_rates(frm, &rs);
  if (rs.rs_nrates > IEEE80211_RATE_SIZE) frm = ieee80211_add_xrates(frm, &rs);
  preq->band_data[0].len = htole16(frm - pos);
  remain -= frm - pos;

  if (fw_has_capa(&drv->m_pDevice->fw.ucode_capa,
                  IWL_UCODE_TLV_CAPA_DS_PARAM_SET_IE_SUPPORT)) {
    if (remain < 3) {
      IWL_ERR(0, "no space for ie support\n");
      return ENOBUFS;
    }
    *frm++ = IEEE80211_ELEMID_DSPARMS;
    *frm++ = 1;
    *frm++ = 0;
    remain -= 3;
  }

  if (drv->m_pDevice->nvm_data->sku_cap_band_52ghz_enable) {
    memset(&rs, 0, sizeof(ieee80211_rateset));
    memcpy(&rs, &ieee80211_std_rateset_11a, sizeof(ieee80211_std_rateset_11a));

    if (rs.rs_nrates > IEEE80211_RATE_SIZE) {
      if (remain < 4 + rs.rs_nrates) {
        IWL_ERR(0, "no space for 5ghz nrates\n");
        return ENOBUFS;
      }
    } else if (remain < 2 + rs.rs_nrates) {
      IWL_ERR(0, "no space for 5ghz nrates 2\n");
      return ENOBUFS;
    }
    preq->band_data[1].offset = htole16(frm - reinterpret_cast<uint8_t*>(wh));
    pos = frm;
    frm = ieee80211_add_rates(frm, &rs);
    if (rs.rs_nrates > IEEE80211_RATE_SIZE)
      frm = ieee80211_add_xrates(frm, &rs);
    preq->band_data[1].len = htole16(frm - pos);
    remain -= frm - pos;
  }

  if (ext_chan) {
    preq_v2->common_data.offset = htole16(frm - reinterpret_cast<uint8_t*>(wh));
    pos = frm;

    preq_v2->common_data.len = htole16(frm - pos);
  } else {
    preq->common_data.offset = htole16(frm - reinterpret_cast<uint8_t*>(wh));
    pos = frm;

    preq->common_data.len = htole16(frm - pos);
  }

  return 0;
}
#define IWL_SCAN_CHANNEL_NSSIDS(x) (((1 << (x)) - 1) << 1)
#define IWL_SCAN_CHANNEL_TYPE_ACTIVE (1 << 0)
#define IWL_SCAN_CHANNEL_UMAC_NSSIDS(x) ((1 << (x)) - 1)

int iwl_umac_scan_fill_channels(IWLMvmDriver* drv,
                                apple80211_scan_data* appleReq,
                                iwl_scan_channel_cfg_umac* chan, int n_ssids) {
#ifndef notyet
  int num_channels = appleReq->num_channels;
#else
  int num_channels = drv->m_pDevice->n_chans;
#endif

  bool scan_all = false;

  if (num_channels == 0) {
    // they probably want us to scan every channel
    num_channels = drv->m_pDevice->ie_dev->getChannelMapSize();
    scan_all = true;
  }

  struct apple80211_channel* c;
  uint8_t nchan;

  apple80211_channel* channel_map = NULL;
  if (!scan_all) {
    channel_map = drv->m_pDevice->ie_dev->getScanChannelMap();
  } else {
    channel_map = drv->m_pDevice->ie_dev->getChannelMap();
  }

  if (channel_map == NULL) {
    IWL_ERR(0, "Channel map was null\n");
    return -1;
  }

  for (nchan = 0; nchan < num_channels; nchan++) {
    c = &channel_map[nchan];

    if (!c) continue;

    if (c->channel == 0)  // channel should never be 0
      continue;

    IWL_DEBUG(0, "adding chan %d to scan\n", c->channel);
    chan->v1.channel_num = htole16(c->channel);

#ifndef notyet
    if (fw_has_api(&drv->m_pDevice->fw.ucode_capa,
                   IWL_UCODE_TLV_API_SCAN_EXT_CHAN_VER)) {
#else
    if (0) {
#endif
      if (c->flags & APPLE80211_C_FLAG_5GHZ) chan->v2.band = 1;

      chan->v2.iter_count = 1;
      chan->v2.iter_interval = htole16(0);
    } else {
      chan->v1.iter_count = 1;
      chan->v1.iter_interval = htole16(0);
    }
    chan->flags |= htole32(IWL_SCAN_CHANNEL_UMAC_NSSIDS(n_ssids));

    chan++;
  }

  return nchan;
}

bool iwl_scan_use_ebs(IWLMvmDriver* drv) {
  const struct iwl_ucode_capabilities* capa = &drv->m_pDevice->fw.ucode_capa;
  return ((capa->flags & IWL_UCODE_TLV_FLAGS_EBS_SUPPORT) &&
          drv->m_pDevice->last_ebs_successful);
}

int iwl_umac_scan(IWLMvmDriver* drv, apple80211_scan_data* appleReq) {
  // clang-format off
  iwl_host_cmd hcmd = {
      .id = iwl_cmd_id(SCAN_REQ_UMAC, IWL_ALWAYS_LONG_GROUP, 0),
      .len = {
              0,
          },
      .data = {
              NULL,
          },
      .flags = CMD_ASYNC,
      .dataflags = {
              IWL_HCMD_DFL_DUP,
          },
  };
  // clang-format on

  bool adaptive_dwell = fw_has_api(&drv->m_pDevice->fw.ucode_capa,
                                   IWL_UCODE_TLV_API_ADAPTIVE_DWELL);
  bool ext_chan;

  // Patch out ext_chan for 8xxx devices..
  // Without this patch, we would not be able to scan (for some god awful
  // reason)
  if (drv->m_pDevice->cfg->trans.device_family == IWL_DEVICE_FAMILY_8000)
    ext_chan = false;
  else
    ext_chan = fw_has_api(&drv->m_pDevice->fw.ucode_capa,
                          IWL_UCODE_TLV_API_SCAN_EXT_CHAN_VER);

    // bool adaptive_dwell = false;

#ifdef notyet
  int num_channels = appleReq->num_channels;
#else
  int num_channels = drv->m_pDevice->fw.ucode_capa.n_scan_channels;
#endif

  IWL_INFO(0, "requested scan for %d channels\n", num_channels);
  iwl_scan_req_umac* req;
  iwl_scan_req_umac_tail_v1* tail;
  iwl_scan_req_umac_tail_v2* tail_v2;
  size_t req_len;
  int err;

  if (!fw_has_capa(&drv->m_pDevice->fw.ucode_capa,
                   IWL_UCODE_TLV_CAPA_UMAC_SCAN))
    IWL_ERR(0, "firmware does not support umac\n");
  else
    IWL_INFO(0, "firmware supports umac :)\n");

  if (!adaptive_dwell) {
    IWL_INFO(0, "no adaptive dwell\n");
    req_len = IWL_SCAN_REQ_UMAC_SIZE_V1 +
              (sizeof(iwl_scan_channel_cfg_umac) * num_channels) +
              sizeof(iwl_scan_req_umac_tail_v1);
  } else {
    IWL_INFO(0, "adaptive dwell\n");
    if (ext_chan) {
      req_len = IWL_SCAN_REQ_UMAC_SIZE_V7 +
                (sizeof(iwl_scan_channel_cfg_umac) * num_channels) +
                sizeof(iwl_scan_req_umac_tail_v2);
    } else {
      req_len = IWL_SCAN_REQ_UMAC_SIZE_V7 +
                (sizeof(iwl_scan_channel_cfg_umac) * num_channels) +
                sizeof(iwl_scan_req_umac_tail_v1);
    }
  }

  if (req_len > MAX_CMD_PAYLOAD_SIZE) {
    IWL_ERR(
        0,
        "request length is longer then payload size? (wanted: %lu, max: %lu)\n",
        req_len, MAX_CMD_PAYLOAD_SIZE);
    return ENOMEM;
  }

  req = reinterpret_cast<iwl_scan_req_umac*>(kzalloc(req_len));

  hcmd.len[0] = req_len;
  hcmd.data[0] = req;

  req->general_flags = cpu_to_le16(IWL_UMAC_SCAN_GEN_FLAGS_PASS_ALL);
  req->ooc_priority = cpu_to_le32(IWL_SCAN_PRIORITY_HIGH);

  if (!adaptive_dwell) {
    IWL_INFO(0, "no adaptive dwell 2\n");
    req->general_flags |= cpu_to_le32(IWL_UMAC_SCAN_GEN_FLAGS_EXTENDED_DWELL);
  } else {
    req->general_flags |=
        cpu_to_le32(IWL_UMAC_SCAN_GEN_FLAGS_PROB_REQ_HIGH_TX_RATE);
    req->general_flags |= cpu_to_le32(IWL_UMAC_SCAN_GEN_FLAGS_ADAPTIVE_DWELL);
  }

  int channel_flags;
  channel_flags = 0;

  if (iwl_scan_use_ebs(drv)) {
    channel_flags = IWL_SCAN_CHANNEL_FLAG_EBS |
                    IWL_SCAN_CHANNEL_FLAG_EBS_ACCURATE |
                    IWL_SCAN_CHANNEL_FLAG_CACHE_ADD;

    if (fw_has_api(&drv->m_pDevice->fw.ucode_capa, IWL_UCODE_TLV_API_FRAG_EBS))
      channel_flags |= IWL_SCAN_CHANNEL_FLAG_EBS_FRAG;
  }

  req->scan_start_mac_id = 4;

  if (adaptive_dwell) {
    IWL_INFO(0, "adaptive dwell targeted\n");
    req->v7.active_dwell = 10;
    req->v7.passive_dwell = 110;
    req->v7.fragmented_dwell = 44;
    req->v7.adwell_default_n_aps_social =
        10;                            // IWL_SCAN_ADWELL_DEFAULT_N_APS_SOCIAL
    req->v7.adwell_default_n_aps = 2;  // IWL_SCAN_ADWELL_DEFAULT_LB_N_APS
    req->v7.adwell_max_budget = htole16(300);
    req->v7.scan_priority = htole32(IWL_SCAN_PRIORITY_HIGH);

    if (fw_has_api(&drv->m_pDevice->fw.ucode_capa,
                   IWL_UCODE_TLV_API_ADWELL_HB_DEF_N_AP) &&
        drv->m_pDevice->cfg->trans.device_family != IWL_DEVICE_FAMILY_8000) {
      req->v9.adwell_default_hb_n_aps = 8;  // IWL_SCAN_ADWELL_DEFAULT_HB_N_APS
    }

    if (fw_has_capa(&drv->m_pDevice->fw.ucode_capa,
                    IWL_UCODE_TLV_CAPA_CDB_SUPPORT)) {
      req->v7.max_out_time[0] = htole32(120);
      req->v7.suspend_time[0] = htole32(120);
    }

    if (!fw_has_api(&drv->m_pDevice->fw.ucode_capa,
                    IWL_UCODE_TLV_API_ADAPTIVE_DWELL_V2) ||
        drv->m_pDevice->cfg->trans.device_family == IWL_DEVICE_FAMILY_8000) {
      // 8xxx series devices cannot handle adaptive dwell v2 for some reason.
      // However, 9xxx series devices can handle it just fine ???

      req->v7.fragmented_dwell = 44;  // IWL_SCAN_DWELL_FRAGMENTED
      req->v7.active_dwell = 10;
      req->v7.passive_dwell = 110;
      req->v7.channel.flags = channel_flags;
      req->v7.channel.count = iwl_umac_scan_fill_channels(
          drv, appleReq, (struct iwl_scan_channel_cfg_umac*)req->v7.data,
          appleReq->ssid_len != 0);

    } else {
      req->v8.active_dwell[0] = 10;
      req->v8.passive_dwell[0] = 110;
      req->v8.num_of_fragments[0] = 3;
      req->v8.channel.flags = channel_flags;
      req->v8.channel.count = iwl_umac_scan_fill_channels(
          drv, appleReq, (struct iwl_scan_channel_cfg_umac*)req->v8.data,
          appleReq->ssid_len != 0);

      req->v8.general_flags2 = IWL_UMAC_SCAN_GEN_FLAGS2_ALLOW_CHNL_REORDER;

      if (fw_has_capa(&drv->m_pDevice->fw.ucode_capa,
                      IWL_UCODE_TLV_CAPA_CDB_SUPPORT)) {
        req->v8.active_dwell[1] = 10;
        req->v8.passive_dwell[1] = 110;
      }

      IWL_INFO(0, "adaptive v2\n");
    }

    // clang-format off
    if (ext_chan)
      tail_v2 =
          (iwl_scan_req_umac_tail_v2*)((char*)&req->v7.data +  // NOLINT(readability/casting)
                                       sizeof(iwl_scan_channel_cfg_umac) *
                                           num_channels);
    else
      tail =
          (iwl_scan_req_umac_tail_v1*)((char*)&req->v7.data +  // NOLINT(readability/casting)
                                       sizeof(iwl_scan_channel_cfg_umac) *
                                           num_channels);
    // clang-format on
  } else {
    IWL_INFO(0, "no adaptive dwell\n");
    req->v1.active_dwell = 10;
    req->v1.passive_dwell = 110;
    req->v1.fragmented_dwell = 44;
    req->v1.extended_dwell = 90;
    req->v1.scan_priority = cpu_to_le32(IWL_SCAN_PRIORITY_HIGH);
    req->v1.channel.flags = channel_flags;
    req->v1.channel.count = iwl_umac_scan_fill_channels(
        drv, appleReq, (struct iwl_scan_channel_cfg_umac*)&req->v1.data,
        appleReq->ssid_len != 0);
    // req->v1.max_out_time = htole32(120);
    // req->v1.suspend_time = htole32(120);
    // clang-format off
    tail =
        (iwl_scan_req_umac_tail_v1*)((char*)&req->v1.data +  // NOLINT(readability/casting)
                                     sizeof(iwl_scan_channel_cfg_umac) *
                                         num_channels);
    // clang-format on
  }

  if (appleReq->ssid_len != 0) {
    IWL_INFO(0, "Directed scan towards: %s\n", appleReq->ssid);
    if (ext_chan) {
      tail_v2->direct_scan[0].id = IEEE80211_ELEMID_SSID;
      tail_v2->direct_scan[0].len = appleReq->ssid_len;
      memcpy(tail_v2->direct_scan[0].ssid, appleReq->ssid, appleReq->ssid_len);
    } else {
      tail->direct_scan[0].id = IEEE80211_ELEMID_SSID;
      tail->direct_scan[0].len = appleReq->ssid_len;
      memcpy(tail->direct_scan[0].ssid, appleReq->ssid, appleReq->ssid_len);
    }
    req->general_flags |= cpu_to_le16(IWL_UMAC_SCAN_GEN_FLAGS_PRE_CONNECT);
  } else {
    IWL_INFO(0, "General scan\n");
    req->general_flags |= cpu_to_le16(IWL_UMAC_SCAN_GEN_FLAGS_PASSIVE);
  }

  if (fw_has_capa(&drv->m_pDevice->fw.ucode_capa,
                  IWL_UCODE_TLV_CAPA_DS_PARAM_SET_IE_SUPPORT))
    req->general_flags |= cpu_to_le16(IWL_UMAC_SCAN_GEN_FLAGS_RRM_ENABLED);

  IWL_INFO(0, "Filling probe request\n");
  if (ext_chan)
    err = iwl_fill_probe_req(
        drv, appleReq,
        reinterpret_cast<iwl_scan_probe_req_v1*>(&tail_v2->preq));
  else
    err = iwl_fill_probe_req(drv, appleReq, &tail->preq);

  if (err) {
    IWL_ERR(0, "filling probe req failed\n");
    IOFree(req, req_len);
    return err;
  }

  if (ext_chan) {
    tail_v2->schedule[0].interval = 0;
    tail_v2->schedule[0].iter_count = 1;
  } else {
    tail->schedule[0].interval = 0;
    tail->schedule[0].iter_count = 1;
  }

  err = drv->sendCmd(&hcmd);
  IOFree(req, req_len);

  return err;
}

static uint16_t iwl_scan_rx_chain(IWLMvmDriver* drv) {
  uint16_t rx_chain;
  uint8_t rx_ant;

  rx_ant = iwl_mvm_get_valid_rx_ant(drv->m_pDevice);
  rx_chain = rx_ant << PHY_RX_CHAIN_VALID_POS;
  rx_chain |= rx_ant << PHY_RX_CHAIN_FORCE_MIMO_SEL_POS;
  rx_chain |= rx_ant << PHY_RX_CHAIN_FORCE_SEL_POS;
  rx_chain |= 0x1 << PHY_RX_CHAIN_DRIVER_FORCE_POS;
  return htole16(rx_chain);
}

#define RATE_MCS_ANT_NUM 3
static uint32_t iwl_scan_rate_n_flags(IWLMvmDriver* drv, int flags,
                                      int no_cck) {
  uint32_t tx_ant;
  int i, ind;

  for (i = 0, ind = drv->m_pDevice->last_scan_ant; i < RATE_MCS_ANT_NUM; i++) {
    ind = (ind + 1) % RATE_MCS_ANT_NUM;
    if (iwl_mvm_get_valid_tx_ant(drv->m_pDevice) & (1 << ind)) {
      drv->m_pDevice->last_scan_ant = ind;
      break;
    }
  }
  tx_ant = (1 << drv->m_pDevice->last_scan_ant) << RATE_MCS_ANT_POS;

  if ((flags & IEEE80211_CHAN_2GHZ) && !no_cck)
    return htole32(IWL_RATE_1M_PLCP | RATE_MCS_CCK_MSK | tx_ant);
  else
    return htole32(IWL_RATE_6M_PLCP | tx_ant);
}

uint8_t iwl_lmac_scan_fill_channels(IWLMvmDriver* drv,
                                    apple80211_scan_data* appleReq,
                                    struct iwl_scan_channel_cfg_lmac* chan,
                                    int n_ssids) {
  struct apple80211_channel* c;
  uint8_t nchan;

  for (nchan = 0; nchan < appleReq->num_channels; nchan++) {
    c = &appleReq->channels[nchan];

    chan->channel_num = htole16(c->channel);
    chan->iter_count = htole16(1);
    chan->iter_interval = htole32(0);
    chan->flags = htole32(IWL_UNIFIED_SCAN_CHANNEL_PARTIAL);
    chan->flags |= htole32(IWL_SCAN_CHANNEL_NSSIDS(n_ssids));

    if ((c->flags & APPLE80211_C_FLAG_ACTIVE) && n_ssids != 0)
      chan->flags |= htole32(IWL_SCAN_CHANNEL_TYPE_ACTIVE);

    chan++;
    nchan++;
  }

  return nchan;
}

int iwl_lmac_scan(IWLMvmDriver* drv, apple80211_scan_data* appleReq) {
  // clang-format off
  iwl_host_cmd hcmd = {
      .id = iwl_cmd_id(SCAN_OFFLOAD_REQUEST_CMD, IWL_ALWAYS_LONG_GROUP, 0),
      .len = {
              0,
          },
      .data = {
              NULL,
          },
      .flags = 0,
      .dataflags = {
              IWL_HCMD_DFL_NOCOPY,
          },
  };
  // clang-format on
  iwl_scan_req_lmac* req;
  size_t len;

  int err;
  len = sizeof(iwl_scan_req_lmac) +
        (sizeof(iwl_scan_channel_cfg_lmac) * appleReq->num_channels) +
        sizeof(iwl_scan_probe_req_v1);

  if (len > MAX_CMD_PAYLOAD_SIZE) return ENOMEM;

  req = reinterpret_cast<iwl_scan_req_lmac*>(kzalloc(len));
  hcmd.len[0] = len;
  hcmd.data[0] = req;

  req->active_dwell = 10;
  req->passive_dwell = 110;
  req->fragmented_dwell = 44;
  req->extended_dwell = 90;
  req->max_out_time = 0;
  req->suspend_time = 0;

  req->scan_prio = htole32(IWL_SCAN_PRIORITY_HIGH);
  IWL_INFO(0, "valid rx ant: %d\n", drv->m_pDevice->fw.valid_rx_ant);
  req->rx_chain_select = iwl_scan_rx_chain(drv);
  req->iter_num = htole32(1);
  req->delay = 0;

  req->scan_flags = htole32(IWL_MVM_LMAC_SCAN_FLAG_PASS_ALL |
                            IWL_MVM_LMAC_SCAN_FLAG_ITER_COMPLETE |
                            IWL_MVM_LMAC_SCAN_FLAG_EXTENDED_DWELL);
  if (appleReq->ssid_len == 0)
    req->scan_flags |= htole32(IWL_MVM_LMAC_SCAN_FLAG_PASSIVE);
  else
    req->scan_flags |= htole32(IWL_MVM_LMAC_SCAN_FLAG_PRE_CONNECTION);

  if (fw_has_capa(&drv->m_pDevice->fw.ucode_capa,
                  IWL_UCODE_TLV_CAPA_DS_PARAM_SET_IE_SUPPORT))
    req->scan_flags |= htole32(IWL_MVM_LMAC_SCAN_FLAGS_RRM_ENABLED);

  req->flags = htole32(PHY_BAND_24);
  if (drv->m_pDevice->nvm_data->sku_cap_band_52ghz_enable)
    req->flags |= htole32(PHY_BAND_5);
  req->filter_flags = htole32(MAC_FILTER_ACCEPT_GRP | MAC_FILTER_IN_BEACON);
  /* Tx flags 2 GHz. */

  req->tx_cmd[0].tx_flags = htole32(TX_CMD_FLG_SEQ_CTL | TX_CMD_FLG_BT_DIS);
  req->tx_cmd[0].rate_n_flags =
      iwl_scan_rate_n_flags(drv, IEEE80211_CHAN_2GHZ, 1 /*XXX*/);
  req->tx_cmd[0].sta_id = IWM_AUX_STA_ID;

  /* Tx flags 5 GHz. */
  req->tx_cmd[1].tx_flags = htole32(TX_CMD_FLG_SEQ_CTL | TX_CMD_FLG_BT_DIS);
  req->tx_cmd[1].rate_n_flags =
      iwl_scan_rate_n_flags(drv, IEEE80211_CHAN_5GHZ, 1 /*XXX*/);
  req->tx_cmd[1].sta_id = IWM_AUX_STA_ID;

  if (appleReq->ssid_len != 0) {
    req->direct_scan[0].id = IEEE80211_ELEMID_SSID;
    req->direct_scan[0].len = appleReq->ssid_len;
    memcpy(req->direct_scan[0].ssid, appleReq->ssid, appleReq->ssid_len);
  }

  req->n_channels = iwl_lmac_scan_fill_channels(
      drv, appleReq, reinterpret_cast<iwl_scan_channel_cfg_lmac*>(req->data),
      appleReq->ssid_len != 0);

  err = iwl_fill_probe_req(
      drv, appleReq,
      reinterpret_cast<iwl_scan_probe_req_v1*>(
          req->data +
          sizeof(struct iwl_scan_channel_cfg_lmac) * appleReq->num_channels));

  if (err) {
    IWL_ERR(0, "filling probe req failed\n");
    IOFree(req, len);
    return err;
  }

  req->schedule[0].iterations = 1;
  req->schedule[0].full_scan_mul = 1;

  /* Disable EBS. */
  req->channel_opt[0].non_ebs_ratio = 1;
  req->channel_opt[1].non_ebs_ratio = 1;

  err = drv->sendCmd(&hcmd);
  IOFree(req, len);

  return err;
}

int iwl_disable_beacon_filter(IWLMvmDriver* drv) {
  struct iwl_beacon_filter_cmd cmd;
  int err;

  memset(&cmd, 0, sizeof(cmd));

  return drv->sendCmdPdu(REPLY_BEACON_FILTERING_CMD, 0, sizeof(cmd), &cmd);
}
