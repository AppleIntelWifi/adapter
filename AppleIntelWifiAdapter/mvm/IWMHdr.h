//
//  IWMHdr.h
//  AppleIntelWifiAdapter
//
//  Created by qcwap on 2020/2/21.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWMHdr_h
#define IWMHdr_h

#include "../compat/openbsd/net80211/ieee80211_var.h"
#include "../compat/openbsd/net80211/ieee80211_amrr.h"
#include "../compat/openbsd/net80211/ieee80211_mira.h"
#include "../fw/api/phy-ctxt.h"

#define IWM_MIN_DBM    -100
#define IWM_MAX_DBM    -33    /* realistic guess */

#define IWM_STATION_ID 0
#define IWM_AUX_STA_ID 1


struct iwm_node {
    struct ieee80211_node in_ni;
    struct iwl_phy_ctx *in_phyctxt;

    uint16_t in_id;
    uint16_t in_color;

    struct ieee80211_amrr_node in_amn;
    struct ieee80211_mira_node in_mn;

    /* Set in 11n mode if we don't receive ACKs for OFDM frames. */
    int ht_force_cck;

};

#endif /* IWMHdr_h */
