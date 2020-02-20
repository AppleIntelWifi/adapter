//
//  IWLMvmPhy.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/19/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmPhy_hpp
#define IWLMvmPhy_hpp

#include "IWLMvmDriver.hpp"

int
iwl_phy_ctxt_add(IWLMvmDriver* drv, struct iwl_phy_ctx *ctxt,
    struct ieee80211_channel *chan,
                 uint8_t chains_static, uint8_t chains_dynamic);

int
iwl_phy_ctxt_changed(IWLMvmDriver* drv,
    struct iwl_phy_ctx *ctxt, struct ieee80211_channel *chan,
                     uint8_t chains_static, uint8_t chains_dynamic);

int
iwl_phy_ctxt_apply(IWLMvmDriver* drv,
    struct iwl_phy_ctx *ctxt,
    uint8_t chains_static, uint8_t chains_dynamic,
                   uint32_t action, uint32_t apply_time);

void
iwl_phy_ctxt_cmd_data(IWLMvmDriver* drv,
    struct iwl_phy_context_cmd *cmd, struct ieee80211_channel *chan,
                      uint8_t chains_static, uint8_t chains_dynamic);

void
iwl_phy_ctxt_cmd_hdr(IWLMvmDriver* drv, struct iwl_phy_ctx *ctxt,
                     struct iwl_phy_context_cmd *cmd, uint32_t action, uint32_t apply_time);


#endif /* IWLMvmPhy_hpp */
