//
//  IWLMvmSmartFifo.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/19/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmSmartFifo.hpp"

void iwl_fill_sf_cmd(IWLMvmDriver* dev, iwl_sf_cfg_cmd* cmd, ieee80211_node* ni) {
    int i, j, watermark;

    cmd->watermark[SF_LONG_DELAY_ON] = htole32(SF_W_MARK_SCAN);

    /*
     * If we are in association flow - check antenna configuration
     * capabilities of the AP station, and choose the watermark accordingly.
     */
    if (ni) {
        #ifndef IEEE80211_NO_HT
                if (ni->ni_flags & IEEE80211_NODE_HT) {
        #ifdef notyet
                    if (ni->ni_rxmcs[2] != 0)
                        watermark = IWM_SF_W_MARK_MIMO3;
                    else if (ni->ni_rxmcs[1] != 0)
                        watermark = IWM_SF_W_MARK_MIMO2;
                    else
        #endif
                        watermark = SF_W_MARK_SISO;
                } else
        #endif
                    watermark = SF_W_MARK_LEGACY;
        
    } else {
        watermark = SF_W_MARK_MIMO2;
    }
    
    cmd->watermark[SF_FULL_ON] = htole32(watermark);

    for (i = 0; i < SF_NUM_SCENARIO; i++) {
        for (j = 0; j < SF_NUM_TIMEOUT_TYPES; j++) {
            cmd->long_delay_timeouts[i][j] =
                    htole32(SF_LONG_DELAY_AGING_TIMER);
        }
    }

    if (ni) {
        memcpy(cmd->full_on_timeouts, iwl_sf_full_timeout,
               sizeof(iwl_sf_full_timeout));
    } else {
        memcpy(cmd->full_on_timeouts, iwl_sf_full_timeout_def,
               sizeof(iwl_sf_full_timeout_def));
    }
}

int iwl_sf_config(IWLMvmDriver* drv, int new_state) {
    IWLTransport* trans = drv->trans;
    ieee80211com* ic = &trans->m_pDevice->ie_ic;
    if(!ic)
        return -1;
    
    struct iwl_sf_cfg_cmd sf_cmd = {
        .state = htole32(new_state),
    };
    
    /*
    if(trans->m_pDevice->cfg->trans.device_family == IWL_DEVICE_FAMILY_8000)
        sf_cmd.state |= htole32(SF_CFG_DUMMY_NOTIF_OFF);
    */
    
    switch (new_state) {
    case SF_UNINIT:
    case SF_INIT_OFF:
        iwl_fill_sf_cmd(drv, &sf_cmd, NULL);
        break;
    case SF_FULL_ON:
        iwl_fill_sf_cmd(drv, &sf_cmd, ic->ic_bss);
        break;
    default:
        return EINVAL;
    }
    
    return drv->sendCmdPdu(REPLY_SF_CFG_CMD, CMD_ASYNC, sizeof(sf_cmd), &sf_cmd);
}
