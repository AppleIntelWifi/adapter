//
//  IWLUcodeParse.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLUcodeParse.hpp"
#include "FWFile.h"
#include <linux/types.h>

IWLUcodeParse::IWLUcodeParse(IWLDevice *drv)
{
    this->drv = drv;
}

IWLUcodeParse::~IWLUcodeParse()
{
    this->drv = NULL;
}

bool IWLUcodeParse::parseFW(const void *raw, size_t len, struct iwl_fw *fw, struct iwl_firmware_pieces *pieces)
{
    /* Data from ucode file:  header followed by uCode images */
    iwl_ucode_header *ucode = (struct iwl_ucode_header*)raw;
    if (ucode->ver) {
        return parseV1V2Firmware(raw, len, fw, pieces);
    } else {
        return parseTLVFirmware(raw, len, fw, pieces);
    }
}

#define IWLAGN_RTC_INST_LOWER_BOUND        (0x000000)
#define IWLAGN_RTC_INST_UPPER_BOUND        (0x020000)
#define IWLAGN_RTC_DATA_LOWER_BOUND        (0x800000)
#define IWLAGN_RTC_DATA_UPPER_BOUND        (0x80C000)

bool IWLUcodeParse::parseV1V2Firmware(const void *raw, size_t len, struct iwl_fw *fw, struct iwl_firmware_pieces *pieces)
{
    struct iwl_ucode_header *ucode = (struct iwl_ucode_header *)raw;
    u32 api_ver, hdr_size, build;
    char buildstr[25];
    const u8 *src;
    
    fw->ucode_ver = le32_to_cpu(ucode->ver);
    api_ver = IWL_UCODE_API(fw->ucode_ver);
    switch (api_ver) {
        default:
            hdr_size = 28;
            if (len < hdr_size) {
                IWL_ERR(0, "File size too small!\n");
                return false;
            }
            build = le32_to_cpu(ucode->u.v2.build);
            setSecSize(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_INST,
                       le32_to_cpu(ucode->u.v2.inst_size));
            setSecSize(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_DATA,
                       le32_to_cpu(ucode->u.v2.data_size));
            setSecSize(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_INST,
                       le32_to_cpu(ucode->u.v2.init_size));
            setSecSize(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_DATA,
                       le32_to_cpu(ucode->u.v2.init_data_size));
            src = ucode->u.v2.data;
            break;
        case 0:
        case 1:
        case 2:
            hdr_size = 24;
            if (len < hdr_size) {
                IWL_ERR(0, "File size too small!\n");
                return false;
            }
            build = 0;
            setSecSize(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_INST,
                       le32_to_cpu(ucode->u.v1.inst_size));
            setSecSize(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_DATA,
                       le32_to_cpu(ucode->u.v1.data_size));
            setSecSize(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_INST,
                       le32_to_cpu(ucode->u.v1.init_size));
            setSecSize(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_DATA,
                       le32_to_cpu(ucode->u.v1.init_data_size));
            src = ucode->u.v1.data;
            break;
    }
    
    if (build)
        snprintf(buildstr, sizeof(buildstr), " build %u", build);
    else
        buildstr[0] = '\0';
    
    snprintf(fw->fw_version,
             sizeof(fw->fw_version),
             "%u.%u.%u.%u%s %s",
             IWL_UCODE_MAJOR(fw->ucode_ver),
             IWL_UCODE_MINOR(fw->ucode_ver),
             IWL_UCODE_API(fw->ucode_ver),
             IWL_UCODE_SERIAL(fw->ucode_ver),
             buildstr, reducedFWName(drv->name));
    
    /* Verify size of file vs. image size info in file's header */
    if (len != hdr_size +
        getSecSize(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_INST) +
        getSecSize(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_DATA) +
        getSecSize(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_INST) +
        getSecSize(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_DATA)) {
        
        IWL_ERR(0,
                "uCode file size %d does not match expected size\n",
                (int)len);
        return -EINVAL;
    }
    
    setSecData(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_INST, src);
    src += getSecSize(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_INST);
    setSecOffset(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_INST,
                 IWLAGN_RTC_INST_LOWER_BOUND);
    setSecData(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_DATA, src);
    src += getSecSize(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_DATA);
    setSecOffset(pieces, IWL_UCODE_REGULAR, IWL_UCODE_SECTION_DATA,
                 IWLAGN_RTC_DATA_LOWER_BOUND);
    setSecData(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_INST, src);
    src += getSecSize(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_INST);
    setSecOffset(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_INST,
                 IWLAGN_RTC_INST_LOWER_BOUND);
    setSecData(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_DATA, src);
    src += getSecSize(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_DATA);
    setSecOffset(pieces, IWL_UCODE_INIT, IWL_UCODE_SECTION_DATA,
                 IWLAGN_RTC_DATA_LOWER_BOUND);
    
    return true;
}

