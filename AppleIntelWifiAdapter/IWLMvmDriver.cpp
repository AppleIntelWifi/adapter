//
//  IWLMvmDriver.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/17.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmDriver.hpp"
#include "IWLUcodeParse.hpp"
#include "IWLMvmTransOpsGen1.hpp"
#include "IWLMvmTransOpsGen2.hpp"
#include "MvmCmd.hpp"
#include "txq.h"

bool IWLMvmDriver::init(IOPCIDevice *pciDevice)
{
    this->fwLoadLock = IOLockAlloc();
    this->m_pDevice = new IWLDevice();
    if (!this->m_pDevice->init(pciDevice)) {
        return false;
    }
    IWLTransport *trans = new IWLTransport();
    if (!trans->init(m_pDevice)) {
        return false;
    }
    if (m_pDevice->cfg->trans.gen2) {
        this->trans_ops = new IWLMvmTransOpsGen2(trans);
    } else {
        this->trans_ops = new IWLMvmTransOpsGen1(trans);
    }
    return true;
}

void IWLMvmDriver::release()
{
    if (this->fwLoadLock) {
        IOLockFree(this->fwLoadLock);
        this->fwLoadLock = NULL;
    }
    if (trans_ops) {
        trans_ops->release();
        delete trans_ops;
        trans_ops = NULL;
    }
    if (m_pDevice) {
        m_pDevice->release();
        delete m_pDevice;
        m_pDevice = NULL;
    }
}

