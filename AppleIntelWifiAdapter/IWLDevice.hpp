//
//  IWLDevice.hpp
//  AppleIntelWifiAdapter
//
//  Created by qcwap on 2020/1/5.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLDevice_hpp
#define IWLDevice_hpp

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include "IWLTransport.hpp"

#include "IWLDeviceList.h"

class IWLDevice
{
public:
    
    bool init();
    
    int probe(IOPCIDevice *pciDevice);
    
    void release();
    
private:
    
    
private:
    IOPCIDevice *pciDevice;
    IWLTransport *iwl_trans;
};


#endif /* IWLDevice_hpp */
