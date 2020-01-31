//
//  IWLTransOpsBase.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/19.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransOps.h"
#include "IWLDebug.h"

bool IWLTransOps::prepareCardHW()
{
    IWL_INFO(0, "prepareCardHW\n");
    int t = 0;
    int ret = setHWReady();
    /* If the card is ready, exit 0 */
    if (ret >= 0)
        return false;
    this->io->setBit(CSR_DBG_LINK_PWR_MGMT_REG, CSR_RESET_LINK_PWR_MGMT_DISABLED);
    IODelay(2000);
    for (int iter = 0; iter < 10; iter++) {
        /* If HW is not ready, prepare the conditions to check again */
        this->io->setBit(CSR_HW_IF_CONFIG_REG,
                         CSR_HW_IF_CONFIG_REG_PREPARE);
        do {
            ret = setHWReady();
            if (ret >= 0)
                return false;
            
            IODelay(1000);
            t += 200;
        } while (t < 150000);
        IODelay(25);
    }
    IWL_ERR(0, "Couldn't prepare the card\n");
    return true;
}

#define HW_READY_TIMEOUT (50)

int IWLTransOps::setHWReady()
{
    int ret;
    this->io->setBit(CSR_HW_IF_CONFIG_REG,
                     CSR_HW_IF_CONFIG_REG_BIT_NIC_READY);
    /* See if we got it */
    ret = this->io->iwlPollBit(CSR_HW_IF_CONFIG_REG,
                               CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
                               CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
                               HW_READY_TIMEOUT);
    if (ret >= 0)
        this->io->setBit(CSR_MBOX_SET_REG, CSR_MBOX_SET_REG_OS_ALIVE);
    IWL_WARN(0, "hardware%s ready\n", ret < 0 ? " not" : "");
    return ret;
}

bool IWLTransOps::apmInit()
{
    
    return true;
}


void IWLTransOps::apmStop()
{
    
}
