//
//  IWLNvm.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/8.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmDriver.hpp"

/* Default NVM size to read */
#define IWL_NVM_DEFAULT_CHUNK_SIZE (2 * 1024)

#define NVM_WRITE_OPCODE 1
#define NVM_READ_OPCODE 0

/* load nvm chunk response */
enum {
    READ_NVM_CHUNK_SUCCEED = 0,
    READ_NVM_CHUNK_NOT_VALID_ADDRESS = 1
};

struct iwl_nvm_data *IWLMvmDriver::getNvm(IWLTransport *trans,
                                          const struct iwl_fw *fw)
{
    struct iwl_nvm_get_info cmd = {};
    struct iwl_nvm_data *nvm;
    struct iwl_host_cmd hcmd = {
        .flags = CMD_WANT_SKB | CMD_SEND_IN_RFKILL,
        .data = { &cmd, },
        .len = { sizeof(cmd) },
        .id = WIDE_ID(REGULATORY_AND_NVM_GROUP, NVM_GET_INFO)
    };
    int  ret;
    bool empty_otp;
    u32 mac_flags;
    u32 sbands_flags = 0;
    /*
     * All the values in iwl_nvm_get_info_rsp v4 are the same as
     * in v3, except for the channel profile part of the
     * regulatory.  So we can just access the new struct, with the
     * exception of the latter.
     */
    struct iwl_nvm_get_info_rsp *rsp;
    struct iwl_nvm_get_info_rsp_v3 *rsp_v3;
    bool v4 = fw_has_api(&fw->ucode_capa,
                         IWL_UCODE_TLV_API_REGULATORY_NVM_INFO);
    size_t rsp_size = v4 ? sizeof(*rsp) : sizeof(*rsp_v3);
    void *channel_profile;
    
    //    ret = iwl_trans_send_cmd(trans, &hcmd);
    ret = trans->sendCmd(&hcmd);
    if (ret)
        return (iwl_nvm_data *)ERR_PTR(ret);
    
    if (iwl_rx_packet_payload_len(hcmd.resp_pkt) != rsp_size) {
        IWL_ERR(0, "Invalid payload len in NVM response from FW %d", iwl_rx_packet_payload_len(hcmd.resp_pkt));
        ret = -EINVAL;
        trans->freeResp(&hcmd);
        return (struct iwl_nvm_data *)ERR_PTR(ret);
    }
    
    rsp = (struct iwl_nvm_get_info_rsp *)hcmd.resp_pkt->data;
    empty_otp = !!(le32_to_cpu(rsp->general.flags) &
                   NVM_GENERAL_FLAGS_EMPTY_OTP);
    if (empty_otp)
        IWL_INFO(trans, "OTP is empty\n");
    
    //    nvm = (struct iwl_nvm_data *)kzalloc(struct_size(nvm, channels, IWL_NUM_CHANNELS));
    size_t nvm_size = sizeof(*nvm) + sizeof(ieee80211_channel) * IWL_NUM_CHANNELS;
    nvm = (struct iwl_nvm_data *)kzalloc(nvm_size);
    if (!nvm) {
        ret = -ENOMEM;
        goto out;
    }
    
    iwl_set_hw_address_from_csr(trans, nvm);
    /* TODO: if platform NVM has MAC address - override it here */
    
    if (!is_valid_ether_addr(nvm->hw_addr)) {
        IWL_ERR(trans, "no valid mac address was found\n");
        ret = -EINVAL;
        goto err_free;
    }
    
    IWL_INFO(trans, "base HW address: %pM\n", nvm->hw_addr);
    
    /* Initialize general data */
    nvm->nvm_version = le16_to_cpu(rsp->general.nvm_version);
    nvm->n_hw_addrs = rsp->general.n_hw_addrs;
    if (nvm->n_hw_addrs == 0)
        IWL_WARN(trans,
                 "Firmware declares no reserved mac addresses. OTP is empty: %d\n",
                 empty_otp);
    
    /* Initialize MAC sku data */
    mac_flags = le32_to_cpu(rsp->mac_sku.mac_sku_flags);
    nvm->sku_cap_11ac_enable =
    !!(mac_flags & NVM_MAC_SKU_FLAGS_802_11AC_ENABLED);
    nvm->sku_cap_11n_enable =
    !!(mac_flags & NVM_MAC_SKU_FLAGS_802_11N_ENABLED);
    nvm->sku_cap_11ax_enable =
    !!(mac_flags & NVM_MAC_SKU_FLAGS_802_11AX_ENABLED);
    nvm->sku_cap_band_24ghz_enable =
    !!(mac_flags & NVM_MAC_SKU_FLAGS_BAND_2_4_ENABLED);
    nvm->sku_cap_band_52ghz_enable =
    !!(mac_flags & NVM_MAC_SKU_FLAGS_BAND_5_2_ENABLED);
    nvm->sku_cap_mimo_disabled =
    !!(mac_flags & NVM_MAC_SKU_FLAGS_MIMO_DISABLED);
    
    /* Initialize PHY sku data */
    nvm->valid_tx_ant = (u8)le32_to_cpu(rsp->phy_sku.tx_chains);
    nvm->valid_rx_ant = (u8)le32_to_cpu(rsp->phy_sku.rx_chains);
    
    if (le32_to_cpu(rsp->regulatory.lar_enabled) &&
        fw_has_capa(&fw->ucode_capa,
                    IWL_UCODE_TLV_CAPA_LAR_SUPPORT)) {
        nvm->lar_enabled = true;
        sbands_flags |= IWL_NVM_SBANDS_FLAGS_LAR;
    }
    
    rsp_v3 = (struct iwl_nvm_get_info_rsp_v3 *)rsp;
    channel_profile = v4 ? (void *)rsp->regulatory.channel_profile :
    (void *)rsp_v3->regulatory.channel_profile;
    
    iwl_init_sbands(trans, nvm, channel_profile,
                    nvm->valid_tx_ant & fw->valid_tx_ant,
                    nvm->valid_rx_ant & fw->valid_rx_ant,
                    sbands_flags, v4);
    
    trans->freeResp(&hcmd);
    return nvm;
    
err_free:
    IOFree(nvm, nvm_size);
out:
    trans->freeResp(&hcmd);
    return (struct iwl_nvm_data *)ERR_PTR(ret);
}