bool IWLMvmDriver::probe()
{
    IOInterruptState flags;
    const struct iwl_cfg *cfg_7265d = NULL;
    UInt16 vendorID = m_pDevice->pciDevice->configRead16(kIOPCIConfigVendorID);
    if (vendorID != PCI_VENDOR_ID_INTEL) {
        return false;
    }
    UInt16 deviceID = m_pDevice->deviceID;
    UInt16 subSystemDeviceID = m_pDevice->subSystemDeviceID;
    
    IOLog("hw_rf_id=0x%02x\n", m_pDevice->hw_rf_id);
    IOLog("hw_rev=0x%02x\n", m_pDevice->hw_rev);
    
    for (int i = 0; i < ARRAY_SIZE(iwl_dev_info_table); i++) {
        const struct iwl_dev_info *dev_info = &iwl_dev_info_table[i];
        if ((dev_info->device == (u16)IWL_CFG_ANY ||
             dev_info->device == deviceID) &&
            (dev_info->subdevice == (u16)IWL_CFG_ANY ||
             dev_info->subdevice == subSystemDeviceID) &&
            (dev_info->mac_type == (u16)IWL_CFG_ANY ||
             dev_info->mac_type ==
             CSR_HW_REV_TYPE(m_pDevice->hw_rev)) &&
            (dev_info->mac_step == (u8)IWL_CFG_ANY ||
             dev_info->mac_step ==
             CSR_HW_REV_STEP(m_pDevice->hw_rev)) &&
            (dev_info->rf_type == (u16)IWL_CFG_ANY ||
             dev_info->rf_type ==
             CSR_HW_RFID_TYPE(m_pDevice->hw_rf_id)) &&
            (dev_info->rf_id == (u8)IWL_CFG_ANY ||
             dev_info->rf_id ==
             IWL_SUBDEVICE_RF_ID(subSystemDeviceID)) &&
            (dev_info->no_160 == (u8)IWL_CFG_ANY ||
             dev_info->no_160 ==
             IWL_SUBDEVICE_NO_160(subSystemDeviceID)) &&
            (dev_info->cores == (u8)IWL_CFG_ANY ||
             dev_info->cores ==
             IWL_SUBDEVICE_CORES(subSystemDeviceID))) {
            m_pDevice->cfg = dev_info->cfg;
            m_pDevice->name = dev_info->name;
        }
    }
    
    if (m_pDevice->cfg) {
        /*
         * special-case 7265D, it has the same PCI IDs.
         *
         * Note that because we already pass the cfg to the transport above,
         * all the parameters that the transport uses must, until that is
         * changed, be identical to the ones in the 7265D configuration.
         */
        if (m_pDevice->cfg == &iwl7265_2ac_cfg)
            cfg_7265d = &iwl7265d_2ac_cfg;
        else if (m_pDevice->cfg == &iwl7265_2n_cfg)
            cfg_7265d = &iwl7265d_2n_cfg;
        else if (m_pDevice->cfg == &iwl7265_n_cfg)
            cfg_7265d = &iwl7265d_n_cfg;
        if (cfg_7265d &&
            (m_pDevice->hw_rev & CSR_HW_REV_TYPE_MSK) == CSR_HW_REV_TYPE_7265D)
            m_pDevice->cfg = cfg_7265d;
        
        if (m_pDevice->cfg == &iwlax210_2ax_cfg_so_hr_a0) {
            if (m_pDevice->hw_rev == CSR_HW_REV_TYPE_TY) {
                m_pDevice->cfg = &iwlax210_2ax_cfg_ty_gf_a0;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_JF)) {
                m_pDevice->cfg = &iwlax210_2ax_cfg_so_jf_a0;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_GF)) {
                m_pDevice->cfg = &iwlax211_2ax_cfg_so_gf_a0;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_GF4)) {
                m_pDevice->cfg = &iwlax411_2ax_cfg_so_gf4_a0;
            }
        } else if (m_pDevice->cfg == &iwl_ax101_cfg_qu_hr) {
            if ((CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id) ==
                 CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR) &&
                 m_pDevice->hw_rev == CSR_HW_REV_TYPE_QNJ_B0) ||
                (CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id) ==
                 CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR1))) {
                m_pDevice->cfg = &iwl22000_2ax_cfg_qnj_hr_b0;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR) &&
                       m_pDevice->hw_rev == CSR_HW_REV_TYPE_QUZ) {
                m_pDevice->cfg = &iwl_ax101_cfg_quz_hr;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR)) {
                m_pDevice->cfg = &iwl_ax101_cfg_qu_hr;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HRCDB)) {
                IWL_ERR(0, "RF ID HRCDB is not supported\n");
                goto error;
            } else {
                IWL_ERR(0, "Unrecognized RF ID 0x%08x\n",
                        CSR_HW_RF_ID_TYPE_CHIP_ID(m_pDevice->hw_rf_id));
                goto error;
            }
        }
        
        /*
         * The RF_ID is set to zero in blank OTP so read version to
         * extract the RF_ID.
         */
        if (m_pDevice->cfg->trans.rf_id &&
            !CSR_HW_RFID_TYPE(m_pDevice->hw_rf_id)) {
            IOInterruptState flags;
            
            if (trans_ops->trans->grabNICAccess(&flags)) {
                u32 val;
                
                val = trans_ops->trans->iwlReadUmacPRPHNoGrab(WFPM_CTRL_REG);
                val |= ENABLE_WFPM;
                trans_ops->trans->iwlWriteUmacPRPHNoGrab(WFPM_CTRL_REG, val);
                val = trans_ops->trans->iwlReadPRPHNoGrab(SD_REG_VER);
                
                val &= 0xff00;
                switch (val) {
                    case REG_VER_RF_ID_JF:
                        m_pDevice->hw_rf_id = CSR_HW_RF_ID_TYPE_JF;
                        break;
                        /* TODO: get value for REG_VER_RF_ID_HR */
                    default:
                        m_pDevice->hw_rf_id = CSR_HW_RF_ID_TYPE_HR;
                }
                trans_ops->trans->releaseNICAccess(&flags);
            }
        }
        
        /*
         * This is a hack to switch from Qu B0 to Qu C0.  We need to
         * do this for all cfgs that use Qu B0, except for those using
         * Jf, which have already been moved to the new table.  The
         * rest must be removed once we convert Qu with Hr as well.
         */
        if (m_pDevice->hw_rev == CSR_HW_REV_TYPE_QU_C0) {
            if (m_pDevice->cfg == &iwl_ax101_cfg_qu_hr)
                m_pDevice->cfg = &iwl_ax101_cfg_qu_c0_hr_b0;
            else if (m_pDevice->cfg == &iwl_ax201_cfg_qu_hr)
                m_pDevice->cfg = &iwl_ax201_cfg_qu_c0_hr_b0;
            else if (m_pDevice->cfg == &killer1650s_2ax_cfg_qu_b0_hr_b0)
                m_pDevice->cfg = &killer1650s_2ax_cfg_qu_c0_hr_b0;
            else if (m_pDevice->cfg == &killer1650i_2ax_cfg_qu_b0_hr_b0)
                m_pDevice->cfg = &killer1650i_2ax_cfg_qu_c0_hr_b0;
        }
        
        /* same thing for QuZ... */
        if (m_pDevice->hw_rev == CSR_HW_REV_TYPE_QUZ) {
            if (m_pDevice->cfg == &iwl_ax101_cfg_qu_hr)
                m_pDevice->cfg = &iwl_ax101_cfg_quz_hr;
            else if (m_pDevice->cfg == &iwl_ax201_cfg_qu_hr)
                m_pDevice->cfg = &iwl_ax201_cfg_quz_hr;
        }
        
        /* if we don't have a name yet, copy name from the old cfg */
        if (!m_pDevice->name)
            m_pDevice->name = m_pDevice->cfg->name;
        
        
        IOLog("device name=%s\n", m_pDevice->name);
        
        //        if (m_pDevice->trans_cfg->mq_rx_supported) {
        //            if (!m_pDevice->cfg->num_rbds) {
        //                goto error;
        //            }
        //            trans_pcie->num_rx_bufs = iwl_trans_ops->trans->cfg->num_rbds;
        //        } else {
        //            trans_pcie->num_rx_bufs = RX_QUEUE_SIZE;
        //        }
        
        if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_8000 &&
            trans_ops->trans->grabNICAccess(&flags)) {
            u32 hw_step;
            
            IOLog("HW_STEP_LOCATION_BITS\n");
            
            hw_step = trans_ops->trans->iwlReadUmacPRPHNoGrab(WFPM_CTRL_REG);
            hw_step |= ENABLE_WFPM;
            trans_ops->trans->iwlWriteUmacPRPHNoGrab(WFPM_CTRL_REG, hw_step);
            hw_step = trans_ops->trans->iwlReadPRPHNoGrab(CNVI_AUX_MISC_CHIP);
            hw_step = (hw_step >> HW_STEP_LOCATION_BITS) & 0xF;
            if (hw_step == 0x3)
                m_pDevice->hw_rev = (m_pDevice->hw_rev & 0xFFFFFFF3) |
                (SILICON_C_STEP << 2);
            trans_ops->trans->releaseNICAccess(&flags);
        }
    }
    return true;
