//
//  IWLEepromParser.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/2.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLEepromParser_hpp
#define IWLEepromParser_hpp

#include "IWLeeprom.h"
#include "IWLTransport.hpp"

/**
 * iwl_parse_eeprom_data - parse EEPROM data and return values
 *
 * @dev: device pointer we're parsing for, for debug only
 * @cfg: device configuration for parsing and overrides
 * @eeprom: the EEPROM data
 * @eeprom_size: length of the EEPROM data
 *
 * This function parses all EEPROM values we need and then
 * returns a (newly allocated) struct containing all the
 * relevant values for driver use. The struct must be freed
 * later with iwl_free_nvm_data().
 */
struct iwl_nvm_data *
iwl_parse_eeprom_data(IWLTransport *trans, const struct iwl_cfg *cfg,
              const u8 *eeprom, size_t eeprom_size);

int iwl_init_sband_channels(struct iwl_nvm_data *data,
                struct ieee80211_supported_band *sband,
                int n_channels, enum nl80211_band band);

void iwl_init_ht_hw_capab(IWLTransport *io,
              struct iwl_nvm_data *data,
              struct ieee80211_sta_ht_cap *ht_info,
              enum nl80211_band band,
              u8 tx_chains, u8 rx_chains);

int iwl_read_eeprom(IWLTransport *trans, u8 **eeprom, size_t *eeprom_size);

#endif /* IWLEepromParser_hpp */