bool IWLUcodeParse::parseTLVFirmware(const void *raw, size_t len, struct iwl_fw *fw, struct iwl_firmware_pieces *pieces)
{
    struct iwl_tlv_ucode_header *ucode = (struct iwl_tlv_ucode_header *)raw;
    struct iwl_ucode_tlv *tlv;
    const u8 *data;
    u32 tlv_len;
    u32 usniffer_img;
    enum iwl_ucode_tlv_type tlv_type;
    const u8 *tlv_data;
    char buildstr[25];
    u32 build, paging_mem_size;
    int num_of_cpus;
    bool usniffer_req = false;
    iwl_ucode_capabilities *capa = &fw->ucode_capa;
    
    
    if (len < sizeof(*ucode)) {
        IWL_ERR(0, "uCode has invalid length: %zd\n", len);
        return false;
    }
    
    if (ucode->magic != cpu_to_le32(IWL_TLV_UCODE_MAGIC)) {
        IWL_ERR(0, "invalid uCode magic: 0X%x\n",
                le32_to_cpu(ucode->magic));
        return false;
    }
    
    fw->ucode_ver = le32_to_cpu(ucode->ver);
    memcpy(fw->human_readable, ucode->human_readable,
           sizeof(fw->human_readable));
    build = le32_to_cpu(ucode->build);
    
    if (build)
        snprintf(buildstr, sizeof(buildstr), " build %u", build);
    else
        buildstr[0] = '\0';
    
    snprintf(fw->fw_version,
             sizeof(fw->fw_version),
             "%u.%u.%u.%u%s %s",
             IWL_UCODE_MAJOR(fw->ucode_ver),
             IWL_UCODE_MINOR(fw->ucode_ver),
             IWL_UCODE_API(fw->ucode_ver),
             IWL_UCODE_SERIAL(fw->ucode_ver),
             buildstr, reducedFWName(drv->name));
    
    data = ucode->data;
    
    len -= sizeof(*ucode);
    
    while (len >= sizeof(*tlv)) {
        len -= sizeof(*tlv);
        tlv = (iwl_ucode_tlv *)data;
        
        tlv_len = le32_to_cpu(tlv->length);
        tlv_type = (enum iwl_ucode_tlv_type)le32_to_cpu(tlv->type);
        tlv_data = tlv->data;
        
        if (len < tlv_len) {
            IWL_ERR(0, "invalid TLV len: %zd/%u\n",
                    len, tlv_len);
            return false;
        }
        len -= _ALIGN(tlv_len, 4);
        data += sizeof(*tlv) + _ALIGN(tlv_len, 4);
        
        switch (tlv_type) {
            case IWL_UCODE_TLV_INST:
                setSecData(pieces, IWL_UCODE_REGULAR,
                           IWL_UCODE_SECTION_INST, tlv_data);
                setSecSize(pieces, IWL_UCODE_REGULAR,
                           IWL_UCODE_SECTION_INST, tlv_len);
                setSecOffset(pieces, IWL_UCODE_REGULAR,
                             IWL_UCODE_SECTION_INST,
                             IWLAGN_RTC_INST_LOWER_BOUND);
                break;
            case IWL_UCODE_TLV_DATA:
                setSecData(pieces, IWL_UCODE_REGULAR,
                           IWL_UCODE_SECTION_DATA, tlv_data);
                setSecSize(pieces, IWL_UCODE_REGULAR,
                           IWL_UCODE_SECTION_DATA, tlv_len);
                setSecOffset(pieces, IWL_UCODE_REGULAR,
                             IWL_UCODE_SECTION_DATA,
                             IWLAGN_RTC_DATA_LOWER_BOUND);
                break;
            case IWL_UCODE_TLV_INIT:
                setSecData(pieces, IWL_UCODE_INIT,
                           IWL_UCODE_SECTION_INST, tlv_data);
                setSecSize(pieces, IWL_UCODE_INIT,
                           IWL_UCODE_SECTION_INST, tlv_len);
                setSecOffset(pieces, IWL_UCODE_INIT,
                             IWL_UCODE_SECTION_INST,
                             IWLAGN_RTC_INST_LOWER_BOUND);
                break;
            case IWL_UCODE_TLV_INIT_DATA:
                setSecData(pieces, IWL_UCODE_INIT,
                           IWL_UCODE_SECTION_DATA, tlv_data);
                setSecSize(pieces, IWL_UCODE_INIT,
                           IWL_UCODE_SECTION_DATA, tlv_len);
                setSecOffset(pieces, IWL_UCODE_INIT,
                             IWL_UCODE_SECTION_DATA,
                             IWLAGN_RTC_DATA_LOWER_BOUND);
                break;
            case IWL_UCODE_TLV_BOOT:
                IWL_ERR(0, "Found unexpected BOOT ucode\n");
                break;
            case IWL_UCODE_TLV_PROBE_MAX_LEN:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                capa->max_probe_length =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_PAN:
                if (tlv_len)
                    goto invalid_tlv_len;
                capa->flags |= IWL_UCODE_TLV_FLAGS_PAN;
                break;
            case IWL_UCODE_TLV_FLAGS:
                /* must be at least one u32 */
                if (tlv_len < sizeof(u32))
                    goto invalid_tlv_len;
                /* and a proper number of u32s */
                if (tlv_len % sizeof(u32))
                    goto invalid_tlv_len;
                /*
                 * This driver only reads the first u32 as
                 * right now no more features are defined,
                 * if that changes then either the driver
                 * will not work with the new firmware, or
                 * it'll not take advantage of new features.
                 */
                capa->flags = le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_API_CHANGES_SET:
                if (tlv_len != sizeof(struct iwl_ucode_api))
                    goto invalid_tlv_len;
                setUcodeApiFlags(tlv_data, capa);
                break;
            case IWL_UCODE_TLV_ENABLED_CAPABILITIES:
                if (tlv_len != sizeof(struct iwl_ucode_capa))
                    goto invalid_tlv_len;
                setUcodeCapabilities(tlv_data, capa);
                break;
            case IWL_UCODE_TLV_INIT_EVTLOG_PTR:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                pieces->init_evtlog_ptr =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_INIT_EVTLOG_SIZE:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                pieces->init_evtlog_size =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_INIT_ERRLOG_PTR:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                pieces->init_errlog_ptr =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_RUNT_EVTLOG_PTR:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                pieces->inst_evtlog_ptr =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_RUNT_EVTLOG_SIZE:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                pieces->inst_evtlog_size =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_RUNT_ERRLOG_PTR:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                pieces->inst_errlog_ptr =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_ENHANCE_SENS_TBL:
                if (tlv_len)
                    goto invalid_tlv_len;
                fw->enhance_sensitivity_table = true;
                break;
            case IWL_UCODE_TLV_WOWLAN_INST:
                setSecData(pieces, IWL_UCODE_WOWLAN,
                           IWL_UCODE_SECTION_INST, tlv_data);
                setSecSize(pieces, IWL_UCODE_WOWLAN,
                           IWL_UCODE_SECTION_INST, tlv_len);
                setSecOffset(pieces, IWL_UCODE_WOWLAN,
                             IWL_UCODE_SECTION_INST,
                             IWLAGN_RTC_INST_LOWER_BOUND);
                break;
            case IWL_UCODE_TLV_WOWLAN_DATA:
                setSecData(pieces, IWL_UCODE_WOWLAN,
                           IWL_UCODE_SECTION_DATA, tlv_data);
                setSecSize(pieces, IWL_UCODE_WOWLAN,
                           IWL_UCODE_SECTION_DATA, tlv_len);
                setSecOffset(pieces, IWL_UCODE_WOWLAN,
                             IWL_UCODE_SECTION_DATA,
                             IWLAGN_RTC_DATA_LOWER_BOUND);
                break;
            case IWL_UCODE_TLV_PHY_CALIBRATION_SIZE:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                capa->standard_phy_calibration_size =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_SEC_RT:
                storeUcodeSec(pieces, tlv_data, IWL_UCODE_REGULAR,
                              tlv_len);
                fw->type = IWL_FW_MVM;
                break;
            case IWL_UCODE_TLV_SEC_INIT:
                storeUcodeSec(pieces, tlv_data, IWL_UCODE_INIT,
                              tlv_len);
                fw->type = IWL_FW_MVM;
                break;
            case IWL_UCODE_TLV_SEC_WOWLAN:
                storeUcodeSec(pieces, tlv_data, IWL_UCODE_WOWLAN,
                              tlv_len);
                fw->type = IWL_FW_MVM;
                break;
            case IWL_UCODE_TLV_DEF_CALIB:
                if (tlv_len != sizeof(struct iwl_tlv_calib_data))
                    goto invalid_tlv_len;
                if (setDefaultCalib(tlv_data))
                    goto tlv_error;
                break;
            case IWL_UCODE_TLV_PHY_SKU:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                fw->phy_config = le32_to_cpup((__le32 *)tlv_data);
#ifdef CPTCFG_IWLWIFI_SUPPORT_DEBUG_OVERRIDES
                if (drv->trans->dbg_cfg.valid_ants & ~ANT_ABC)
                    IWL_ERR(0,
                            "Invalid value for antennas: 0x%x\n",
                            drv->trans->dbg_cfg.valid_ants);
                /* Make sure value stays in range */
                drv->trans->dbg_cfg.valid_ants &= ANT_ABC;
                if (drv->trans->dbg_cfg.valid_ants) {
                    u32 phy_config = ~(FW_PHY_CFG_TX_CHAIN |
                                       FW_PHY_CFG_RX_CHAIN);
                    
                    phy_config |=
                    (drv->trans->dbg_cfg.valid_ants <<
                     FW_PHY_CFG_TX_CHAIN_POS);
                    phy_config |=
                    (drv->trans->dbg_cfg.valid_ants <<
                     FW_PHY_CFG_RX_CHAIN_POS);
                    
                    fw->phy_config &= phy_config;
                }
#endif
                fw->valid_tx_ant = (fw->phy_config &
                                    FW_PHY_CFG_TX_CHAIN) >>
                FW_PHY_CFG_TX_CHAIN_POS;
                fw->valid_rx_ant = (fw->phy_config &
                                    FW_PHY_CFG_RX_CHAIN) >>
                FW_PHY_CFG_RX_CHAIN_POS;
                break;
            case IWL_UCODE_TLV_SECURE_SEC_RT:
                storeUcodeSec(pieces, tlv_data, IWL_UCODE_REGULAR,
                              tlv_len);
                fw->type = IWL_FW_MVM;
                break;
            case IWL_UCODE_TLV_SECURE_SEC_INIT:
                storeUcodeSec(pieces, tlv_data, IWL_UCODE_INIT,
                              tlv_len);
                fw->type = IWL_FW_MVM;
                break;
            case IWL_UCODE_TLV_SECURE_SEC_WOWLAN:
                storeUcodeSec(pieces, tlv_data, IWL_UCODE_WOWLAN,
                              tlv_len);
                fw->type = IWL_FW_MVM;
                break;
            case IWL_UCODE_TLV_NUM_OF_CPU:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                num_of_cpus =
                le32_to_cpup((__le32 *)tlv_data);
                
                if (num_of_cpus == 2) {
                    fw->img[IWL_UCODE_REGULAR].is_dual_cpus =
                    true;
                    fw->img[IWL_UCODE_INIT].is_dual_cpus =
                    true;
                    fw->img[IWL_UCODE_WOWLAN].is_dual_cpus =
                    true;
                } else if ((num_of_cpus > 2) || (num_of_cpus < 1)) {
                    IWL_ERR(drv, "Driver support upto 2 CPUs\n");
                    return false;
                }
                break;
            case IWL_UCODE_TLV_CSCHEME:
                if (storeCscheme(fw, tlv_data, tlv_len))
                    goto invalid_tlv_len;
                break;
            case IWL_UCODE_TLV_N_SCAN_CHANNELS:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                capa->n_scan_channels =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            case IWL_UCODE_TLV_FW_VERSION: {
                __le32 *ptr = (__le32 *)tlv_data;
                u32 major, minor;
                u8 local_comp;
                
                if (tlv_len != sizeof(u32) * 3)
                    goto invalid_tlv_len;
                
                major = le32_to_cpup(ptr++);
                minor = le32_to_cpup(ptr++);
                local_comp = le32_to_cpup(ptr);
                
                if (strncmp((const char *)fw->human_readable, "stream:", 7))
                    snprintf(fw->fw_version,
                             sizeof(fw->fw_version),
                             "%u.%08x.%u %s", major, minor,
                             local_comp, reducedFWName(drv->name));
                else
                    snprintf(fw->fw_version,
                             sizeof(fw->fw_version),
                             "%u.%u.%u %s", major, minor,
                             local_comp, reducedFWName(drv->name));
                break;
            }
            case IWL_UCODE_TLV_FW_DBG_DEST: {
                //                struct iwl_fw_dbg_dest_tlv *dest = NULL;
                //                struct iwl_fw_dbg_dest_tlv_v1 *dest_v1 = NULL;
                //                u8 mon_mode;
                //
                //                pieces->dbg_dest_ver = (u8 *)tlv_data;
                //                if (*pieces->dbg_dest_ver == 1) {
                //                    dest = (void *)tlv_data;
                //                } else if (*pieces->dbg_dest_ver == 0) {
                //                    dest_v1 = (void *)tlv_data;
                //                } else {
                //                    IWL_ERR(0,
                //                            "The version is %d, and it is invalid\n",
                //                            *pieces->dbg_dest_ver);
                //                    break;
                //                }
                //
                //                    #ifdef CPTCFG_IWLWIFI_DEVICE_TESTMODE
                //                    #ifdef CPTCFG_IWLWIFI_SUPPORT_DEBUG_OVERRIDES
                //                                if (drv->trans->dbg_cfg.dbm_destination_path) {
                //                                    IWL_ERR(drv,
                //                                        "Ignoring destination, ini file present\n");
                //                                    break;
                //                                }
                //                    #endif
                //                    #endif
                //
                //                if (pieces->dbg_dest_tlv_init) {
                //                    IWL_ERR(0,
                //                            "dbg destination ignored, already exists\n");
                //                    break;
                //                }
                //
                //                pieces->dbg_dest_tlv_init = true;
                //
                //                if (dest_v1) {
                //                    pieces->dbg_dest_tlv_v1 = dest_v1;
                //                    mon_mode = dest_v1->monitor_mode;
                //                } else {
                //                    pieces->dbg_dest_tlv = dest;
                //                    mon_mode = dest->monitor_mode;
                //                }
                
                //                IWL_INFO(drv, "Found debug destination: %s\n",
                //                         get_fw_dbg_mode_string(mon_mode));
                //
                //                fw->dbg.n_dest_reg = (dest_v1) ?
                //                tlv_len -
                //                offsetof(struct iwl_fw_dbg_dest_tlv_v1,
                //                         reg_ops) :
                //                tlv_len -
                //                offsetof(struct iwl_fw_dbg_dest_tlv,
                //                         reg_ops);
                //
                //                fw->dbg.n_dest_reg /=
                //                sizeof(fw->dbg.dest_tlv->reg_ops[0]);
                
                IWL_INFO(drv, "Found debug destination: \n");
                
                break;
            }
            case IWL_UCODE_TLV_FW_DBG_CONF: {
                //                struct iwl_fw_dbg_conf_tlv *conf = (struct iwl_fw_dbg_conf_tlv *)tlv_data;
                //
                //                if (!pieces->dbg_dest_tlv_init) {
                //                    IWL_ERR(0,
                //                            "Ignore dbg config %d - no destination configured\n",
                //                            conf->id);
                //                    break;
                //                }
                //
                //                if (conf->id >= ARRAY_SIZE(fw->dbg.conf_tlv)) {
                //                    IWL_ERR(0,
                //                            "Skip unknown configuration: %d\n",
                //                            conf->id);
                //                    break;
                //                }
                //
                //                if (pieces->dbg_conf_tlv[conf->id]) {
                //                    IWL_ERR(0,
                //                            "Ignore duplicate dbg config %d\n",
                //                            conf->id);
                //                    break;
                //                }
                //
                //                if (conf->usniffer)
                //                    usniffer_req = true;
                //
                //                IWL_INFO(0, "Found debug configuration: %d\n",
                //                         conf->id);
                //
                //                pieces->dbg_conf_tlv[conf->id] = conf;
                //                pieces->dbg_conf_tlv_len[conf->id] = tlv_len;
                break;
            }
            case IWL_UCODE_TLV_FW_DBG_TRIGGER: {
                //                struct iwl_fw_dbg_trigger_tlv *trigger =
                //                (struct iwl_fw_dbg_trigger_tlv *)tlv_data;
                //                u32 trigger_id = le32_to_cpu(trigger->id);
                //
                //                if (trigger_id >= ARRAY_SIZE(fw->dbg.trigger_tlv)) {
                //                    IWL_ERR(0,
                //                            "Skip unknown trigger: %u\n",
                //                            trigger->id);
                //                    break;
                //                }
                //
                //                if (pieces->dbg_trigger_tlv[trigger_id]) {
                //                    IWL_ERR(0,
                //                            "Ignore duplicate dbg trigger %u\n",
                //                            trigger->id);
                //                    break;
                //                }
                //
                //                IWL_INFO(0, "Found debug trigger: %u\n", trigger->id);
                //
                //                pieces->dbg_trigger_tlv[trigger_id] = trigger;
                //                pieces->dbg_trigger_tlv_len[trigger_id] = tlv_len;
                break;
            }
            case IWL_UCODE_TLV_FW_DBG_DUMP_LST: {
                if (tlv_len != sizeof(u32)) {
                    IWL_ERR(0,
                            "dbg lst mask size incorrect, skip\n");
                    break;
                }
                
                fw->dbg.dump_mask =
                le32_to_cpup((__le32 *)tlv_data);
                break;
            }
            case IWL_UCODE_TLV_SEC_RT_USNIFFER:
                drv->usniffer_images = true;
                storeUcodeSec(pieces, tlv_data,
                              IWL_UCODE_REGULAR_USNIFFER,
                              tlv_len);
                break;
            case IWL_UCODE_TLV_PAGING:
                if (tlv_len != sizeof(u32))
                    goto invalid_tlv_len;
                paging_mem_size = le32_to_cpup((__le32 *)tlv_data);
                
                IWL_INFO(0,
                         "Paging: paging enabled (size = %u bytes)\n",
                         paging_mem_size);
                
                if (paging_mem_size > MAX_PAGING_IMAGE_SIZE) {
                    IWL_ERR(0,
                            "Paging: driver supports up to %lu bytes for paging image\n",
                            MAX_PAGING_IMAGE_SIZE);
                    return false;
                }
                
                if (paging_mem_size & (FW_PAGING_SIZE - 1)) {
                    IWL_ERR(0,
                            "Paging: image isn't multiple %lu\n",
                            FW_PAGING_SIZE);
                    return false;
                }
                
                fw->img[IWL_UCODE_REGULAR].paging_mem_size =
                paging_mem_size;
                usniffer_img = IWL_UCODE_REGULAR_USNIFFER;
                fw->img[usniffer_img].paging_mem_size =
                paging_mem_size;
                break;
            case IWL_UCODE_TLV_FW_GSCAN_CAPA:
                /* ignored */
                break;
            case IWL_UCODE_TLV_FW_MEM_SEG: {
                //                struct iwl_fw_dbg_mem_seg_tlv *dbg_mem =
                //                (void *)tlv_data;
                //                size_t size;
                //                struct iwl_fw_dbg_mem_seg_tlv *n;
                //
                //                if (tlv_len != (sizeof(*dbg_mem)))
                //                    goto invalid_tlv_len;
                //
                //                IWL_DEBUG_INFO(0, "Found debug memory segment: %u\n",
                //                               dbg_mem->data_type);
                //
                //                size = sizeof(*pieces->dbg_mem_tlv) *
                //                (pieces->n_mem_tlv + 1);
                //                n = krealloc(pieces->dbg_mem_tlv, size, GFP_KERNEL);
                //                if (!n)
                //                    return false;
                //                pieces->dbg_mem_tlv = n;
                //                pieces->dbg_mem_tlv[pieces->n_mem_tlv] = *dbg_mem;
                //                pieces->n_mem_tlv++;
                break;
            }
            case IWL_UCODE_TLV_IML: {
                fw->iml_len = tlv_len;
                fw->iml = (u8 *)kmemdup((const void *)tlv_data, tlv_len);
                if (!fw->iml)
                    return false;
                break;
            }
            case IWL_UCODE_TLV_FW_RECOVERY_INFO: {
                typedef struct {
                    __le32 buf_addr;
                    __le32 buf_size;
                } RecoverInfo, *PRecoverInfo;
                PRecoverInfo recov_info = (PRecoverInfo)tlv_data;
                
                if (tlv_len != sizeof(RecoverInfo))
                    goto invalid_tlv_len;
                capa->error_log_addr =
                le32_to_cpu(recov_info->buf_addr);
                capa->error_log_size =
                le32_to_cpu(recov_info->buf_size);
            }
                break;
            case IWL_UCODE_TLV_FW_FSEQ_VERSION: {
                typedef struct {
                    u8 version[32];
                    u8 sha1[20];
                } SEQVER, *PSEQVER;
                PSEQVER fseq_ver = (PSEQVER)tlv_data;
                
                if (tlv_len != sizeof(SEQVER))
                    goto invalid_tlv_len;
                IWL_INFO(0, "TLV_FW_FSEQ_VERSION: %s\n",
                         fseq_ver->version);
            }
                break;
            case IWL_UCODE_TLV_UMAC_DEBUG_ADDRS: {
                //                struct iwl_umac_debug_addrs *dbg_ptrs =
                //                (iwl_umac_debug_addrs *)tlv_data;
                //
                //                if (tlv_len != sizeof(*dbg_ptrs))
                //                    goto invalid_tlv_len;
                //                if (drv->trans->trans_cfg->device_family <
                //                    IWL_DEVICE_FAMILY_22000)
                //                    break;
                //                drv->trans->dbg.umac_error_event_table =
                //                le32_to_cpu(dbg_ptrs->error_info_addr) &
                //                ~FW_ADDR_CACHE_CONTROL;
                //                drv->trans->dbg.error_event_table_tlv_status |=
                //                IWL_ERROR_EVENT_TABLE_UMAC;
                break;
            }
            case IWL_UCODE_TLV_LMAC_DEBUG_ADDRS: {
                //                struct iwl_lmac_debug_addrs *dbg_ptrs =
                //                (void *)tlv_data;
                //
                //                if (tlv_len != sizeof(*dbg_ptrs))
                //                    goto invalid_tlv_len;
                //                if (drv->trans->trans_cfg->device_family <
                //                    IWL_DEVICE_FAMILY_22000)
                //                    break;
                //                drv->trans->dbg.lmac_error_event_table[0] =
                //                le32_to_cpu(dbg_ptrs->error_event_table_ptr) &
                //                ~FW_ADDR_CACHE_CONTROL;
                //                drv->trans->dbg.error_event_table_tlv_status |=
                //                IWL_ERROR_EVENT_TABLE_LMAC1;
                break;
            }
            case IWL_UCODE_TLV_TYPE_DEBUG_INFO:
            case IWL_UCODE_TLV_TYPE_BUFFER_ALLOCATION:
            case IWL_UCODE_TLV_TYPE_HCMD:
            case IWL_UCODE_TLV_TYPE_REGIONS:
            case IWL_UCODE_TLV_TYPE_TRIGGERS:
                //                if (iwlwifi_mod_params.enable_ini)
                //                    iwl_dbg_tlv_alloc(drv->trans, tlv, false);
                break;
            case IWL_UCODE_TLV_CMD_VERSIONS:
                if (tlv_len % sizeof(struct iwl_fw_cmd_version)) {
                    IWL_ERR(0,
                            "Invalid length for command versions: %u\n",
                            tlv_len);
                    tlv_len /= sizeof(struct iwl_fw_cmd_version);
                    tlv_len *= sizeof(struct iwl_fw_cmd_version);
                }
                if (WARN_ON(capa->cmd_versions))
                    return false;
                capa->cmd_versions = (iwl_fw_cmd_version *)kmemdup(tlv_data, tlv_len);
                if (!capa->cmd_versions)
                    return false;
                capa->n_cmd_versions =
                tlv_len / sizeof(struct iwl_fw_cmd_version);
                break;
            default:
                IWL_INFO(0, "unknown TLV: %d\n", tlv_type);
                break;
        }
    }
    
    if (!fw_has_capa(capa, IWL_UCODE_TLV_CAPA_USNIFFER_UNIFIED) &&
        usniffer_req && !drv->usniffer_images) {
        IWL_ERR(0,
                "user selected to work with usniffer but usniffer image isn't available in ucode package\n");
        return false;
    }
    
    if (len) {
        IWL_ERR(0, "invalid TLV after parsing: %zd\n", len);
        goto tlv_error;
    }
    
    return true;
invalid_tlv_len:
    IWL_ERR(0, "TLV %d has invalid size: %u\n", tlv_type, tlv_len);
tlv_error:
    IWL_ERR(0, "Error TLV\n");
    //        iwl_print_hex_dump(drv, IWL_DL_FW, (u8 *)data, len);
    return false;
}