error:
    return false;
}

bool IWLMvmDriver::start()
{
    char tag[8];
    char firmware_name[64];
    if (m_pDevice->cfg->trans.device_family == IWL_DEVICE_FAMILY_9000 &&
        (CSR_HW_REV_STEP(m_pDevice->hw_rev) != SILICON_B_STEP &&
         CSR_HW_REV_STEP(m_pDevice->hw_rev) != SILICON_C_STEP)) {
        IWL_ERR(0,
                "Only HW steps B and C are currently supported (0x%0x)\n",
                m_pDevice->hw_rev);
        return false;
    }
    snprintf(tag, sizeof(tag), "%d", m_pDevice->cfg->ucode_api_max);
    snprintf(firmware_name, sizeof(firmware_name), "%s%s.ucode", m_pDevice->cfg->fw_name_pre, tag);
    IWL_INFO(0, "attempting to load firmware '%s'\n", firmware_name);
    IOLockLock(fwLoadLock);
    IOReturn ret = OSKextRequestResource(OSKextGetCurrentIdentifier(), firmware_name, reqFWCallback, this, NULL);
    if (ret != kIOReturnSuccess) {
        IWL_ERR(0, "Error loading firmware %s\n", firmware_name);
        return false;
    }
    IOLockSleep(fwLoadLock, this, 0);
    IOLockUnlock(fwLoadLock);
    return m_pDevice->firmwareLoadToBuf ? this->drvStart() : false;
}

#define IWL_DEFAULT_SCAN_CHANNELS 40

