//
//  IWLMvmDriver.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/17.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmDriver_hpp
#define IWLMvmDriver_hpp

#include <libkern/OSKextLib.h>
#include <IOKit/IOLib.h>

#include "IWLTransport.hpp"

class IWLMvmDriver {
    
public:

    bool init(IOPCIDevice *pciDevice);
    
    void release();
    
    bool probe();
    
    bool start();
    
    static void reqFWCallback(
                              OSKextRequestTag requestTag,
                              OSReturn result,
                              const void* resourceData,
                              uint32_t resourceDataLength,
                              void* context);
    
private:
    
    IWLDevice *m_pDevice;
    
    IWLTransport *trans;
};

#endif /* IWLMvmDriver_hpp */
