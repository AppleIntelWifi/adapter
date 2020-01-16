//
//  IWLTransport.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLTransport_hpp
#define IWLTransport_hpp

#include "IWLTransOps.h"
#include "IWLIO.hpp"

/**
 * enum iwl_trans_status: transport status flags
 * @STATUS_SYNC_HCMD_ACTIVE: a SYNC command is being processed
 * @STATUS_DEVICE_ENABLED: APM is enabled
 * @STATUS_TPOWER_PMI: the device might be asleep (need to wake it up)
 * @STATUS_INT_ENABLED: interrupts are enabled
 * @STATUS_RFKILL_HW: the actual HW state of the RF-kill switch
 * @STATUS_RFKILL_OPMODE: RF-kill state reported to opmode
 * @STATUS_FW_ERROR: the fw is in error state
 * @STATUS_TRANS_GOING_IDLE: shutting down the trans, only special commands
 *    are sent
 * @STATUS_TRANS_IDLE: the trans is idle - general commands are not to be sent
 * @STATUS_TRANS_DEAD: trans is dead - avoid any read/write operation
 */
enum iwl_trans_status {
    STATUS_SYNC_HCMD_ACTIVE,
    STATUS_DEVICE_ENABLED,
    STATUS_TPOWER_PMI,
    STATUS_INT_ENABLED,
    STATUS_RFKILL_HW,
    STATUS_RFKILL_OPMODE,
    STATUS_FW_ERROR,
    STATUS_TRANS_GOING_IDLE,
    STATUS_TRANS_IDLE,
    STATUS_TRANS_DEAD,
};

class IWLTransport : IWLIO
{
    
public:
    IWLTransport();
    ~IWLTransport();
    
    bool init(IWLDevice *device);
    
    void release();
    
    IWLTransOps *getTransOps();
    
    void setPMI(bool state);
    
private:
    
    
private:
    //a bit-mask of transport status flags
    unsigned long status;
    IWLTransOps *trans_ops;
    
};

#endif /* IWLTransport_hpp */
