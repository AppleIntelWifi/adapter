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
    iwl_trans->setDevice(pciDevice);
    this->pciDevice = pciDevice;
    
    return 0;
}
