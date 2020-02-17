//
//  IWLIO.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLIO.hpp"

/* PCI registers */
#define PCI_CFG_RETRY_TIMEOUT    0x041

bool IWLIO::init(IWLDevice *device)
{
    this->m_pDevice = device;
    //init pci
    m_pDevice->enablePCI();
    fMemMap = m_pDevice->pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!fMemMap) {
        IWL_ERR(0, "map memory fail\n");
        return false;
    }
    fHwBase = reinterpret_cast<volatile void *>(fMemMap->getVirtualAddress());
    if (!fHwBase) {
        IWL_ERR(0, "map fHwBase fail\n");
        return false;
    }
    /* We disable the RETRY_TIMEOUT register (0x41) to keep
     * PCI Tx retries from interfering with C3 CPU state */
    m_pDevice->pciDevice->configWrite8(PCI_CFG_RETRY_TIMEOUT, 0x00);
    return true;
}

void IWLIO::release()
{
    OSSafeReleaseNULL(fMemMap);
}

bool IWLIO::grabNICAccess(IOInterruptState *state)
{
    int ret;
    *state = IOSimpleLockLockDisableInterrupt(this->m_pDevice->registerRWLock);
    if (this->m_pDevice->holdNICWake) {
        return true;
    }
    setBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
    if (m_pDevice->cfg->trans.device_family >= IWL_DEVICE_FAMILY_8000) {
        IODelay(2);
    }
    ret = iwlPollBit(CSR_GP_CNTRL,
                     CSR_GP_CNTRL_REG_VAL_MAC_ACCESS_EN,
                     (CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY |
                      CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP), 15000);
    if (ret < 0) {
        u32 cntrl = iwlRead32(CSR_GP_CNTRL);
        IWL_WARN(0, "Timeout waiting for hardware access (CSR_GP_CNTRL 0x%08x)\n",
                 cntrl);
        //TODO iwl_trans_pcie_dump_regs now ignore it
        //iwl_trans_pcie_dump_regs
        if (cntrl == ~0U) {
            IWL_ERR(trans, "Device gone - scheduling removal!\n");
        } else {
            iwlWrite32(CSR_RESET,
                       CSR_RESET_REG_FLAG_FORCE_NMI);
        }
        IOSimpleLockUnlockEnableInterrupt(this->m_pDevice->registerRWLock, *state);
        return false;
    }
    return true;
}

void IWLIO::releaseNICAccess(IOInterruptState *state)
{
    if (this->m_pDevice->holdNICWake) {
        goto out;
    }
    clearBit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
out:
    IOSimpleLockUnlockEnableInterrupt(this->m_pDevice->registerRWLock, *state);
}

int IWLIO::iwlReadMem(u32 addr, void *buf, int dwords)
{
    IOInterruptState flags;
    int offs, ret = 0;
    u32 *vals = (u32 *)buf;
    
    if (grabNICAccess(&flags)) {
        iwlWrite32(HBUS_TARG_MEM_RADDR, addr);
        for (offs = 0; offs < dwords; offs++)
            vals[offs] = iwlRead32(HBUS_TARG_MEM_RDAT);
        releaseNICAccess(&flags);
    } else {
        ret = -EBUSY;
    }
    return ret;
}

u32 IWLIO::iwlReadMem32(u32 addr)
{
    u32 value;
    if (WARN_ON(iwlReadMem(addr, &value, 1)))
        return 0xa5a5a5a5;
    return value;
}

int IWLIO::iwlWriteMem(u32 addr, void *buf, int dwords)
{
    IOInterruptState flags;
    int offs, ret = 0;
    const u32 *vals = (u32 *)buf;
    if (grabNICAccess(&flags)) {
        iwlWrite32(HBUS_TARG_MEM_WADDR, addr);
        for (offs = 0; offs < dwords; offs++)
            iwlWrite32(HBUS_TARG_MEM_WDAT,
                        vals ? vals[offs] : 0);
        releaseNICAccess(&flags);
    } else {
        ret = -EBUSY;
    }
    return ret;
}

u32 IWLIO::iwlWriteMem32(u32 addr, u32 val)
{
    return iwlWriteMem(addr, &val, 1);
}

