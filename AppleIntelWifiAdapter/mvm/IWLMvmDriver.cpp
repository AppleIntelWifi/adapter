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
#include "../fw/NotificationWait.hpp"
#include "IWLMvmSmartFifo.hpp"
#include "IWLMvmSta.hpp"
#include "IWLMvmPhy.hpp"

#define super OSObject

bool IWLMvmDriver::init(IOPCIDevice *pciDevice)
{
    super::init();
    this->fwLoadLock = IOLockAlloc();
    this->m_pDevice = new IWLDevice();
    this->m_pDevice->controller = controller;
    if (!this->m_pDevice->init(pciDevice)) {
        return false;
    }
    this->trans = new IWLTransport();
    if (!this->trans->init(m_pDevice)) {
        IWL_INFO(0, "failed to init trans\n");
        return false;
    }
    if (m_pDevice->cfg->trans.gen2) {
        IWL_INFO(0, "gen2 device\n");
        this->trans_ops = new IWLMvmTransOpsGen2(this->trans);
    } else {
        IWL_INFO(0, "gen1 device\n");
        this->trans_ops = new IWLMvmTransOpsGen1(this->trans);
    }
    return true;
}

void IWLMvmDriver::release()
{
    ieee80211Release();
    if (this->fwLoadLock) {
        IOLockFree(this->fwLoadLock);
        this->fwLoadLock = NULL;
    }
    if (this->trans_ops) {
        delete this->trans_ops;
        this->trans_ops = NULL;
    }
    if (this->trans) {
        this->trans->release();
        delete this->trans;
        this->trans = NULL;
    }
    if (this->m_pDevice) {
        this->m_pDevice->release();
        delete this->m_pDevice;
        this->m_pDevice = NULL;
    }
    super::free();
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
        
        
        IWL_INFO(0, "device name=%s\n", m_pDevice->name);
        
        if (m_pDevice->cfg->trans.mq_rx_supported) {
            if (!m_pDevice->cfg->num_rbds) {
                goto error;
            }
            trans->num_rx_bufs = m_pDevice->cfg->num_rbds;
        } else {
            trans->num_rx_bufs = RX_QUEUE_SIZE;
        }
        
        if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_8000 &&
            trans_ops->trans->grabNICAccess(&flags)) {
            u32 hw_step;
            
            IWL_INFO(0, "HW_STEP_LOCATION_BITS\n");
            
            hw_step = trans_ops->trans->iwlReadUmacPRPHNoGrab(WFPM_CTRL_REG);
            hw_step |= ENABLE_WFPM;
            trans_ops->trans->iwlWriteUmacPRPHNoGrab(WFPM_CTRL_REG, hw_step);
            hw_step = trans_ops->trans->iwlReadPRPHNoGrab(CNVI_AUX_MISC_CHIP);
            IWL_INFO(0, "AUX CHIP: %x", hw_step);
            hw_step = (hw_step >> HW_STEP_LOCATION_BITS) & 0xF;
            IWL_INFO(0, "We actually got: %x", hw_step);
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
    IWL_INFO(0, "HW Rev: %0x\n", m_pDevice->hw_rev);
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
    return true;
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
    int err;

    enum iwl_amsdu_size rb_size_default;
    //TODO
    //    /********************************
    //     * 1. Allocating and configuring HW data
    //     ********************************/
    //    hw = ieee80211_alloc_hw(sizeof(struct iwl_op_mode) +
    //                            sizeof(struct iwl_mvm),
    //                            &iwl_mvm_hw_ops);
    //    if (!hw)
    //        return NULL;
    IWL_INFO(0, "driver start\n");
    if (!ieee80211Init()) {
        IWL_ERR(0, "ieee80211 init fail.\n");
        return false;
    }
    
    this->trans->scd_set_active = true;
    
    if (iwl_mvm_has_new_rx_api(&this->m_pDevice->fw)) {
        
    } else {
        
    }
    
    m_pDevice->btForceAntMode = BT_FORCE_ANT_WIFI;
    if (iwl_mvm_has_unified_ucode(m_pDevice)) {
        m_pDevice->cur_fw_img = IWL_UCODE_REGULAR;
    } else {
        m_pDevice->cur_fw_img = IWL_UCODE_INIT;
    }
    if (trans->m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_AX210)
        rb_size_default = IWL_AMSDU_2K;
    else
        rb_size_default = IWL_AMSDU_4K;
    switch (m_pDevice->iwlwifi_mod_params.amsdu_size) {
        case IWL_AMSDU_DEF:
            trans->rx_buf_size = rb_size_default;
            break;
        case IWL_AMSDU_4K:
            trans->rx_buf_size = IWL_AMSDU_4K;
            break;
        case IWL_AMSDU_8K:
            trans->rx_buf_size = IWL_AMSDU_8K;
            break;
        case IWL_AMSDU_12K:
            trans->rx_buf_size = IWL_AMSDU_12K;
            break;
        default:
            IWL_INFO(0, "Unsupported amsdu_size: %d\n", m_pDevice->iwlwifi_mod_params.amsdu_size);
            trans->rx_buf_size = rb_size_default;
    }
    IWLTransport *trans = trans_ops->trans;
    trans->wide_cmd_header = true;
    trans->command_groups = iwl_mvm_groups;
    trans->command_groups_size = ARRAY_SIZE(iwl_mvm_groups);
    trans->cmd_queue = IWL_MVM_DQA_CMD_QUEUE;
    trans->cmd_fifo = IWL_MVM_TX_FIFO_CMD;
    iwl_notification_wait_init(&m_pDevice->notif_wait);
    iwl_phy_db_init(trans, &this->phy_db);
    err = trans_ops->startHW();
    err = runInitMvmUCode(true);
    if (err && err != -ERFKILL) {
        IWL_ERR(0, "init mvm ucode fail\n");
    }
//    struct iwl_led_cmd led_cmd = {
//        .status = cpu_to_le32(true),
//    };
//    struct iwl_host_cmd cmd = {
//        .id = WIDE_ID(LONG_GROUP, LEDS_CMD),
//        .len = { sizeof(led_cmd), },
//        .data = { &led_cmd, },
//        .flags = CMD_ASYNC,
//    };
//    sendCmd(&cmd);
    
    
    if (!err) {
        stopDevice();
    } else if (err < 0) {
        IWL_ERR(0, "Failed to run INIT ucode: %d\n", err);
        goto fail;
    }
    
    //iwl_phy_db_free(&this->phy_db);
     

    
    return true;
    
fail:
    return false;
    
}

