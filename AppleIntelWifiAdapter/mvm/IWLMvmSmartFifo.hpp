//
//  IWLMvmSmartFifo.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/19/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmSmartFifo_hpp
#define IWLMvmSmartFifo_hpp

#include "../fw/api/sf.h"
#include "../trans/IWLTransport.hpp"
#include "../mvm/IWLMvmDriver.hpp"

static const uint32_t
iwl_sf_full_timeout_def[SF_NUM_SCENARIO][SF_NUM_TIMEOUT_TYPES] = {
    {
        htole32(SF_SINGLE_UNICAST_AGING_TIMER_DEF),
        htole32(SF_SINGLE_UNICAST_IDLE_TIMER_DEF)
    },
    {
        htole32(SF_AGG_UNICAST_AGING_TIMER_DEF),
        htole32(SF_AGG_UNICAST_IDLE_TIMER_DEF)
    },
    {
        htole32(SF_MCAST_AGING_TIMER_DEF),
        htole32(SF_MCAST_IDLE_TIMER_DEF)
    },
    {
        htole32(SF_BA_AGING_TIMER_DEF),
        htole32(SF_BA_IDLE_TIMER_DEF)
    },
    {
        htole32(SF_TX_RE_AGING_TIMER_DEF),
        htole32(SF_TX_RE_IDLE_TIMER_DEF)
    },
};

static const uint32_t
iwl_sf_full_timeout[SF_NUM_SCENARIO][SF_NUM_TIMEOUT_TYPES] = {
    {
        htole32(SF_SINGLE_UNICAST_AGING_TIMER),
        htole32(SF_SINGLE_UNICAST_IDLE_TIMER)
    },
    {
        htole32(SF_AGG_UNICAST_AGING_TIMER),
        htole32(SF_AGG_UNICAST_IDLE_TIMER)
    },
    {
        htole32(SF_MCAST_AGING_TIMER),
        htole32(SF_MCAST_IDLE_TIMER)
    },
    {
        htole32(SF_BA_AGING_TIMER),
        htole32(SF_BA_IDLE_TIMER)
    },
    {
        htole32(SF_TX_RE_AGING_TIMER),
        htole32(SF_TX_RE_IDLE_TIMER)
    },
};


int iwl_sf_config(IWLMvmDriver* drv, int new_state);
void iwl_fill_sf_cmd(IWLMvmDriver* drv, iwl_sf_cfg_cmd *sf_cmd, ieee80211_node *ni);

#endif /* IWLMvmSmartFifo_hpp */