struct fw_sec *IWLUcodeParse::getSec(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec)
{
    return &pieces->img[type].sec[sec];
}

void IWLUcodeParse::alloccSecData(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec)
{
    struct fw_img_parsing *img = &pieces->img[type];
    struct fw_sec *sec_memory;
    int size = sec + 1;
    size_t alloc_size = sizeof(*img->sec) * size;
    
    if (img->sec && img->sec_counter >= size)
        return;
    sec_memory = (fw_sec *)IOMalloc(alloc_size);
    
    if (!sec_memory)
        return;
    
    memcpy((void *)sec_memory, (void *)img->sec, sizeof(*img->sec) * sec);
    IOFree((void *)img->sec, sizeof(*img->sec) * sec);
    
    img->sec = sec_memory;
    img->sec_counter = size;
}

void IWLUcodeParse::setSecData(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec, const void *data)
{
    alloccSecData(pieces, type, sec);
    pieces->img[type].sec[sec].data = data;
}

void IWLUcodeParse::setSecSize(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec, size_t size)
{
    alloccSecData(pieces, type, sec);
    pieces->img[type].sec[sec].size = size;
}

void IWLUcodeParse::setSecOffset(iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec, u32 offset)
{
    alloccSecData(pieces, type, sec);
    pieces->img[type].sec[sec].offset = offset;
}