bool IWLMvmDriver::enableDevice() {
    int err;
    
    iwl_phy_db_init(trans, &this->phy_db);
    err = trans_ops->startHW();
    m_pDevice->cur_fw_img = IWL_UCODE_INIT;
    err = runInitMvmUCode(false);
    // now we run the proper ucode
    if(err)
        goto fail;
    
    stopDevice();
    
    
    err = trans_ops->startHW();
    
    m_pDevice->cur_fw_img = IWL_UCODE_REGULAR;
    err = loadUcodeWaitAlive(IWL_UCODE_REGULAR);
    if(err < 0) {
        IWL_ERR(0, "Failed to run REGULAR ucode: %d\n", err);
        goto fail;
    }

    /*

    */
    
    err = iwl_sf_config(this, SF_INIT_OFF);
    if(err < 0) {
        IWL_ERR(0, "Smart FIFO failed to activate: %d\n", err);
        goto fail;
    }

    err = sendTXAntCfg(iwl_mvm_get_valid_tx_ant(m_pDevice));
    if(err < 0) {
        IWL_ERR(0, "Failed to send TX ant: %d\n", err);
        goto fail;
    }
    
    if(!iwl_mvm_has_unified_ucode(m_pDevice)) {
        err = iwl_send_phy_db_data(&phy_db);
        if(err < 0) {
            IWL_ERR(0, "Failed to send phy db: %d\n", err);
            goto fail;
        }
    }
    
    IOSleep(100);
    
    err = sendPhyCfgCmd();
    if (err < 0) {
        IWL_ERR(0, "Failed to send phy cfg cmd: %d\n", err);
        goto fail;
    }
    
    
    err = sendBTInitConf();
    if(err < 0) {
        IWL_ERR(0, "Failed to activate BT coex: %d\n", err);
        goto fail;
    }
    
    err = iwl_mvm_add_aux_sta(this);
    if(err < 0) {
        IWL_ERR(0, "Failed to add aux station: %d\n", err);
        goto fail;
    }
    
    ieee80211Run();
    
    for(int i = 0; i < NUM_PHY_CTX; i++)
    {
        if ((err = iwl_phy_ctxt_add(this,
            &this->m_pDevice->phy_ctx[i], &m_pDevice->ie_ic.ic_channels[1], 1, 1)) != 0)
            goto fail;
    }
    
fail:
    return false;
}


void IWLMvmDriver::stopDevice()
{
    clear_bit(IWL_MVM_STATUS_FIRMWARE_RUNNING, &trans->m_pDevice->status);
    trans_ops->stopDevice();
    //TODO xvt fw paging mode
    //    iwl_free_fw_paging(&mvm->fwrt);
}

