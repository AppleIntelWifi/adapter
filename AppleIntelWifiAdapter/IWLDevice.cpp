//
//  IWLDevice.cpp
//  AppleIntelWifiAdapter
//
//  Created by qcwap on 2020/1/5.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLDevice.hpp"

bool IWLDevice::init()
{
    this->iwl_trans = new IWLTransport();
    return true;
}

void IWLDevice::release()
{
    if (this->iwl_trans) {
        delete this->iwl_trans;
        this->iwl_trans = NULL;
    }
}

int IWLDevice::probe(IOPCIDevice *pciDevice)
{
    const struct iwl_cfg *cfg = NULL;
    const struct iwl_cfg *cfg_7265d = NULL;
    iwl_trans->setDevice(pciDevice);
    this->pciDevice = pciDevice;
    UInt16 vendorID = pciDevice->configRead16(kIOPCIConfigVendorID);
    if (vendorID != PCI_VENDOR_ID_INTEL) {
        IOLog("这不就是因特尔吗？？%04x\n", vendorID);
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
            pciDeviceConfig = cfg;
            IOLog("device name=%s\n", dev_info->name);
            IOLog("%s", cfg->fw_name_pre);
        }
    }
    
//    IOLog("我不断的寻找\n");
//    for (int i = 0; i < ARRAY_SIZE(iwl_hw_card_ids); i++) {
//        pci_device_id dev = iwl_hw_card_ids[i];
//        if (dev.subdevice == subSystemDeviceID && dev.device == deviceID) {
//            pciDeviceConfig = (pci_device_id *)IOMalloc(sizeof(pci_device_id));
//            *pciDeviceConfig = dev;
//            const struct iwl_cfg *cc = (struct iwl_cfg *)pciDeviceConfig->driver_data;
//            IOLog("我找到啦！！！%s", cc->fw_name_pre);
//            break;
//        }
//    }
//    if (pciDeviceConfig) {
//        
//        /*
//         * special-case 7265D, it has the same PCI IDs.
//         *
//         * Note that because we already pass the cfg to the transport above,
//         * all the parameters that the transport uses must, until that is
//         * changed, be identical to the ones in the 7265D configuration.
//         */
//        if (cfg == &iwl7265_2ac_cfg)
//            cfg_7265d = &iwl7265d_2ac_cfg;
//        else if (cfg == &iwl7265_2n_cfg)
//            cfg_7265d = &iwl7265d_2n_cfg;
//        else if (cfg == &iwl7265_n_cfg)
//            cfg_7265d = &iwl7265d_n_cfg;
//        if (cfg_7265d &&
//            (revision & CSR_HW_REV_TYPE_MSK) == CSR_HW_REV_TYPE_7265D)
//            cfg = cfg_7265d;
//        
//        if (pciDeviceName != NULL) {
//            IOLog("device name=%s\n", pciDeviceName);
//        }
//        IOLog("%s", cfg->fw_name_pre);
//
//    }
    return pciDeviceConfig != NULL;;
}
