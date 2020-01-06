//
//  IWLTransport.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"

IWLTransport::IWLTransport()
{
}

IWLTransport::~IWLTransport()
{
}

void IWLTransport::setDevice(IOPCIDevice *device)
{
    this->pciDevice = device;
}

void IWLTransport::osWriteInt8(volatile void *base, uintptr_t byteOffset, uint8_t data)
{
    *(volatile uint8_t *)((uintptr_t)base + byteOffset) = data;
}