int IWLMvmDriver::irqHandler(int irq, void *dev_id)
{
    isr_statistics *isr_stats = &trans->isr_stats;
//        u32 inta;
   u32 handled = 0;
//        /* dram interrupt table not set yet,
//         * use legacy interrupt.
//         */
//        if (likely(trans_ops->trans->use_ict))
//            inta = trans_ops->trans->intrCauseICT();
//        else
//            inta = trans_ops->trans->intrCauseNonICT();
//
    u32 inta = trans->iwlRead32(CSR_INT);
    inta &= trans_ops->trans->inta_mask;
    /*
     * Ignore interrupt if there's nothing in NIC to service.
     * This may be due to IRQ shared with another device,
     * or due to sporadic interrupts thrown from our NIC.
     */
    if (unlikely(!inta)) {
        IWL_INFO(0, "Ignore interrupt, inta == 0\n");
        /*
         * Re-enable interrupts here since we don't
         * have anything to service
         */
        if (test_bit(STATUS_INT_ENABLED, &trans_ops->trans->status))
            trans->enableIntrDirectly();
        return -1;
    }
    
         if (unlikely(inta == 0xFFFFFFFF || (inta & 0xFFFFFFF0) == 0xa5a5a5a0)) {
             /*
              * Hardware disappeared. It might have
              * already raised an interrupt.
              */
             IWL_WARN(trans, "HARDWARE GONE?? INTA == 0x%08x\n", inta);
             goto out;
         }
         /* Ack/clear/reset pending uCode interrupts.
          * Note:  Some bits in CSR_INT are "OR" of bits in CSR_FH_INT_STATUS,
          */
         /* There is a hardware bug in the interrupt mask function that some
          * interrupts (i.e. CSR_INT_BIT_SCD) can still be generated even if
          * they are disabled in the CSR_INT_MASK register. Furthermore the
          * ICT interrupt handling mechanism has another bug that might cause
          * these unmasked interrupts fail to be detected. We workaround the
          * hardware bugs here by ACKing all the possible interrupts so that
          * interrupt coalescing can still be achieved.
          */
         trans->iwlWrite32(CSR_INT, inta | ~trans->inta_mask);
         /* Now service all interrupt bits discovered above. */
         if (inta & CSR_INT_BIT_HW_ERR) {
             IWL_ERR(trans, "Hardware error detected.  Restarting.\n");
             /* Tell the device to stop sending interrupts */
             trans->disableIntr();
             isr_stats->hw++;
             trans->irqHandleError();
             handled |= CSR_INT_BIT_HW_ERR;
             goto out;
         }
         /* NIC fires this, but we don't use it, redundant with WAKEUP */
         if (inta & CSR_INT_BIT_SCD) {
             IWL_INFO(trans,
                      "Scheduler finished to transmit the frame/frames.\n");
             isr_stats->sch++;
         }
         /* Alive notification via Rx interrupt will do the real work */
         if (inta & CSR_INT_BIT_ALIVE) {
             IWL_INFO(trans, "Alive interrupt\n");
             isr_stats->alive++;
             if (trans->m_pDevice->cfg->trans.gen2) {
                 /*
                  * We can restock, since firmware configured
                  * the RFH
                  */
                 trans->rxMqRestock(trans->rxq);
             }
             //IOLockWakeup(trans->ucode_write_waitq, &this->alive_wait, true);
             handled |= CSR_INT_BIT_ALIVE;
         }
         /* Safely ignore these bits for debug checks below */
         inta &= ~(CSR_INT_BIT_SCD | CSR_INT_BIT_ALIVE);
         /* HW RF KILL switch toggled */
         if (inta & CSR_INT_BIT_RF_KILL) {
             
             trans_ops->irqRfKillHandle();
             handled |= CSR_INT_BIT_RF_KILL;
         }
     
         /* Chip got too hot and stopped itself */
         if (inta & CSR_INT_BIT_CT_KILL) {
             IWL_ERR(trans, "Microcode CT kill error detected.\n");
             isr_stats->ctkill++;
             handled |= CSR_INT_BIT_CT_KILL;
         }
     
         /* Error detected by uCode */
         if (inta & CSR_INT_BIT_SW_ERR) {
             IWL_ERR(trans, "Microcode SW error detected. "
                     " Restarting 0x%X.\n", inta);
             isr_stats->sw++;
             trans->irqHandleError();
             handled |= CSR_INT_BIT_SW_ERR;
         }
     
         /* uCode wakes up after power-down sleep */
         if (inta & CSR_INT_BIT_WAKEUP) {
             IWL_INFO(trans, "Wakeup interrupt\n");
             trans->rxqCheckWrPtr();
             trans->txqCheckWrPtrs();
             
             isr_stats->wakeup++;
     
             handled |= CSR_INT_BIT_WAKEUP;
         }
     
         /* All uCode command responses, including Tx command responses,
          * Rx "responses" (frame-received notification), and other
          * notifications from uCode come through here*/
         if (inta & (CSR_INT_BIT_FH_RX | CSR_INT_BIT_SW_RX |
                     CSR_INT_BIT_RX_PERIODIC)) {
             IWL_INFO(trans, "Rx interrupt\n");
             if (inta & (CSR_INT_BIT_FH_RX | CSR_INT_BIT_SW_RX)) {
                 handled |= (CSR_INT_BIT_FH_RX | CSR_INT_BIT_SW_RX);
                 trans->iwlWrite32(CSR_FH_INT_STATUS,
                                   CSR_FH_INT_RX_MASK);
             }
             if (inta & CSR_INT_BIT_RX_PERIODIC) {
                 handled |= CSR_INT_BIT_RX_PERIODIC;
                 trans->iwlWrite32(
                                   CSR_INT, CSR_INT_BIT_RX_PERIODIC);
             }
             /* Sending RX interrupt require many steps to be done in the
              * the device:
              * 1- write interrupt to current index in ICT table.
              * 2- dma RX frame.
              * 3- update RX shared data to indicate last write index.
              * 4- send interrupt.
              * This could lead to RX race, driver could receive RX interrupt
              * but the shared data changes does not reflect this;
              * periodic interrupt will detect any dangling Rx activity.
              */
     
             /* Disable periodic interrupt; we use it as just a one-shot. */
             trans->iwlWrite8(CSR_INT_PERIODIC_REG,
                              CSR_INT_PERIODIC_DIS);
     
             /*
              * Enable periodic interrupt in 8 msec only if we received
              * real RX interrupt (instead of just periodic int), to catch
              * any dangling Rx interrupt.  If it was just the periodic
              * interrupt, there was no dangling Rx activity, and no need
              * to extend the periodic interrupt; one-shot is enough.
              */
             if (inta & (CSR_INT_BIT_FH_RX | CSR_INT_BIT_SW_RX))
                 trans->iwlWrite8(CSR_INT_PERIODIC_REG,
                                  CSR_INT_PERIODIC_ENA);
     
             isr_stats->rx++;
     
             local_bh_disable();
             trans->handleRx(0);
             local_bh_enable();
         }
     
         /* This "Tx" DMA channel is used only for loading uCode */
         if (inta & CSR_INT_BIT_FH_TX) {
             trans->iwlWrite32(CSR_FH_INT_STATUS, CSR_FH_INT_TX_MASK);
             IWL_INFO(trans, "uCode load interrupt\n");
             isr_stats->tx++;
             handled |= CSR_INT_BIT_FH_TX;
             /* Wake up uCode load routine, now that load is complete */
             IOLockLock(trans->ucode_write_waitq);
             trans->ucode_write_complete = true;
             IOLockWakeup(trans->ucode_write_waitq, &trans->ucode_write_complete, true);
             IOLockUnlock(trans->ucode_write_waitq);
             //iwl_notification_notify(&m_pDevice->notif_wait);
             //wake_up(&trans_pcie->ucode_write_waitq);
         }
     
         if (inta & ~handled) {
             IWL_ERR(trans, "Unhandled INTA bits 0x%08x\n", inta & ~handled);
             isr_stats->unhandled++;
         }
     
         if (inta & ~(trans->inta_mask)) {
             IWL_WARN(trans, "Disabled INTA bits 0x%08x were pending\n",
                      inta & ~trans->inta_mask);
         }
     
         /* only Re-enable all interrupt if disabled by irq */
         if (test_bit(STATUS_INT_ENABLED, &trans->status))
             trans->enableIntrDirectly();
         /* we are loading the firmware, enable FH_TX interrupt only */
         else if (handled & CSR_INT_BIT_FH_TX)
             trans->enableFWLoadIntr();
         /* Re-enable RF_KILL if it occurred */
         else if (handled & CSR_INT_BIT_RF_KILL)
             trans->enableRFKillIntr();
         /* Re-enable the ALIVE / Rx interrupt if it occurred */
         else if (handled & (CSR_INT_BIT_ALIVE | CSR_INT_BIT_FH_RX))
             trans->iwl_enable_fw_load_int_ctx_info();
     out:
     
    return 0;
}

