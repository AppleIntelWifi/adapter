//
//  IWLTransportRx.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"
#include <IOKit/IOLocks.h>

void IWLTransport::disableICT()
{
    IOSimpleLockLock(this->irq_lock);
    this->use_ict = false;
    IOSimpleLockUnlock(this->irq_lock);
}

void IWLTransport::rxStop()
{
    
}
