//
//  IWLFw.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/8.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmDriver.hpp"

static bool iwl_wait_phy_db_entry(struct iwl_notif_wait_data *notif_wait,
                                  struct iwl_rx_packet *pkt, void *data)
{
    IWL_INFO(0, "runInitMvmUCode init_complete\n");
    struct iwl_phy_db *phy_db = (struct iwl_phy_db *)data;
    
    if (pkt->hdr.cmd != CALIB_RES_NOTIF_PHY_DB) {
        WARN_ON(pkt->hdr.cmd != INIT_COMPLETE_NOTIF);
        return true;
    }
    
    WARN_ON(iwl_phy_db_set_section(phy_db, pkt));
    
    return false;
}

int IWLMvmDriver::runInitMvmUCode(bool read_nvm)
{
    IWL_INFO(0, "runInitMVMUcode %s", read_nvm ? "read nvm" : "do not read nvm");
    struct iwl_notification_wait calib_wait;
    static const u16 init_complete[] = {
        INIT_COMPLETE_NOTIF,
        CALIB_RES_NOTIF_PHY_DB
    };
    int ret;
    if (iwl_mvm_has_unified_ucode(m_pDevice))
        return runUnifiedMvmUcode(true);
    
    //this->trans->mutex = IOLockAlloc();
    
    if(!this->trans) {
        IWL_ERR(0, "trans ???");
        return 0;
    }
    
    if(!this->trans->mutex) {
        IWL_ERR(0, "Could not alloc mutex\n");
        return 0;
    }
    
    if(!IOLockTryLock(this->trans->mutex)) {
        IWL_ERR(0, "Could not lock mutex\n");
        return 0;
    }
    IWL_INFO(0, "Launching initial calibration sequence");
    m_pDevice->rfkill_safe_init_done = false;
    iwl_init_notification_wait(&m_pDevice->notif_wait,
                               &calib_wait,
                               init_complete,
                               ARRAY_SIZE(init_complete),
                               iwl_wait_phy_db_entry,
                               &this->phy_db);
    
    /*
     * Some things may run in the background now, but we
     * just wait for the calibration complete notification.
     */
    /*
    ret = iwl_wait_notification(&m_pDevice->notif_wait, &calib_wait,
                                MVM_UCODE_CALIB_TIMEOUT);
    
    
    if (!ret)
        goto out;
     */
    IOLockUnlock(this->trans->mutex);
    /* Will also start the device */
    ret = loadUcodeWaitAlive(IWL_UCODE_INIT);
    if (ret) {
        IWL_ERR(0, "Failed to start INIT ucode: %d\n", ret);
        goto remove_notif;
    }
    
    if (m_pDevice->cfg->trans.device_family < IWL_DEVICE_FAMILY_8000) {
        ret = sendBTInitConf();
        if (ret)
            goto remove_notif;
    }
    
    /* Read the NVM only at driver load time, no need to do this twice */
    if (read_nvm) {
        ret = nvmInit();
        if (ret) {
            IWL_ERR(0, "Failed to read NVM: %d\n", ret);
            goto remove_notif;
        }
    }
    
    return 0;
    
    //TODO see when it will be happened
    //    /* In case we read the NVM from external file, load it to the NIC */
    //    if (mvm->nvm_file_name) {
    ////        iwl_mvm_load_nvm_to_nic(mvm);
    //    }
    
    if (m_pDevice->nvm_data->nvm_version < m_pDevice->cfg->nvm_ver) {
        IWL_ERR(0, "Too old NVM version (0x%0x, required = 0x%0x)",
                m_pDevice->nvm_data->nvm_version, m_pDevice->cfg->nvm_ver);
    }
    
    /*
     * abort after reading the nvm in case RF Kill is on, we will complete
     * the init seq later when RF kill will switch to off
     */
    if (iwl_mvm_is_radio_hw_killed(m_pDevice)) {
        IWL_ERR(0,
                "jump over all phy activities due to RF kill\n");
        goto remove_notif;
    }
    
    m_pDevice->rfkill_safe_init_done = true;
    
    /* Send TX valid antennas before triggering calibrations */
    ret = sendTXAntCfg(iwl_mvm_get_valid_tx_ant(m_pDevice));
    if (ret)
        goto remove_notif;
    
    ret = sendPhyCfgCmd();
    if (ret) {
        IWL_ERR(0, "Failed to run INIT calibrations: %d\n",
                ret);
        goto remove_notif;
    }
    
    IWL_INFO(0, "wait firmware notification\n");
    
    /*
     * Some things may run in the background now, but we
     * just wait for the calibration complete notification.
     */

    ret = iwl_wait_notification(&m_pDevice->notif_wait, &calib_wait,
                                MVM_UCODE_CALIB_TIMEOUT);
    if (!ret)
        goto out;
    
    if (iwl_mvm_is_radio_hw_killed(m_pDevice)) {
        IWL_INFO(0, "RFKILL while calibrating.\n");
        ret = 0;
    } else {
        IWL_ERR(0, "Failed to run INIT calibrations: %d\n",
                ret);
    }
    goto out;
remove_notif:
    //iwl_remove_notification(&m_pDevice->notif_wait, &calib_wait);
out:
    m_pDevice->rfkill_safe_init_done = false;
    if (!m_pDevice->nvm_data) {
        //zxy fake nvm_data for debug, don‘t need for now
        //        /* we want to debug INIT and we have no NVM - fake */
        //        mvm->nvm_data = kzalloc(sizeof(struct iwl_nvm_data) +
        //                    sizeof(struct ieee80211_channel) +
        //                    sizeof(struct ieee80211_rate),
        //                    GFP_KERNEL);
        //        if (!mvm->nvm_data)
        //            return -ENOMEM;
        //        mvm->nvm_data->bands[0].channels = mvm->nvm_data->channels;
        //        mvm->nvm_data->bands[0].n_channels = 1;
        //        mvm->nvm_data->bands[0].n_bitrates = 1;
        //        mvm->nvm_data->bands[0].bitrates =
        //            (void *)mvm->nvm_data->channels + 1;
        //        mvm->nvm_data->bands[0].bitrates->hw_value = 10;
    }
    return ret;
}