/*
 * prepare the NVM host command w/ the pointers to the nvm buffer
 * and send it to fw
 */
static int iwl_nvm_write_chunk(IWLMvmDriver *mvm, u16 section,
                               u16 offset, u16 length, const u8 *data)
{
    struct iwl_nvm_access_cmd nvm_access_cmd = {
        .offset = cpu_to_le16(offset),
        .length = cpu_to_le16(length),
        .type = cpu_to_le16(section),
        .op_code = NVM_WRITE_OPCODE,
    };
    struct iwl_host_cmd cmd = {
        .id = NVM_ACCESS_CMD,
        .len = { sizeof(struct iwl_nvm_access_cmd), length },
        .flags = CMD_WANT_SKB | CMD_SEND_IN_RFKILL,
        .data = { &nvm_access_cmd, data },
        /* data may come from vmalloc, so use _DUP */
        .dataflags = { 0, IWL_HCMD_DFL_DUP },
    };
    struct iwl_rx_packet *pkt;
    struct iwl_nvm_access_resp *nvm_resp;
    int ret;
    
    ret = mvm->sendCmd(&cmd);
    if (ret)
        return ret;
    
    pkt = cmd.resp_pkt;
    /* Extract & check NVM write response */
    nvm_resp = (struct iwl_nvm_access_resp *)pkt->data;
    if (le16_to_cpu(nvm_resp->status) != READ_NVM_CHUNK_SUCCEED) {
        IWL_ERR(mvm,
                "NVM access write command failed for section %u (status = 0x%x)\n",
                section, le16_to_cpu(nvm_resp->status));
        ret = -EIO;
    }
    mvm->trans->freeResp(&cmd);
    return ret;
}

