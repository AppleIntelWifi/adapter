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
    int t = 0;
    int ret = setHWReady();
    /* If the card is ready, exit 0 */
    if (ret >= 0)
        return true;
    this->io->setBit(CSR_DBG_LINK_PWR_MGMT_REG, CSR_RESET_LINK_PWR_MGMT_DISABLED);
    IODelay(2000);
    for (int iter = 0; iter < 10; iter++) {
        /* If HW is not ready, prepare the conditions to check again */
        this->io->setBit(CSR_HW_IF_CONFIG_REG,
                         CSR_HW_IF_CONFIG_REG_PREPARE);
        do {
            ret = setHWReady();
            if (ret >= 0)
                return 0;
            
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
    return 0;
}

void IWLTransOps::loadFWChunkFh(u32 dst_addr, dma_addr_t phy_addr, u32 byte_cnt)
{
    this->io->iwlWrite32(FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL), FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_PAUSE);
    this->io->iwlWrite32(FH_SRVC_CHNL_SRAM_ADDR_REG(FH_SRVC_CHNL),
                         dst_addr);
    
    this->io->iwlWrite32(FH_TFDIB_CTRL0_REG(FH_SRVC_CHNL),
                         phy_addr & FH_MEM_TFDIB_DRAM_ADDR_LSB_MSK);
    
    this->io->iwlWrite32(FH_TFDIB_CTRL1_REG(FH_SRVC_CHNL),
                         (iwl_get_dma_hi_addr(phy_addr)
                          << FH_MEM_TFDIB_REG1_ADDR_BITSHIFT) | byte_cnt);
    
    this->io->iwlWrite32(FH_TCSR_CHNL_TX_BUF_STS_REG(FH_SRVC_CHNL),
                         BIT(FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_NUM) |
                         BIT(FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_IDX) |
                         FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_VALID);
    
    this->io->iwlWrite32(FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL),
                                    FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE |
                                    FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_DISABLE |
                                    FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_ENDTFD);
}

int IWLTransOps::loadFWChunk(u32 dst_addr, dma_addr_t phy_addr, u32 byte_cnt)
{
    
    return 0;
}


bool IWLTransOps::apmInit()
{
    
    return true;
}


void IWLTransOps::apmStop()
{
    
}