size_t IWLUcodeParse::getSecSize(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec)
{
    return pieces->img[type].sec[sec].size;
}

/*
 * Gets uCode section from tlv.
 */
int IWLUcodeParse::storeUcodeSec(struct iwl_firmware_pieces *pieces, const void *data, enum iwl_ucode_type type, int size)
{
    struct fw_img_parsing *img;
    struct fw_sec *sec;
    struct fw_sec_parsing *sec_parse;
    size_t alloc_size;
    if (WARN_ON(!pieces || !data || type >= IWL_UCODE_TYPE_MAX))
        return -1;
    
    sec_parse = (struct fw_sec_parsing *)data;
    
    img = &pieces->img[type];
    
    alloc_size = sizeof(*img->sec) * (img->sec_counter + 1);
    
    //TODO 这里替换了krealloc
    sec = (fw_sec *)IOMalloc(alloc_size);
    if (!sec)
        return -ENOMEM;
    
    memcpy((void *)sec, (void *)img->sec, sizeof(*img->sec) * img->sec_counter);
    IOFree((void *)img->sec, sizeof(*img->sec) * img->sec_counter);
    
    img->sec = sec;
    
    sec = &img->sec[img->sec_counter];
    
    sec->offset = le32_to_cpu(sec_parse->offset);
    sec->data = sec_parse->data;
    sec->size = size - sizeof(sec_parse->offset);
    
    ++img->sec_counter;
    return 0;
}