static int iwl_nvm_read_chunk(IWLMvmDriver *mvm, u16 section,
                              u16 offset, u16 length, u8 *data)
{
    struct iwl_nvm_access_cmd nvm_access_cmd = {
        .offset = cpu_to_le16(offset),
        .length = cpu_to_le16(length),
        .type = cpu_to_le16(section),
        .op_code = NVM_READ_OPCODE,
    };
    struct iwl_nvm_access_resp *nvm_resp;
    struct iwl_rx_packet *pkt;
    struct iwl_host_cmd cmd = {
        .id = NVM_ACCESS_CMD,
        .flags = CMD_WANT_SKB | CMD_SEND_IN_RFKILL,
        .data = { &nvm_access_cmd, },
    };
    int ret, bytes_read, offset_read;
    u8 *resp_data;
    
    cmd.len[0] = sizeof(struct iwl_nvm_access_cmd);
    
    ret = mvm->sendCmd(&cmd);
    if (ret)
        return ret;
    
    pkt = cmd.resp_pkt;
    
    /* Extract NVM response */
    nvm_resp = (struct iwl_nvm_access_resp *)pkt->data;
    ret = le16_to_cpu(nvm_resp->status);
    bytes_read = le16_to_cpu(nvm_resp->length);
    offset_read = le16_to_cpu(nvm_resp->offset);
    resp_data = nvm_resp->data;
    if (ret) {
        if ((offset != 0) &&
            (ret == READ_NVM_CHUNK_NOT_VALID_ADDRESS)) {
            /*
             * meaning of NOT_VALID_ADDRESS:
             * driver try to read chunk from address that is
             * multiple of 2K and got an error since addr is empty.
             * meaning of (offset != 0): driver already
             * read valid data from another chunk so this case
             * is not an error.
             */
            IWL_INFO(0,
                     "NVM access command failed on offset 0x%x since that section size is multiple 2K\n",
                     offset);
            ret = 0;
        } else {
            IWL_INFO(0,
                     "NVM access command failed with status %d (device: %s)\n",
                     ret, mvm->m_pDevice->name);
            ret = -ENODATA;
        }
        goto exit;
    }
    
    if (offset_read != offset) {
        IWL_ERR(0, "NVM ACCESS response with invalid offset %d\n",
                offset_read);
        ret = -EINVAL;
        goto exit;
    }
    
    /* Write data to NVM */
    memcpy(data + offset, resp_data, bytes_read);
    ret = bytes_read;
    
exit:
    mvm->trans->freeResp(&cmd);
    return ret;
}

static int iwl_nvm_write_section(IWLMvmDriver *mvm, u16 section,
                                 const u8 *data, u16 length)
{
    int offset = 0;
    
    /* copy data in chunks of 2k (and remainder if any) */
    
    while (offset < length) {
        int chunk_size, ret;
        
        chunk_size = min(IWL_NVM_DEFAULT_CHUNK_SIZE,
                         length - offset);
        
        ret = iwl_nvm_write_chunk(mvm, section, offset,
                                  chunk_size, data + offset);
        if (ret < 0)
            return ret;
        
        offset += chunk_size;
    }
    
    return 0;
}

/*
 * Reads an NVM section completely.
 * NICs prior to 7000 family doesn't have a real NVM, but just read
 * section 0 which is the EEPROM. Because the EEPROM reading is unlimited
 * by uCode, we need to manually check in this case that we don't
 * overflow and try to read more than the EEPROM size.
 * For 7000 family NICs, we supply the maximal size we can read, and
 * the uCode fills the response with as much data as we can,
 * without overflowing, so no check is needed.
 */