static bool iwl_wait_init_complete(struct iwl_notif_wait_data *notif_wait,
                                   struct iwl_rx_packet *pkt, void *data)
{
    WARN_ON(pkt->hdr.cmd != INIT_COMPLETE_NOTIF);
    
    return true;
}

int IWLMvmDriver::runUnifiedMvmUcode(bool read_nvm)
{
    struct iwl_notification_wait init_wait;
    struct iwl_nvm_access_complete_cmd nvm_complete = {};
    struct iwl_init_extended_cfg_cmd init_cfg = {
        .init_flags = cpu_to_le32(BIT(IWL_INIT_NVM)),
    };
    static const u16 init_complete[] = {
        INIT_COMPLETE_NOTIF,
    };
    int ret;
    if (m_pDevice->cfg->tx_with_siso_diversity)
        init_cfg.init_flags |= cpu_to_le32(BIT(IWL_INIT_PHY));
    m_pDevice->rfkill_safe_init_done = false;
    iwl_init_notification_wait(&m_pDevice->notif_wait,
                               &init_wait,
                               init_complete,
                               ARRAY_SIZE(init_complete),
                               iwl_wait_init_complete,
                               NULL);
    /* Will also start the device */
    ret = loadUcodeWaitAlive(IWL_UCODE_REGULAR);
    return 0;
    if (ret) {
        IWL_ERR(mvm, "Failed to start RT ucode: %d\n", ret);
        goto error;
    }
    /* Send init config command to mark that we are sending NVM access
     * commands
     */
    ret = sendCmdPdu(WIDE_ID(SYSTEM_GROUP,
                             INIT_EXTENDED_CFG_CMD),
                     CMD_SEND_IN_RFKILL,
                     sizeof(init_cfg), &init_cfg);
    if (ret) {
        IWL_ERR(mvm, "Failed to run init config command: %d\n",
                ret);
        goto error;
    }
    
    //        /* Load NVM to NIC if needed */
    //        if (mvm->nvm_file_name) {
    //            iwl_read_external_nvm(mvm->trans, mvm->nvm_file_name,
    //                          mvm->nvm_sections);
    //            iwl_mvm_load_nvm_to_nic(mvm);
    //        }
    
    if (IWL_MVM_PARSE_NVM && read_nvm) {
        ret = nvmInit();
        if (ret) {
            IWL_ERR(mvm, "Failed to read NVM: %d\n", ret);
            goto error;
        }
    }
    
    ret = sendCmdPdu(WIDE_ID(REGULATORY_AND_NVM_GROUP,
                             NVM_ACCESS_COMPLETE),
                     CMD_SEND_IN_RFKILL,
                     sizeof(nvm_complete), &nvm_complete);
    if (ret) {
        IWL_ERR(mvm, "Failed to run complete NVM access: %d\n",
                ret);
        goto error;
    }
    
    /* We wait for the INIT complete notification */
    ret = iwl_wait_notification(&m_pDevice->notif_wait, &init_wait,
                                MVM_UCODE_ALIVE_TIMEOUT);
    if (ret)
        return ret;
    
    /* Read the NVM only at driver load time, no need to do this twice */
    if (!IWL_MVM_PARSE_NVM && read_nvm) {
        m_pDevice->nvm_data = getNvm(this->trans, &m_pDevice->fw);
        if (IS_ERR(m_pDevice->nvm_data)) {
            ret = PTR_ERR(m_pDevice->nvm_data);
            m_pDevice->nvm_data = NULL;
            IWL_ERR(mvm, "Failed to read NVM: %d\n", ret);
            return ret;
        }
    }
    m_pDevice->rfkill_safe_init_done = true;
    return 0;
    
error:
    iwl_remove_notification(&m_pDevice->notif_wait, &init_wait);
    return ret;
}