void IWLUcodeParse::setUcodeApiFlags(const u8 *data, struct iwl_ucode_capabilities *capa)
{
    const struct iwl_ucode_api *ucode_api = (iwl_ucode_api *)data;
    u32 api_index = le32_to_cpu(ucode_api->api_index);
    u32 api_flags = le32_to_cpu(ucode_api->api_flags);
    int i;
    
    if (api_index >= DIV_ROUND_UP(NUM_IWL_UCODE_TLV_API, 32)) {
        IWL_WARN(0,
                 "api flags index %d larger than supported by driver\n",
                 api_index);
        return;
    }
    
    for (i = 0; i < 32; i++) {
        if (api_flags & BIT(i))
            set_bit(i + 32 * api_index, capa->_api);
    }
}

void IWLUcodeParse::setUcodeCapabilities(const u8 *data, struct iwl_ucode_capabilities *capa)
{
    const struct iwl_ucode_capa *ucode_capa = (iwl_ucode_capa *)data;
    u32 api_index = le32_to_cpu(ucode_capa->api_index);
    u32 api_flags = le32_to_cpu(ucode_capa->api_capa);
    int i;
    
    if (api_index >= DIV_ROUND_UP(NUM_IWL_UCODE_TLV_CAPA, 32)) {
        IWL_WARN(0,
                 "capa flags index %d larger than supported by driver\n",
                 api_index);
        return;
    }
    
    for (i = 0; i < 32; i++) {
        if (api_flags & BIT(i))
            set_bit(i + 32 * api_index, capa->_capa);
    }
}