static int iwl_nvm_read_section(IWLMvmDriver *mvm, u16 section,
                                u8 *data, u32 size_read)
{
    u16 length, offset = 0;
    int ret;
    
    /* Set nvm section read length */
    length = IWL_NVM_DEFAULT_CHUNK_SIZE;
    
    ret = length;
    
    /* Read the NVM until exhausted (reading less than requested) */
    while (ret == length) {
        /* Check no memory assumptions fail and cause an overflow */
        if ((size_read + offset + length) >
            mvm->m_pDevice->cfg->trans.base_params->eeprom_size) {
            IWL_ERR(mvm, "EEPROM size is too small for NVM\n");
            return -ENOBUFS;
        }
        
        ret = iwl_nvm_read_chunk(mvm, section, offset, length, data);
        if (ret < 0) {
            IWL_INFO(mvm,
                     "Cannot read NVM from section %d offset %d, length %d\n",
                     section, offset, length);
            return ret;
        }
        offset += ret;
    }
    
    iwl_nvm_fixups(mvm->m_pDevice->hw_id, section, data, offset);
    
    IWL_INFO(mvm,
             "NVM section %d read completed\n", section);
    return offset;
}

static struct iwl_nvm_data *
iwl_parse_nvm_sections(IWLMvmDriver *mvm)
{
    struct iwl_nvm_section *sections = mvm->m_pDevice->nvm_sections;
    const __be16 *hw;
    const __le16 *sw, *calib, *regulatory, *mac_override, *phy_sku;
    int regulatory_type;
    
    /* Checking for required sections */
    if (mvm->m_pDevice->cfg->nvm_type == IWL_NVM) {
        if (!mvm->m_pDevice->nvm_sections[NVM_SECTION_TYPE_SW].data ||
            !mvm->m_pDevice->nvm_sections[mvm->m_pDevice->cfg->nvm_hw_section_num].data) {
            IWL_ERR(mvm, "Can't parse empty OTP/NVM sections\n");
            return NULL;
        }
    } else {
        if (mvm->m_pDevice->cfg->nvm_type == IWL_NVM_SDP)
            regulatory_type = NVM_SECTION_TYPE_REGULATORY_SDP;
        else
            regulatory_type = NVM_SECTION_TYPE_REGULATORY;
        
        /* SW and REGULATORY sections are mandatory */
        if (!mvm->m_pDevice->nvm_sections[NVM_SECTION_TYPE_SW].data ||
            !mvm->m_pDevice->nvm_sections[regulatory_type].data) {
            IWL_ERR(mvm,
                    "Can't parse empty family 8000 OTP/NVM sections\n");
            return NULL;
        }
        /* MAC_OVERRIDE or at least HW section must exist */
        if (!mvm->m_pDevice->nvm_sections[mvm->m_pDevice->cfg->nvm_hw_section_num].data &&
            !mvm->m_pDevice->nvm_sections[NVM_SECTION_TYPE_MAC_OVERRIDE].data) {
            IWL_ERR(mvm,
                    "Can't parse mac_address, empty sections\n");
            return NULL;
        }
        
        /* PHY_SKU section is mandatory in B0 */
        if (!mvm->m_pDevice->nvm_sections[NVM_SECTION_TYPE_PHY_SKU].data) {
            IWL_ERR(mvm,
                    "Can't parse phy_sku in B0, empty sections\n");
            return NULL;
        }
    }
    
    hw = (const __be16 *)sections[mvm->m_pDevice->cfg->nvm_hw_section_num].data;
    sw = (const __le16 *)sections[NVM_SECTION_TYPE_SW].data;
    calib = (const __le16 *)sections[NVM_SECTION_TYPE_CALIBRATION].data;
    mac_override =
    (const __le16 *)sections[NVM_SECTION_TYPE_MAC_OVERRIDE].data;
    phy_sku = (const __le16 *)sections[NVM_SECTION_TYPE_PHY_SKU].data;
    
    regulatory = mvm->m_pDevice->cfg->nvm_type == IWL_NVM_SDP ?
    (const __le16 *)sections[NVM_SECTION_TYPE_REGULATORY_SDP].data :
    (const __le16 *)sections[NVM_SECTION_TYPE_REGULATORY].data;
    
    return iwl_parse_nvm_data(mvm->trans, mvm->m_pDevice->cfg, &mvm->m_pDevice->fw, hw, sw, calib,
                              regulatory, mac_override, phy_sku,
                              mvm->m_pDevice->fw.valid_tx_ant, mvm->m_pDevice->fw.valid_rx_ant);
}

