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
#include "fw/IWLUcodeParse.hpp"

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
    
    bool drvStart();
    
private:
    
    IOLock *fwLoadLock;
    
    IWLDevice *m_pDevice;
    
    IWLTransport *trans;
};

static inline bool iwl_mvm_has_new_rx_api(struct iwl_fw *fw)
{
    return fw_has_capa(&fw->ucode_capa,
               IWL_UCODE_TLV_CAPA_MULTI_QUEUE_RX_SUPPORT);
}

#endif /* IWLMvmDriver_hpp */