int IWLMvmDriver::sendPhyCfgCmd()
{
    
    return 0;
}

int IWLMvmDriver::sendTXAntCfg(u8 valid_tx_ant)
{
    
    return 0;
}

static bool iwl_alive_fn(struct iwl_notif_wait_data *notif_wait,
                         struct iwl_rx_packet *pkt, void *data)
{
    IWL_INFO(0, "iwl alive.\n");
//    IWLDevice *mvm =
//    container_of(notif_wait, typeof(IWLDevice), notif_wait);
    struct iwl_mvm_alive_data *alive_data = (struct iwl_mvm_alive_data *)data;
    struct mvm_alive_resp_v3 *palive3;
    struct mvm_alive_resp *palive;
    struct iwl_umac_alive *umac;
    struct iwl_lmac_alive *lmac1;
    struct iwl_lmac_alive *lmac2 = NULL;
    u16 status;
    u32 lmac_error_event_table, umac_error_event_table;

    if (iwl_rx_packet_payload_len(pkt) == sizeof(*palive)) {
        palive = (struct mvm_alive_resp *)pkt->data;
        umac = &palive->umac_data;
        lmac1 = &palive->lmac_data[0];
        lmac2 = &palive->lmac_data[1];
        status = le16_to_cpu(palive->status);
    } else {
        palive3 = (struct mvm_alive_resp_v3 *)pkt->data;
        umac = &palive3->umac_data;
        lmac1 = &palive3->lmac_data;
        status = le16_to_cpu(palive3->status);
    }
    
    alive_data->scd_base_addr = le32_to_cpu(lmac1->dbg_ptrs.scd_base_ptr);
    alive_data->valid = status == IWL_ALIVE_STATUS_OK;
    
//    alive_data->scd_base_addr = 0;
//    alive_data->valid = 1;
    
    return true;
}