int IWLMvmDriver::nvmInit()
{
    int ret = 0, section;
    u32 size_read = 0;
    u8 *nvm_buffer, *temp;
    const char *nvm_file_C = m_pDevice->cfg->default_nvm_file_C_step;
    
    if (WARN_ON_ONCE(m_pDevice->cfg->nvm_hw_section_num >= NVM_MAX_NUM_SECTIONS))
        return -EINVAL;
    /* load NVM values from nic */
    /* Read From FW NVM */
    IWL_INFO(0, "Read from NVM\n");
    nvm_buffer = (u8 *)IOMalloc(m_pDevice->cfg->trans.base_params->eeprom_size);
    if (!nvm_buffer)
        return -ENOMEM;
    for (section = 0; section < NVM_MAX_NUM_SECTIONS; section++) {
        /* we override the constness for initial read */
        IWL_INFO(0, "Parsing section %d\n", section);
        ret = iwl_nvm_read_section(this, section, nvm_buffer,
                                   size_read);
        IWL_INFO(0, "ret: %d\n", ret);
        if (ret == -ENODATA) {
            ret = 0;
            IWL_ERR(0, "Skipped section %d because -ENODATA\n", section);
            continue;
        }
        if (ret < 0)
            break;
        size_read += ret;
        temp = (u8 *)kmemdup(nvm_buffer, ret);
        if (!temp) {
            ret = -ENOMEM;
            IWL_ERR(0, "Could not duplicate memory from NVM\n");
            break;
        }
        IWL_INFO(0, "Parsed section %d successfully\n", section);
        iwl_nvm_fixups(m_pDevice->hw_id, section, temp, ret);
        
        m_pDevice->nvm_sections[section].data = temp;
        m_pDevice->nvm_sections[section].length = ret;
    }
    if (!size_read)
        IWL_ERR(mvm, "OTP is blank\n");
    IOFree(nvm_buffer, m_pDevice->cfg->trans.base_params->eeprom_size);
    //    /* Only if PNVM selected in the mod param - load external NVM  */
    //    if (mvm->nvm_file_name) {
    //        /* read External NVM file from the mod param */
    //        ret = iwl_read_external_nvm(mvm->trans, mvm->nvm_file_name,
    //                                    mvm->nvm_sections);
    //        if (ret) {
    //            mvm->nvm_file_name = nvm_file_C;
    //
    //            if ((ret == -EFAULT || ret == -ENOENT) &&
    //                mvm->nvm_file_name) {
    //                /* in case nvm file was failed try again */
    //                ret = iwl_read_external_nvm(mvm->trans,
    //                                            mvm->nvm_file_name,
    //                                            mvm->nvm_sections);
    //                if (ret)
    //                    return ret;
    //            } else {
    //                return ret;
    //            }
    //        }
    //    }
    
    /* parse the relevant nvm sections */
    m_pDevice->nvm_data = iwl_parse_nvm_sections(this);
    if (!m_pDevice->nvm_data)
        return -ENODATA;
    IWL_INFO(0, "nvm version = %x\n",
             m_pDevice->nvm_data->nvm_version);
    
    u8* hw_addr = m_pDevice->nvm_data->hw_addr;
    if(hw_addr) {
        // ideally, if we're successful then this should pass and we should get the MAC from our card
        IWL_INFO(0, "addr: %02x:%02x:%02x:%02x:%02x:%02x\n", hw_addr[0],
        hw_addr[1], hw_addr[2],
                 hw_addr[3], hw_addr[4], hw_addr[5] );
    }
    
    return ret < 0 ? ret : 0;
}

