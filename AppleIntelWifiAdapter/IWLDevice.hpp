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
    
    int probe();
    
    void release();
    
    void enablePCI();
    
    bool NICInit();
    
    bool NICStart();
    
public:
    IOPCIDevice *pciDevice;
    const struct iwl_cfg *cfg;
    
    IOSimpleLock *registerRWLock;
    bool holdNICWake;
    
private:
    
    
private:
    const char *name;
    
    const iwl_cfg_trans_params *trans_cfg;
};


#endif /* IWLDevice_hpp */
