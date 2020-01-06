//
//  IWLTransport.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLTransport_hpp
#define IWLTransport_hpp

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <linux/types.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>

class IWLTransport
{

public:
    IWLTransport();
    ~IWLTransport();
    void setDevice(IOPCIDevice *device);

private:
    
    void osWriteInt8(volatile void* base, uintptr_t byteOffset, uint8_t data);
    
private:
    IOPCIDevice *pciDevice;
};

#endif /* IWLTransport_hpp */
