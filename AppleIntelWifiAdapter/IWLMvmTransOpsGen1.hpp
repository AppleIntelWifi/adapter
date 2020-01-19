//
//  IWLMvmTransOpsGen1.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmTransOpsGen1_hpp
#define IWLMvmTransOpsGen1_hpp

#include "IWLIO.hpp"
#include "IWLTransOps.h"

class IWLMvmTransOpsGen1 : public IWLTransOps {
    
    
public:
    
    IWLMvmTransOpsGen1() {}
    IWLMvmTransOpsGen1(IWLIO *io) : IWLTransOps(io) {}
    ~IWLMvmTransOpsGen1() {}
    
    bool startHW() override;
    
    void fwAlive(UInt32 scd_addr) override;
    
    bool startFW() override;
    
    void stopDevice() override;
    
    void sendCmd(iwl_host_cmd *cmd) override;
};

#endif /* IWLMvmTransOpsGen1_hpp */