int IWLUcodeParse::setDefaultCalib(const u8 *data)
{
    struct iwl_tlv_calib_data *def_calib =
    (struct iwl_tlv_calib_data *)data;
    u32 ucode_type = le32_to_cpu(def_calib->ucode_type);
    if (ucode_type >= IWL_UCODE_TYPE_MAX) {
        IWL_ERR(0, "Wrong ucode_type %u for default calibration.\n",
                ucode_type);
        return -EINVAL;
    }
    drv->fw.default_calib[ucode_type].flow_trigger =
    def_calib->calib.flow_trigger;
    drv->fw.default_calib[ucode_type].event_trigger =
    def_calib->calib.event_trigger;
    
    return 0;
}

int IWLUcodeParse::storeCscheme(struct iwl_fw *fw, const u8 *data, const u32 len)
{
    int i, j;
    struct iwl_fw_cscheme_list *l = (struct iwl_fw_cscheme_list *)data;
    struct iwl_fw_cipher_scheme *fwcs;
    
    if (len < sizeof(*l) ||
        len < sizeof(l->size) + l->size * sizeof(l->cs[0]))
        return -EINVAL;
    
    for (i = 0, j = 0; i < IWL_UCODE_MAX_CS && i < l->size; i++) {
        fwcs = &l->cs[j];
        
        /* we skip schemes with zero cipher suite selector */
        if (!fwcs->cipher)
            continue;
        
        fw->cs[j++] = *fwcs;
    }
    
    return 0;
}

