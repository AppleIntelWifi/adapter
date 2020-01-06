//
//  IWLTransport.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"

void IWLTransport::setDevice(IOPCIDevice *device)
{
    this->pciDevice = device;
}
