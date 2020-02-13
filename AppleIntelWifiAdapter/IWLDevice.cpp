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
    UInt16 vendorID = pciDevice->configRead16(kIOPCIConfigVendorID);
    if (vendorID != PCI_VENDOR_ID_INTEL) {
        return false;
    }
    deviceID = pciDevice->configRead16(kIOPCIConfigDeviceID);
    for (int i = 0; i < ARRAY_SIZE(iwl_hw_card_ids); i++) {
        pci_device_id dev = iwl_hw_card_ids[i];
        if (dev.device == deviceID) {
            this->cfg = (struct iwl_cfg *)dev.driver_data;
            break;
        }
    }
    subSystemDeviceID = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    this->rx_sync_waitq = IOLockAlloc();
    if(this->cfg != NULL) {
        pciDevice->retain();
        return true;
    }
    else {
        this->pciDevice = NULL;
        return false;
    }
}

void IWLDevice::release()
{
    if (this->registerRWLock) {
        IOSimpleLockFree(this->registerRWLock);
        this->registerRWLock = NULL;
    }
    if (this->rx_sync_waitq) {
        IOLockFree(this->rx_sync_waitq);
        this->rx_sync_waitq = NULL;
    }
    
    if(this->pciDevice) {
        this->pciDevice->release();
    }
}

void IWLDevice::enablePCI()
{
    pciDevice->setBusMasterEnable(true);
    pciDevice->setMemoryEnable(true);
    pciDevice->setIOEnable(true);
}
