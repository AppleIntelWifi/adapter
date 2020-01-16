//
//  IWLTransport.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/6.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransport.hpp"
#include "IWLMvmTransOpsGen2.hpp"
#include "IWLMvmTransOpsGen1.hpp"

#define super IWLIO

IWLTransport::IWLTransport()
{
}

IWLTransport::~IWLTransport()
{
}

bool IWLTransport::init(IWLDevice *device)
{
    if (!super::init(device)) {
        IOLog("IWLTransport init fail\n");
        return false;
    }
    if (m_pDevice->cfg->trans.gen2) {
        trans_ops = new IWLMvmTransOpsGen2();
    } else {
        trans_ops = new IWLMvmTransOpsGen1();
    }
    return true;
}

void IWLTransport::release()
{
    
    super::release();
}

IWLTransOps *IWLTransport::getTransOps()
{
    return trans_ops;
}

void IWLTransport::setPMI(bool state)
{
    if (state) {
        set_bit(STATUS_TPOWER_PMI, &this->status);
    } else {
        clear_bit(STATUS_TPOWER_PMI, &this->status);
    }
}

