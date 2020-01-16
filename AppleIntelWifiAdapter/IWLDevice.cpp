//
//  IWLDevice.cpp
//  AppleIntelWifiAdapter
//
//  Created by qcwap on 2020/1/5.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLDevice.hpp"

bool IWLDevice::init(IOPCIDevice *pciDevice)
{
    this->pciDevice = pciDevice;
    this->registerRWLock = IOSimpleLockAlloc();
    return true;
}

void IWLDevice::release()
{
    if (this->registerRWLock) {
        IOSimpleLockFree(this->registerRWLock);
        this->registerRWLock = NULL;
    }
}

int IWLDevice::probe()
{
    const struct iwl_cfg *cfg = NULL;
    const struct iwl_cfg *cfg_7265d = NULL;
    UInt16 vendorID = pciDevice->configRead16(kIOPCIConfigVendorID);
    if (vendorID != PCI_VENDOR_ID_INTEL) {
        return 0;
    }
    UInt16 deviceID = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subSystemDeviceID = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    UInt8 revision = pciDevice->configRead8(kIOPCIConfigRevisionID);
    uint32_t hw_rf_id = pciDevice->configRead32(CSR_HW_RF_ID);
    
    for (int i = 0; i < ARRAY_SIZE(iwl_dev_info_table); i++) {
        const struct iwl_dev_info *dev_info = &iwl_dev_info_table[i];
        if ((dev_info->device == (u16)IWL_CFG_ANY ||
             dev_info->device == deviceID) &&
            (dev_info->subdevice == (u16)IWL_CFG_ANY ||
             dev_info->subdevice == subSystemDeviceID) &&
            (dev_info->mac_type == (u16)IWL_CFG_ANY ||
             dev_info->mac_type ==
             CSR_HW_REV_TYPE(revision)) &&
            (dev_info->mac_step == (u8)IWL_CFG_ANY ||
             dev_info->mac_step ==
             CSR_HW_REV_STEP(revision)) &&
            (dev_info->rf_type == (u16)IWL_CFG_ANY ||
             dev_info->rf_type ==
             CSR_HW_RFID_TYPE(hw_rf_id)) &&
            (dev_info->rf_id == (u8)IWL_CFG_ANY ||
             dev_info->rf_id ==
             IWL_SUBDEVICE_RF_ID(subSystemDeviceID)) &&
            (dev_info->no_160 == (u8)IWL_CFG_ANY ||
             dev_info->no_160 ==
             IWL_SUBDEVICE_NO_160(subSystemDeviceID)) &&
            (dev_info->cores == (u8)IWL_CFG_ANY ||
             dev_info->cores ==
             IWL_SUBDEVICE_CORES(subSystemDeviceID))) {
            cfg = dev_info->cfg;
            name = dev_info->name;
        }
    }
    
    IOLog("我不断的寻找\n");
    for (int i = 0; i < ARRAY_SIZE(iwl_hw_card_ids); i++) {
        pci_device_id dev = iwl_hw_card_ids[i];
        if (dev.subdevice == subSystemDeviceID && dev.device == deviceID) {
            cfg = (struct iwl_cfg *)dev.driver_data;
            IOLog("我找到啦！！！");
            break;
        }
    }
    if (cfg) {
        /*
         * special-case 7265D, it has the same PCI IDs.
         *
         * Note that because we already pass the cfg to the transport above,
         * all the parameters that the transport uses must, until that is
         * changed, be identical to the ones in the 7265D configuration.
         */
        if (cfg == &iwl7265_2ac_cfg)
            cfg_7265d = &iwl7265d_2ac_cfg;
        else if (cfg == &iwl7265_2n_cfg)
            cfg_7265d = &iwl7265d_2n_cfg;
        else if (cfg == &iwl7265_n_cfg)
            cfg_7265d = &iwl7265d_n_cfg;
        if (cfg_7265d &&
            (revision & CSR_HW_REV_TYPE_MSK) == CSR_HW_REV_TYPE_7265D)
            cfg = cfg_7265d;
        
        if (cfg == &iwlax210_2ax_cfg_so_hr_a0) {
            if (revision == CSR_HW_REV_TYPE_TY) {
                cfg = &iwlax210_2ax_cfg_ty_gf_a0;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_JF)) {
                cfg = &iwlax210_2ax_cfg_so_jf_a0;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_GF)) {
                cfg = &iwlax211_2ax_cfg_so_gf_a0;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_GF4)) {
                cfg = &iwlax411_2ax_cfg_so_gf4_a0;
            }
        } else if (cfg == &iwl_ax101_cfg_qu_hr) {
            if ((CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id) ==
                 CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR) &&
                 revision == CSR_HW_REV_TYPE_QNJ_B0) ||
                (CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id) ==
                 CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR1))) {
                cfg = &iwl22000_2ax_cfg_qnj_hr_b0;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR) &&
                       revision == CSR_HW_REV_TYPE_QUZ) {
                cfg = &iwl_ax101_cfg_quz_hr;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR)) {
                cfg = &iwl_ax101_cfg_qu_hr;
            } else if (CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id) ==
                       CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HRCDB)) {
                IWL_ERR(0, "RF ID HRCDB is not supported\n");
                goto error;
            } else {
                IWL_ERR(0, "Unrecognized RF ID 0x%08x\n",
                        CSR_HW_RF_ID_TYPE_CHIP_ID(hw_rf_id));
                goto error;
            }
        }
        
        /*
         * The RF_ID is set to zero in blank OTP so read version to
         * extract the RF_ID.
         */
        if (cfg->trans.rf_id &&
            !CSR_HW_RFID_TYPE(hw_rf_id)) {
            unsigned long flags;
            
            //            if (iwl_trans_grab_nic_access(iwl_trans, &flags)) {
            //                u32 val;
            //
            //                val = iwl_read_umac_prph_no_grab(iwl_trans,
            //                                                 WFPM_CTRL_REG);
            //                val |= ENABLE_WFPM;
            //                iwl_write_umac_prph_no_grab(iwl_trans, WFPM_CTRL_REG,
            //                                            val);
            //                val = iwl_read_prph_no_grab(iwl_trans, SD_REG_VER);
            //
            //                val &= 0xff00;
            //                switch (val) {
            //                    case REG_VER_RF_ID_JF:
            //                        iwl_trans->hw_rf_id = CSR_HW_RF_ID_TYPE_JF;
            //                        break;
            //                        /* TODO: get value for REG_VER_RF_ID_HR */
            //                    default:
            //                        iwl_trans->hw_rf_id = CSR_HW_RF_ID_TYPE_HR;
            //                }
            //                iwl_trans_release_nic_access(iwl_trans, &flags);
            //            }
        }
        
        /*
         * This is a hack to switch from Qu B0 to Qu C0.  We need to
         * do this for all cfgs that use Qu B0, except for those using
         * Jf, which have already been moved to the new table.  The
         * rest must be removed once we convert Qu with Hr as well.
         */
        if (revision == CSR_HW_REV_TYPE_QU_C0) {
            if (cfg == &iwl_ax101_cfg_qu_hr)
                cfg = &iwl_ax101_cfg_qu_c0_hr_b0;
            else if (cfg == &iwl_ax201_cfg_qu_hr)
                cfg = &iwl_ax201_cfg_qu_c0_hr_b0;
            else if (cfg == &killer1650s_2ax_cfg_qu_b0_hr_b0)
                cfg = &killer1650s_2ax_cfg_qu_c0_hr_b0;
            else if (cfg == &killer1650i_2ax_cfg_qu_b0_hr_b0)
                cfg = &killer1650i_2ax_cfg_qu_c0_hr_b0;
        }
        
        /* same thing for QuZ... */
        if (revision == CSR_HW_REV_TYPE_QUZ) {
            if (cfg == &iwl_ax101_cfg_qu_hr)
                cfg = &iwl_ax101_cfg_quz_hr;
            else if (cfg == &iwl_ax201_cfg_qu_hr)
                cfg = &iwl_ax201_cfg_quz_hr;
        }
        
        
        //初始化
        trans_cfg = &cfg->trans;
        /* if we don't have a name yet, copy name from the old cfg */
        if (!this->name)
            name = cfg->name;
        
        
        IOLog("device name=%s\n", name);
        IOLog("%s", cfg->fw_name_pre);
        
        //        if (trans_cfg->mq_rx_supported) {
        //            if (!cfg->num_rbds) {
        //                goto error;
        //            }
        //            trans_pcie->num_rx_bufs = iwl_trans->cfg->num_rbds;
        //        } else {
        //            trans_pcie->num_rx_bufs = RX_QUEUE_SIZE;
        //        }
        //
        //        if (trans_cfg->device_family >= IWL_DEVICE_FAMILY_8000 &&
        //            iwl_trans_grab_nic_access(iwl_trans, &flags)) {
        //            u32 hw_step;
        //
        //            hw_step = iwl_read_umac_prph_no_grab(iwl_trans, WFPM_CTRL_REG);
        //            hw_step |= ENABLE_WFPM;
        //            iwl_write_umac_prph_no_grab(iwl_trans, WFPM_CTRL_REG, hw_step);
        //            hw_step = iwl_read_prph_no_grab(iwl_trans, CNVI_AUX_MISC_CHIP);
        //            hw_step = (hw_step >> HW_STEP_LOCATION_BITS) & 0xF;
        //            if (hw_step == 0x3)
        //                iwl_trans->hw_rev = (iwl_trans->hw_rev & 0xFFFFFFF3) |
        //                (SILICON_C_STEP << 2);
        //            iwl_trans_release_nic_access(iwl_trans, &flags);
        //        }
    }
    return cfg != NULL;
error:
    return false;
}

void IWLDevice::enablePCI()
{
    pciDevice->setIOEnable(true);
    pciDevice->setMemoryEnable(true);
    pciDevice->setBusMasterEnable(true);
}

bool IWLDevice::NICInit()
{
    
    return true;
}

bool IWLDevice::NICStart()
{
    
    return true;
}