void IWLIO::setBitMask(u32 reg, u32 mask, u32 val)
{
//    IOInterruptState state = IOSimpleLockLockDisableInterrupt(this->m_pDevice->registerRWLock);
    u32 uV;
    uV = iwlRead32(reg);
    uV &= ~mask;
    uV |= val;
    iwlWrite32(reg, uV);
//    IOSimpleLockUnlockEnableInterrupt(this->m_pDevice->registerRWLock, state);
}

void IWLIO::setBit(u32 reg, u32 mask)
{
    setBitMask(reg, mask, mask);
}

void IWLIO::clearBit(u32 reg, u32 mask)
{
    setBitMask(reg, mask, 0);
}

void IWLIO::osWriteInt8(uintptr_t byteOffset, uint8_t data)
{
    *(volatile uint8_t *)((uintptr_t)fHwBase + byteOffset) = data;
}

void IWLIO::iwlWrite8(u32 ofs, u8 val)
{
    osWriteInt8(ofs, val);
}

void IWLIO::iwlWrite32(u32 ofs, u32 val)
{
    _OSWriteInt32(fHwBase, ofs, val);
}

void IWLIO::iwlWrite64(u64 ofs, u64 val)
{
    _OSWriteInt64(fHwBase, ofs, val);
}

void IWLIO::iwlWriteDirect32(u32 reg, u32 value)
{
    IOInterruptState flags;
    if (grabNICAccess(&flags)) {
        iwlWrite32(reg, value);
        releaseNICAccess(&flags);
    }
}

void IWLIO::iwlWriteDirect64(u64 reg, u64 value)
{
    IOInterruptState flags;
    if (grabNICAccess(&flags)) {
        iwlWrite64(reg, value);
        releaseNICAccess(&flags);
    }
}

u32 IWLIO::iwlRead32(u32 ofs)
{
    return _OSReadInt32(fHwBase, ofs);
}

u32 IWLIO::iwlReadDirect32(u32 reg)
{
    u32 value = 0x5a5a5a5a;
    IOInterruptState flags;
    if (grabNICAccess(&flags)) {
        value = iwlRead32(reg);
        releaseNICAccess(&flags);
    }
    return value;
}

int IWLIO::iwlPollBit(u32 addr,
                      u32 bits, u32 mask, int timeout)
{
    int t = 0;
    do {
        if ((iwlRead32(addr) & mask) == (bits & mask))
            return t;
        IODelay(IWL_POLL_INTERVAL);
        t += IWL_POLL_INTERVAL;
    } while (t < timeout);
    
    return -ETIMEDOUT;
}

int IWLIO::iwlPollPRPHBit(u32 addr, u32 bits, u32 mask, int timeout)
{
    int t = 0;
    do {
        if ((iwlReadPRPH(addr) & mask) == mask)
            return t;
        IODelay(IWL_POLL_INTERVAL);
        t += IWL_POLL_INTERVAL;
    } while (t < timeout);

    return -ETIMEDOUT;
}

void IWLIO::iwlSetBitsPRPH(u32 ofs, u32 mask)
{
    IOInterruptState flags;
    if (grabNICAccess(&flags)) {
        iwlWritePRPHNoGrab(ofs, iwlReadPRPHNoGrab(ofs) | mask);
        releaseNICAccess(&flags);
    }
}

void IWLIO::iwlSetBitsMaskPRPH(u32 ofs, u32 bits, u32 mask)
{
    IOInterruptState flags;
    if (grabNICAccess(&flags)) {
        iwlWritePRPHNoGrab(ofs, (iwlReadPRPHNoGrab(ofs) & mask) | bits);
        releaseNICAccess(&flags);
    }
}

void IWLIO::iwlClearBitsPRPH(u32 ofs, u32 mask)
{
    IOInterruptState flags;
    u32 val;
    if (grabNICAccess(&flags)) {
        val = iwlReadPRPHNoGrab(ofs);
        iwlWritePRPHNoGrab(ofs, (val & ~mask));
        releaseNICAccess(&flags);
    }
}

