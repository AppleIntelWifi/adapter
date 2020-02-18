//
//  IWLIO.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLIO_hpp
#define IWLIO_hpp

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <linux/types.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>
#include "../device/IWLDeviceBase.h"
#include "../IWLDevice.hpp"

#define IWL_POLL_INTERVAL 10

/* Max time to wait for nmi interrupt */
#define IWL_TRANS_NMI_TIMEOUT (HZ / 4 * CPTCFG_IWL_TIMEOUT_FACTOR)

class IWLIO {
    
    
public:
    
    bool init(IWLDevice *device);
    
    void release();
    
    bool grabNICAccess(IOInterruptState *state);
    
    void releaseNICAccess(IOInterruptState *state);
    
    int iwlReadMem(u32 addr, void *buf, int dwords);
    
    u32 iwlReadMem32(u32 addr);
    
    int iwlWriteMem(u32 addr, void *buf, int dwords);
    
    u32 iwlWriteMem32(u32 addr, u32 val);
    
    void setPMI(bool state);
    
    void setBitMask(u32 reg, u32 mask, u32 val);
    
    void setBit(u32 reg, u32 mask);
    
    void clearBit(u32 reg, u32 mask);
    
    void iwlWrite8(u32 ofs, u8 val);
    
    void iwlWrite32(u32 ofs, u32 val);
    
    void iwlWrite64(u64 ofs, u64 val);
    
    void iwlWriteDirect32(u32 reg, u32 value);
    
    void iwlWriteDirect64(u64 reg, u64 value);
    
    u32 iwlRead32(u32 ofs);
    
    u32 iwlReadDirect32(u32 reg);
    
    int iwlPollBit(u32 addr,
                   u32 bits, u32 mask, int timeout);
    
    int iwlPollPRPHBit(u32 addr, u32 bits, u32 mask, int timeout);
    
    void iwlSetBitsPRPH(u32 ofs, u32 mask);
    
    void iwlSetBitsMaskPRPH(u32 ofs, u32 bits, u32 mask);
    
    void iwlClearBitsPRPH(u32 ofs, u32 mask);
    
    int iwlPollDirectBit(u32 addr, u32 mask, int timeout);
    
    u32 iwlReadPRPH(u32 ofs);
    
    u32 iwlReadPRPHNoGrab(u32 ofs);
    
    void iwlWritePRPH(u32 addr, u32 val);
    
    void iwlWritePRPHNoGrab(u32 addr, u32 val);
    
    void iwlWritePRPH64NoGrab(u64 ofs, u64 val);
    
    /*
    * UMAC periphery address space changed from 0xA00000 to 0xD00000 starting from
    * device family AX200. So peripheries used in families above and below AX200
    * should go through iwl_..._umac_..._prph.
    */
    u32 iwlUmacPRPH(u32 ofs);
    
    u32 iwlReadUmacPRPHNoGrab(u32 ofs);
    
    u32 iwlReadUmacPRPH(u32 ofs);
    
    void iwlWriteUmacPRPHNoGrab(u32 ofs, u32 val);
    
    void iwlWriteUmacPRPH(u32 ofs, u32 val);
    
    int iwlPollUmacPRPHBit(u32 addr, u32 bits, u32 mask, int timeout);
    
    u32 iwlReadShr(u32 reg);
    
    void iwlWriteShr(u32 reg, u32 val);
    
    void iwlForceNmi();
    
protected:
    
    
private:
    
    void osWriteInt8(uintptr_t byteOffset, uint8_t data);
    
public:
    IWLDevice *m_pDevice;
    
    
private:
    
    IOMemoryMap *fMemMap;
    
    volatile void *fHwBase;
    
};

#endif /* IWLIO_hpp */