void IWLMvmDriver::reqFWCallback(OSKextRequestTag requestTag, OSReturn result, const void *resourceData, uint32_t resourceDataLength, void *context)
{
    IWLMvmDriver *that = (IWLMvmDriver*)context;
    bool ret;
    if (resourceDataLength <= 4) {
        IWL_ERR(0, "Error loading fw, size=%d\n",resourceDataLength);
        IOLockWakeup(that->fwLoadLock, that, true);
        return;
    }
    that->m_pDevice->usniffer_images = false;
    iwl_fw *fw = &that->m_pDevice->fw;
    fw->ucode_capa.max_probe_length = IWL_DEFAULT_MAX_PROBE_LENGTH;
    fw->ucode_capa.standard_phy_calibration_size =
    IWL_DEFAULT_STANDARD_PHY_CALIBRATE_TBL_SIZE;
    fw->ucode_capa.n_scan_channels = IWL_DEFAULT_SCAN_CHANNELS;
    /* dump all fw memory areas by default */
    fw->dbg.dump_mask = 0xffffffff;
    IWL_INFO(drv, "Loaded firmware file '%s' (%zd bytes).\n",
             that->m_pDevice->name, resourceDataLength);
    
    IWLUcodeParse parser(that->m_pDevice);
    ret = parser.parseFW(resourceData, resourceDataLength, &that->m_pDevice->fw, &that->m_pDevice->pieces);
    if (!ret) {
        IWL_ERR(0, "parse fw failed\n");
        IOLockWakeup(that->fwLoadLock, that, true);
        return;
    }

    iwl_firmware_pieces *pieces = &that->m_pDevice->pieces;

    /* Allocate ucode buffers for card's bus-master loading ... */

    /* Runtime instructions and 2 copies of data:
     * 1) unmodified from disk
     * 2) backup cache for save/restore during power-downs
     */
    bool isAllocErr = false;
    for (int i = 0; i < IWL_UCODE_TYPE_MAX; i++) {
        if (parser.allocUcode(pieces, (enum iwl_ucode_type)i)) {
            isAllocErr = true;
            break;
        }
    }

    if (isAllocErr) {
        IOLockWakeup(that->fwLoadLock, that, true);
        return;
    }

    /*
     * The (size - 16) / 12 formula is based on the information recorded
     * for each event, which is of mode 1 (including timestamp) for all
     * new microcodes that include this information.
     */
    fw->init_evtlog_ptr = pieces->init_evtlog_ptr;
    if (pieces->init_evtlog_size)
        fw->init_evtlog_size = (pieces->init_evtlog_size - 16)/12;
    else
        fw->init_evtlog_size =
        that->m_pDevice->cfg->trans.base_params->max_event_log_size;
    fw->init_errlog_ptr = pieces->init_errlog_ptr;
    fw->inst_evtlog_ptr = pieces->inst_evtlog_ptr;
    if (pieces->inst_evtlog_size)
        fw->inst_evtlog_size = (pieces->inst_evtlog_size - 16)/12;
    else
        fw->inst_evtlog_size =
        that->m_pDevice->cfg->trans.base_params->max_event_log_size;
    fw->inst_errlog_ptr = pieces->inst_errlog_ptr;
    /*
     * figure out the offset of chain noise reset and gain commands
     * base on the size of standard phy calibration commands table size
     */
    if (fw->ucode_capa.standard_phy_calibration_size >
        IWL_MAX_PHY_CALIBRATE_TBL_SIZE)
        fw->ucode_capa.standard_phy_calibration_size =
        IWL_MAX_STANDARD_PHY_CALIBRATE_TBL_SIZE;
    
    that->m_pDevice->firmwareLoadToBuf = true;
    
    IOLockWakeup(that->fwLoadLock, that, true);
}

bool IWLMvmDriver::drvStart()
{
    //TODO
//    /********************************
//     * 1. Allocating and configuring HW data
//     ********************************/
//    hw = ieee80211_alloc_hw(sizeof(struct iwl_op_mode) +
//                            sizeof(struct iwl_mvm),
//                            &iwl_mvm_hw_ops);
//    if (!hw)
//        return NULL;
    
    if (iwl_mvm_has_new_rx_api(&this->m_pDevice->fw)) {
        
    } else {
        
    }
    IWLTransport *trans = trans_ops->trans;
    trans->wide_cmd_header = true;
    trans->command_groups = iwl_mvm_groups;
    trans->command_groups_size = ARRAY_SIZE(iwl_mvm_groups);
    trans->cmd_queue = IWL_MVM_DQA_CMD_QUEUE;
    trans->cmd_fifo = IWL_MVM_TX_FIFO_CMD;
    return true;
}

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