int IWLIO::iwlPollDirectBit(u32 addr, u32 mask, int timeout)
{
    int t = 0;
    do {
        if ((iwlReadDirect32(addr) & mask) == mask)
            return t;
        IODelay(IWL_POLL_INTERVAL);
        t += IWL_POLL_INTERVAL;
    } while (t < timeout);

    return -ETIMEDOUT;
}

u32 IWLIO::iwlReadPRPH(u32 ofs)
{
    IOInterruptState flags;
    u32 val = 0x5a5a5a5a;

    if (grabNICAccess(&flags)) {
        val = iwlReadPRPHNoGrab(ofs);
        releaseNICAccess(&flags);
    }
    return val;
}

u32 IWLIO::iwlReadPRPHNoGrab(u32 ofs)
{
    iwlWrite32(HBUS_TARG_PRPH_RADDR, ((ofs & 0x000FFFFF) | (3 << 24)));
    return iwlRead32(HBUS_TARG_PRPH_RDAT);
}

void IWLIO::iwlWritePRPH(u32 addr, u32 val)
{
    iwlWrite32(HBUS_TARG_PRPH_WADDR, ((addr & 0x000FFFFF) | (3 << 24)));
    iwlWrite32(HBUS_TARG_PRPH_WDAT, val);
}

void IWLIO::iwlWritePRPHNoGrab(u32 addr, u32 val)
{
    iwlWritePRPH(addr, val);
}

void IWLIO::iwlWritePRPH64NoGrab(u64 ofs, u64 val)
{
    iwlWritePRPHNoGrab((u32)ofs, val & 0xffffffff);
    iwlWritePRPHNoGrab((u32)ofs + 4, val >> 32);
}

u32 IWLIO::iwlUmacPRPH(u32 ofs)
{
    return ofs + m_pDevice->cfg->trans.umac_prph_offset;
}

u32 IWLIO::iwlReadUmacPRPHNoGrab(u32 ofs)
{
    return iwlReadPRPHNoGrab(iwlUmacPRPH(ofs));
}

u32 IWLIO::iwlReadUmacPRPH(u32 ofs)
{
    return iwlReadPRPH(iwlUmacPRPH(ofs));
}

void IWLIO::iwlWriteUmacPRPHNoGrab(u32 ofs, u32 val)
{
    iwlWritePRPHNoGrab(iwlUmacPRPH(ofs), val);
}

void IWLIO::iwlWriteUmacPRPH(u32 ofs, u32 val)
{
    iwlWritePRPH(iwlUmacPRPH(ofs), val);
}

int IWLIO::iwlPollUmacPRPHBit(u32 addr, u32 bits, u32 mask, int timeout)
{
    return iwlPollPRPHBit(iwlUmacPRPH(addr), bits, mask, timeout);
}

u32 IWLIO::iwlReadShr(u32 reg)
{
    iwlWrite32(HEEP_CTRL_WRD_PCIEX_CTRL_REG, ((reg & 0x0000ffff) | (2 << 28)));
    return iwlRead32(HEEP_CTRL_WRD_PCIEX_DATA_REG);
}

void IWLIO::iwlWriteShr(u32 reg, u32 val)
{
    iwlWrite32(HEEP_CTRL_WRD_PCIEX_DATA_REG, val);
    iwlWrite32(HEEP_CTRL_WRD_PCIEX_CTRL_REG, ((reg & 0x0000ffff) | (3 << 28)));
}

void IWLIO::iwlForceNmi()
{
    if (m_pDevice->cfg->trans.device_family < IWL_DEVICE_FAMILY_9000)
        iwlWritePRPH(DEVICE_SET_NMI_REG,
                   DEVICE_SET_NMI_VAL_DRV);
    else if (m_pDevice->cfg->trans.device_family < IWL_DEVICE_FAMILY_AX210)
        iwlWriteUmacPRPH(UREG_NIC_SET_NMI_DRIVER,
                UREG_NIC_SET_NMI_DRIVER_NMI_FROM_DRIVER_MSK);
    else
        iwlWriteUmacPRPH(UREG_DOORBELL_TO_ISR6,
                    UREG_DOORBELL_TO_ISR6_NMI_BIT);
}