const char *IWLUcodeParse::reducedFWName(const char *name)
{
    if (strncmp(name, "iwlwifi-", 8) == 0) {
        name += 8;
    }
    return name;
}

int IWLUcodeParse::allocUcode(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type)
{
    int i;
    struct fw_desc *sec;
    
    sec = (fw_desc *)kcalloc(pieces->img[type].sec_counter, sizeof(*sec));
    if (!sec)
        return -ENOMEM;
    drv->fw.img[type].sec = sec;
    drv->fw.img[type].num_sec = pieces->img[type].sec_counter;
    
    for (i = 0; i < pieces->img[type].sec_counter; i++)
        if (allocFWDesc(&sec[i], getSec(pieces, type, i)))
            return -ENOMEM;
    return 0;
}

int IWLUcodeParse::allocFWDesc(struct fw_desc *desc, struct fw_sec *sec)
{
    void *data;
    
    desc->data = NULL;
    
    if (!sec || !sec->size)
        return -EINVAL;
    
    data = IOMalloc(sec->size);
    if (!data)
        return -ENOMEM;
    
    desc->len = sec->size;
    desc->offset = sec->offset;
    memcpy(data, sec->data, desc->len);
    desc->data = data;
    
    return 0;
}

void IWLUcodeParse::freeFWDesc(struct fw_desc *desc)
{
    IOFree((void *)desc->data, desc->len);
    desc->data = NULL;
    desc->len = 0;
}

void IWLUcodeParse::freeFWImg(struct fw_img *img)
{
    int i;
    for (i = 0; i < img->num_sec; i++)
        freeFWDesc(&img->sec[i]);
    IOFree(img->sec, sizeof(fw_desc));
}

void IWLUcodeParse::deAllocUcode()
{
    int i;
    
    //        kfree(drv->fw.dbg.dest_tlv);
    //        for (i = 0; i < ARRAY_SIZE(drv->fw.dbg.conf_tlv); i++)
    //            kfree(drv->fw.dbg.conf_tlv[i]);
    //        for (i = 0; i < ARRAY_SIZE(drv->fw.dbg.trigger_tlv); i++)
    //            kfree(drv->fw.dbg.trigger_tlv[i]);
    //        kfree(drv->fw.dbg.mem_tlv);
    IOFree(drv->fw.iml, drv->fw.iml_len);
    IOFree((void *)drv->fw.ucode_capa.cmd_versions, sizeof(iwl_fw_cmd_version));
    
    for (i = 0; i < IWL_UCODE_TYPE_MAX; i++)
        freeFWImg(drv->fw.img + i);
}
