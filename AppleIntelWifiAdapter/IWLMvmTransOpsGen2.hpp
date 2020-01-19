//
//  IWLMvmTransOpsGen2.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmTransOpsGen2_hpp
#define IWLMvmTransOpsGen2_hpp

#include "IWLTransOps.h"

class IWLMvmTransOpsGen2 : public IWLTransOps {
    
public:
    
    IWLMvmTransOpsGen2() {}
    IWLMvmTransOpsGen2(IWLIO *io) : IWLTransOps(io) {}
    ~IWLMvmTransOpsGen2() {}
    
    bool startHW() override;
    
    void fwAlive(UInt32 scd_addr) override;
    
    bool startFW() override;
    
    void stopDevice() override;
    
    void sendCmd(iwl_host_cmd *cmd) override;
    
private:
    
    IWLIO *io;
};

#endif /* IWLMvmTransOpsGen2_hpp */