int IWLMvmDriver::loadUcodeWaitAlive(enum iwl_ucode_type ucode_type)
{
    IWL_INFO(0, "start load ucode\n");
    struct iwl_notification_wait alive_wait;
    struct iwl_mvm_alive_data alive_data = {};
    const struct fw_img *fw;
    int ret;
    enum iwl_ucode_type old_type = m_pDevice->cur_fw_img;
    static const u16 alive_cmd[] = { MVM_ALIVE };
    bool run_in_rfkill =
    ucode_type == IWL_UCODE_INIT || iwl_mvm_has_unified_ucode(m_pDevice);
    
    if (ucode_type == IWL_UCODE_REGULAR &&
        iwl_fw_dbg_conf_usniffer(&m_pDevice->fw, FW_DBG_START_FROM_ALIVE) &&
        !(fw_has_capa(&m_pDevice->fw.ucode_capa,
                      IWL_UCODE_TLV_CAPA_USNIFFER_UNIFIED)))
        fw = iwl_get_ucode_image(&m_pDevice->fw, IWL_UCODE_REGULAR_USNIFFER);
    else
        fw = iwl_get_ucode_image(&m_pDevice->fw, ucode_type);
    if (WARN_ON(!fw))
        return -EINVAL;
    m_pDevice->cur_fw_img = ucode_type;
    clear_bit(IWL_MVM_STATUS_FIRMWARE_RUNNING, &m_pDevice->status);
    
    
    
    iwl_init_notification_wait(&m_pDevice->notif_wait, &alive_wait,
                               alive_cmd, ARRAY_SIZE(alive_cmd),
                               iwl_alive_fn, &alive_data);
    
    /*
     * We want to load the INIT firmware even in RFKILL
     * For the unified firmware case, the ucode_type is not
     * INIT, but we still need to run it.
     */
    ret = trans_ops->startFW(fw, run_in_rfkill);
    
    IWL_INFO(mvm, "startFW result: %d\n", ret);
    
    if (ret) {
        m_pDevice->cur_fw_img = old_type;
        iwl_remove_notification(&m_pDevice->notif_wait, &alive_wait);
        return ret;
    }
//    IOLockLock(trans->ucode_write_waitq);
//    AbsoluteTime deadline;
//    clock_interval_to_deadline(MVM_UCODE_ALIVE_TIMEOUT, kSecondScale, (UInt64 *) &deadline);
//    ret = IOLockSleepDeadline(trans->ucode_write_waitq, &alive_wait,
//                              deadline, THREAD_INTERRUPTIBLE);
//    IOLockUnlock(trans->ucode_write_waitq);
//
//    return 0;
    

    
    /*
     * Some things may run in the background now, but we
     * just wait for the ALIVE notification here.
     */
    IWL_INFO(0, "Waiting for alive signal");
    ret = iwl_wait_notification(&m_pDevice->notif_wait, &alive_wait,
                                MVM_UCODE_ALIVE_TIMEOUT * 100);
    
    if (ret) {
        if (m_pDevice->cfg->trans.device_family >=
            IWL_DEVICE_FAMILY_22000) {
            IWL_ERR(mvm,
                    "SecBoot CPU1 Status: 0x%x, CPU2 Status: 0x%x\n",
                    trans->iwlReadUmacPRPH(UMAG_SB_CPU_1_STATUS),
                    trans->iwlReadUmacPRPH(UMAG_SB_CPU_2_STATUS));
            IWL_ERR(mvm, "UMAC PC: 0x%x\n",
                    trans->iwlReadUmacPRPH(UREG_UMAC_CURRENT_PC));
            IWL_ERR(mvm, "LMAC PC: 0x%x\n",
                    trans->iwlReadUmacPRPH(UREG_LMAC1_CURRENT_PC));
            if (iwl_mvm_is_cdb_supported(m_pDevice))
                IWL_ERR(mvm, "LMAC2 PC: 0x%x\n",
                        trans->iwlReadUmacPRPH(UREG_LMAC2_CURRENT_PC));
        } else if (m_pDevice->cfg->trans.device_family >=
                   IWL_DEVICE_FAMILY_8000) {
            IWL_ERR(mvm,
                    "SecBoot CPU1 Status: 0x%x, CPU2 Status: 0x%x\n",
                    trans->iwlReadPRPH(SB_CPU_1_STATUS),
                    trans->iwlReadPRPH(SB_CPU_2_STATUS));
        }

        if (ret == -ETIMEDOUT)
            IWL_ERR(0, "timeout. iwl_fw_dbg_error_collect\n");
        m_pDevice->cur_fw_img = old_type;
        return ret;
    }
    //alive_data.valid = true;
    if (!alive_data.valid) {
        IWL_ERR(mvm, "Loaded ucode is not valid!\n");
        m_pDevice->cur_fw_img = old_type;
        return -EIO;
    }
    
    trans_ops->fwAlive(alive_data.scd_base_addr);
    
    /*
     * Note: all the queues are enabled as part of the interface
     * initialization, but in firmware restart scenarios they
     * could be stopped, so wake them up. In firmware restart,
     * mac80211 will have the queues stopped as well until the
     * reconfiguration completes. During normal startup, they
     * will be empty.
     */
    
    memset(&m_pDevice->queue_info, 0, sizeof(m_pDevice->queue_info));
    /*
     * Set a 'fake' TID for the command queue, since we use the
     * hweight() of the tid_bitmap as a refcount now. Not that
     * we ever even consider the command queue as one we might
     * want to reuse, but be safe nevertheless.
     */
    m_pDevice->queue_info[IWL_MVM_DQA_CMD_QUEUE].tid_bitmap =
    BIT(IWL_MAX_TID_COUNT + 2);
    
    set_bit(IWL_MVM_STATUS_FIRMWARE_RUNNING, &m_pDevice->status);
    
    return 0;
}
