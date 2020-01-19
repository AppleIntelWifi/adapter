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

#include "IWLDeviceList.h"

class IWLDevice
{
public:
    
    bool init(IOPCIDevice *pciDevice);
    
    void release();
    
    void enablePCI();
    
public:
    
    IOPCIDevice *pciDevice;
    
    const struct iwl_cfg *cfg;
    const char *name;
    uint32_t hw_rf_id;
    uint32_t hw_rev;
    UInt16 deviceID;
    uint16_t hw_id;
    uint16_t subSystemDeviceID;
    char hw_id_str[64];
    
    IOSimpleLock *registerRWLock;
    
    bool holdNICWake;
    
private:
    
    
};


#endif /* IWLDevice_hpp */