struct iwl_mcc_update_resp *IWLMvmDriver::updateMcc(const char *alpha2, enum iwl_mcc_source src_id)
{
    struct iwl_mcc_update_cmd mcc_update_cmd = {
        .mcc = cpu_to_le16(alpha2[0] << 8 | alpha2[1]),
        .source_id = (u8)src_id,
    };
    struct iwl_mcc_update_resp *resp_cp;
    struct iwl_rx_packet *pkt;
    struct iwl_host_cmd cmd = {
        .id = MCC_UPDATE_CMD,
        .flags = CMD_WANT_SKB,
        .data = { &mcc_update_cmd },
    };
    
    int ret;
    u32 status;
    int resp_len, n_channels;
    u16 mcc;
    
    if (WARN_ON_ONCE(!iwl_mvm_is_lar_supported(m_pDevice)))
        return (struct iwl_mcc_update_resp *)ERR_PTR(-EOPNOTSUPP);
    cmd.len[0] = sizeof(struct iwl_mcc_update_cmd);
    cmd.resp_pkt_len = sizeof(struct iwl_rx_packet) + sizeof(struct iwl_mcc_update_cmd);
    
    IWL_INFO(0, "send MCC update to FW with '%c%c' src = %d\n",
             alpha2[0], alpha2[1], src_id);
    
    ret = sendCmd(&cmd);
    if (ret)
        return (struct iwl_mcc_update_resp *)ERR_PTR(ret);
    
    pkt = cmd.resp_pkt;
    
    /* Extract MCC response */
    if (fw_has_capa(&m_pDevice->fw.ucode_capa,
                    IWL_UCODE_TLV_CAPA_MCC_UPDATE_11AX_SUPPORT)) {
        struct iwl_mcc_update_resp *mcc_resp = (struct iwl_mcc_update_resp *)pkt->data;
        
        n_channels =  __le32_to_cpu(mcc_resp->n_channels);
        resp_len = sizeof(struct iwl_mcc_update_resp) +
        n_channels * sizeof(__le32);
        resp_cp = (struct iwl_mcc_update_resp*)kmemdup(mcc_resp, resp_len);
        if (!resp_cp) {
            resp_cp = (struct iwl_mcc_update_resp *)ERR_PTR(-ENOMEM);
            goto exit;
        }
    } else {
        struct iwl_mcc_update_resp_v3 *mcc_resp_v3 = (struct iwl_mcc_update_resp_v3 *)pkt->data;
        
        n_channels =  __le32_to_cpu(mcc_resp_v3->n_channels);
        resp_len = sizeof(struct iwl_mcc_update_resp) +
        n_channels * sizeof(__le32);
        resp_cp = (struct iwl_mcc_update_resp *)kzalloc(resp_len);
        if (!resp_cp) {
            resp_cp = (struct iwl_mcc_update_resp *)ERR_PTR(-ENOMEM);
            goto exit;
        }
        
        resp_cp->status = mcc_resp_v3->status;
        resp_cp->mcc = mcc_resp_v3->mcc;
        resp_cp->cap = cpu_to_le16(mcc_resp_v3->cap);
        resp_cp->source_id = mcc_resp_v3->source_id;
        resp_cp->time = mcc_resp_v3->time;
        resp_cp->geo_info = mcc_resp_v3->geo_info;
        resp_cp->n_channels = mcc_resp_v3->n_channels;
        memcpy(resp_cp->channels, mcc_resp_v3->channels,
               n_channels * sizeof(__le32));
    }
    
    status = le32_to_cpu(resp_cp->status);
    
    mcc = le16_to_cpu(resp_cp->mcc);
    
    /* W/A for a FW/NVM issue - returns 0x00 for the world domain */
    if (mcc == 0) {
        mcc = 0x3030;  /* "00" - world */
        resp_cp->mcc = cpu_to_le16(mcc);
    }
    
    IWL_INFO(0,
             "MCC response status: 0x%x. new MCC: 0x%x ('%c%c') n_chans: %d\n",
             status, mcc, mcc >> 8, mcc & 0xff, n_channels);
exit:
    trans->freeResp(&cmd);
    return resp_cp;
}
