//
//  IWLPhyDb.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/8.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLPhyDb_hpp
#define IWLPhyDb_hpp

#include <linux/types.h>
#include <linux/kernel.h>
#include "IWLTransport.hpp"

#define CHANNEL_NUM_SIZE    4    /* num of channels in calib_ch size */

struct iwl_phy_db_entry {
    u16    size;
    u8    *data;
};

/**
 * struct iwl_phy_db - stores phy configuration and calibration data.
 *
 * @cfg: phy configuration.
 * @calib_nch: non channel specific calibration data.
 * @calib_ch: channel specific calibration data.
 * @n_group_papd: number of entries in papd channel group.
 * @calib_ch_group_papd: calibration data related to papd channel group.
 * @n_group_txp: number of entries in tx power channel group.
 * @calib_ch_group_txp: calibration data related to tx power chanel group.
 */
struct iwl_phy_db {
    struct iwl_phy_db_entry    cfg;
    struct iwl_phy_db_entry    calib_nch;
    int n_group_papd;
    struct iwl_phy_db_entry    *calib_ch_group_papd;
    int n_group_txp;
    struct iwl_phy_db_entry    *calib_ch_group_txp;
    IWLTransport *trans;
};

enum iwl_phy_db_section_type {
    IWL_PHY_DB_CFG = 1,
    IWL_PHY_DB_CALIB_NCH,
    IWL_PHY_DB_UNUSED,
    IWL_PHY_DB_CALIB_CHG_PAPD,
    IWL_PHY_DB_CALIB_CHG_TXP,
    IWL_PHY_DB_MAX
};

#define PHY_DB_CMD 0x6c

/* for parsing of tx power channel group data that comes from the firmware*/
struct iwl_phy_db_chg_txp {
    __le32 space;
    __le16 max_channel_idx;
} __packed;


void iwl_phy_db_init(IWLTransport *trans, struct iwl_phy_db *phy_db);

void iwl_phy_db_free(struct iwl_phy_db *phy_db);

int iwl_phy_db_set_section(struct iwl_phy_db *phy_db,
               struct iwl_rx_packet *pkt);

int iwl_phy_db_get_section_data(struct iwl_phy_db *phy_db,
                                u32 type, u8 **data, u16 *size, u16 ch_id);

int iwl_send_phy_db_data(struct iwl_phy_db *phy_db);

#endif /* IWLPhyDb_hpp */
