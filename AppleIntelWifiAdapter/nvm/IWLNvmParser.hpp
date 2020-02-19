//
//  IWLNvmParser.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/2.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLNvmParser_hpp
#define IWLNvmParser_hpp

#include "IWLeeprom.h"
#include "IWLTransport.hpp"
#include "../fw/api/nvm-reg.h"
#include "../fw/api/commands.h"
#include "IWLTransOps.h"

#include <net80211/ieee80211_var.h>
#include <linux/types.h>

/* radio config bits (actual values from NVM definition) */
#define NVM_RF_CFG_DASH_MSK(x)   (x & 0x3)         /* bits 0-1   */
#define NVM_RF_CFG_STEP_MSK(x)   ((x >> 2)  & 0x3) /* bits 2-3   */
#define NVM_RF_CFG_TYPE_MSK(x)   ((x >> 4)  & 0x3) /* bits 4-5   */
#define NVM_RF_CFG_PNUM_MSK(x)   ((x >> 6)  & 0x3) /* bits 6-7   */
#define NVM_RF_CFG_TX_ANT_MSK(x) ((x >> 8)  & 0xF) /* bits 8-11  */
#define NVM_RF_CFG_RX_ANT_MSK(x) ((x >> 12) & 0xF) /* bits 12-15 */

#define EXT_NVM_RF_CFG_FLAVOR_MSK(x)   ((x) & 0xF)
#define EXT_NVM_RF_CFG_DASH_MSK(x)   (((x) >> 4) & 0xF)
#define EXT_NVM_RF_CFG_STEP_MSK(x)   (((x) >> 8) & 0xF)
#define EXT_NVM_RF_CFG_TYPE_MSK(x)   (((x) >> 12) & 0xFFF)
#define EXT_NVM_RF_CFG_TX_ANT_MSK(x) (((x) >> 24) & 0xF)
#define EXT_NVM_RF_CFG_RX_ANT_MSK(x) (((x) >> 28) & 0xF)

/**
 * enum iwl_nvm_sbands_flags - modification flags for the channel profiles
 *
 * @IWL_NVM_SBANDS_FLAGS_LAR: LAR is enabled
 * @IWL_NVM_SBANDS_FLAGS_NO_WIDE_IN_5GHZ: disallow 40, 80 and 160MHz on 5GHz
 */
enum iwl_nvm_sbands_flags {
    IWL_NVM_SBANDS_FLAGS_LAR        = BIT(0),
    IWL_NVM_SBANDS_FLAGS_NO_WIDE_IN_5GHZ    = BIT(1),
};

void iwl_set_hw_address_from_csr(IWLTransport *trans,
                                 struct iwl_nvm_data *data);

/**
 * iwl_parse_nvm_data - parse NVM data and return values
 *
 * This function parses all NVM values we need and then
 * returns a (newly allocated) struct containing all the
 * relevant values for driver use. The struct must be freed
 * later with iwl_free_nvm_data().
 */
struct iwl_nvm_data *
iwl_parse_nvm_data(IWLTransport *trans, const struct iwl_cfg *cfg,
                   const struct iwl_fw *fw,
                   const __be16 *nvm_hw, const __le16 *nvm_sw,
                   const __le16 *nvm_calib, const __le16 *regulatory,
                   const __le16 *mac_override, const __le16 *phy_sku,
                   u8 tx_chains, u8 rx_chains);

/**
 * iwl_parse_mcc_info - parse MCC (mobile country code) info coming from FW
 *
 * This function parses the regulatory channel data received as a
 * MCC_UPDATE_CMD command. It returns a newly allocation regulatory domain,
 * to be fed into the regulatory core. In case the geo_info is set handle
 * accordingly. An ERR_PTR is returned on error.
 * If not given to the regulatory core, the user is responsible for freeing
 * the regdomain returned here with kfree.
 */
struct ieee80211_regdomain *
iwl_parse_nvm_mcc_info(IWLDevice *dev, const struct iwl_cfg *cfg,
                       int num_of_ch, __le32 *channels, u16 fw_mcc,
                       u16 geo_info, u16 cap);

/**
 * iwl_read_external_nvm - Reads external NVM from a file into nvm_sections
 */
int iwl_read_external_nvm(IWLTransport *trans,
                          const char *nvm_file_name,
                          struct iwl_nvm_section *nvm_sections);

void iwl_nvm_fixups(u32 hw_id, unsigned int section, u8 *data,
                    unsigned int len);

void iwl_init_sbands(IWLTransport *trans,
                     struct iwl_nvm_data *data,
                     const void *nvm_ch_flags, u8 tx_chains,
                     u8 rx_chains, u32 sbands_flags, bool v4);

#endif /* IWLNvmParser_hpp */
