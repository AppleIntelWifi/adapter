//
//  IWLMvmDriver.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/17.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmDriver_hpp
#define IWLMvmDriver_hpp

#include "IWLTransport.hpp"

class IWLMvmDriver {
    
public:

    bool init(IOPCIDevice *pciDevice);
    
    void release();
    
    int probe();
    
    bool start();
    
private:
    
    IWLDevice *m_pDevice;
    
    IOLock *fwLoadLock;
    
    
    IWLTransport *trans;
};

#endif /* IWLMvmDriver_hpp */
