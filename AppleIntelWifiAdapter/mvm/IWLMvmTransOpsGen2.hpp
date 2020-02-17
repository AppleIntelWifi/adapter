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
    IWLMvmTransOpsGen2(IWLTransport *trans) : IWLTransOps(trans) {}
    virtual ~IWLMvmTransOpsGen2() {}
    
    int nicInit() override;
    
    void fwAlive(UInt32 scd_addr) override;
    
    int startFW(const struct fw_img *fw, bool run_in_rfkill) override;
    
    void stopDevice() override;
    
    void stopDeviceDirectly() override;
    
    int apmInit() override;
    
    void apmStop(bool op_mode_leave) override;
    
    int forcePowerGating() override;
    
private:
    int txInit(int txq_id, int queue_size);
    
    int rxInit();
};

#endif /* IWLMvmTransOpsGen2_hpp */
