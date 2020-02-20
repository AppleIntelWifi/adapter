//
//  IWLTransOps.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLTransOps_h
#define IWLTransOps_h

#include <linux/types.h>
#include "IWLTransport.hpp"
#include "IWLDevice.hpp"
#include "Mvm.h"

class IWLTransOps {
    
public:
    
    IWLTransOps() {}
    IWLTransOps(IWLTransport *trans);
    virtual ~IWLTransOps() {}
    
    void nicConfig();
    
    int startHW();
    
    bool setHWRFKillState(bool state);
    
    void setRfKillState(bool state);//iwl_trans_pcie_rf_kill
    
    void handleStopRFKill(bool was_in_rfkill);
    
    void irqRfKillHandle();//iwl_pcie_handle_rfkill_irq
    
    bool checkHWRFKill();
    
    bool fwRunning();
    
    virtual int nicInit() = 0;

    /*
    * Start up NIC's basic functionality after it has been reset
    * (e.g. after platform boot, or shutdown via iwl_pcie_apm_stop())
    * NOTE:  This does not load uCode nor start the embedded processor
    */
    virtual int apmInit() = 0;
    
    virtual void apmStop(bool op_mode_leave) = 0;
    
    virtual void fwAlive(UInt32 scd_addr) = 0;
    
    virtual int startFW(const struct fw_img *fw, bool run_in_rfkill) = 0;
    
    virtual void stopDevice() = 0; //iwl_trans_pcie_stop_device and iwl_trans_pcie_gen2_stop_device
    
    virtual void stopDeviceDirectly() = 0;//_iwl_trans_pcie_gen2_stop_device and _iwl_trans_pcie_stop_device
    
    virtual int forcePowerGating() = 0;
    
    
private:
    
    
public:
    IWLTransport *trans;
    
    
private:
    
};

#endif /* IWLTransOps_h */
