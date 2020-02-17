//
//  MvmTransOps.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/1.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLTransOps.h"

void IWLTransOps::nicConfig()
{
    u8 radio_cfg_type, radio_cfg_step, radio_cfg_dash;
    u32 reg_val = 0;
    u32 phy_config = iwl_mvm_get_phy_config(trans->m_pDevice);
    
    radio_cfg_type = (phy_config & FW_PHY_CFG_RADIO_TYPE) >>
    FW_PHY_CFG_RADIO_TYPE_POS;
    radio_cfg_step = (phy_config & FW_PHY_CFG_RADIO_STEP) >>
    FW_PHY_CFG_RADIO_STEP_POS;
    radio_cfg_dash = (phy_config & FW_PHY_CFG_RADIO_DASH) >>
    FW_PHY_CFG_RADIO_DASH_POS;
    
    /* SKU control */
    reg_val |= CSR_HW_REV_STEP(trans->m_pDevice->hw_rev) <<
    CSR_HW_IF_CONFIG_REG_POS_MAC_STEP;
    reg_val |= CSR_HW_REV_DASH(trans->m_pDevice->hw_rev) <<
    CSR_HW_IF_CONFIG_REG_POS_MAC_DASH;
    
    /* radio configuration */
    reg_val |= radio_cfg_type << CSR_HW_IF_CONFIG_REG_POS_PHY_TYPE;
    reg_val |= radio_cfg_step << CSR_HW_IF_CONFIG_REG_POS_PHY_STEP;
    reg_val |= radio_cfg_dash << CSR_HW_IF_CONFIG_REG_POS_PHY_DASH;
    
    WARN_ON((radio_cfg_type << CSR_HW_IF_CONFIG_REG_POS_PHY_TYPE) &
            ~CSR_HW_IF_CONFIG_REG_MSK_PHY_TYPE);
    
    /*
     * TODO: Bits 7-8 of CSR in 8000 HW family and higher set the ADC
     * sampling, and shouldn't be set to any non-zero value.
     * The same is supposed to be true of the other HW, but unsetting
     * them (such as the 7260) causes automatic tests to fail on seemingly
     * unrelated errors. Need to further investigate this, but for now
     * we'll separate cases.
     */
    if (trans->m_pDevice->cfg->trans.device_family < IWL_DEVICE_FAMILY_8000)
        reg_val |= CSR_HW_IF_CONFIG_REG_BIT_RADIO_SI;
    
    //TODO now not needed
//    if (iwl_fw_dbg_is_d3_debug_enabled(&mvm->fwrt))
//        reg_val |= CSR_HW_IF_CONFIG_REG_D3_DEBUG;
    
    trans->setBitMask(CSR_HW_IF_CONFIG_REG,
                            CSR_HW_IF_CONFIG_REG_MSK_MAC_DASH |
                            CSR_HW_IF_CONFIG_REG_MSK_MAC_STEP |
                            CSR_HW_IF_CONFIG_REG_MSK_PHY_TYPE |
                            CSR_HW_IF_CONFIG_REG_MSK_PHY_STEP |
                            CSR_HW_IF_CONFIG_REG_MSK_PHY_DASH |
                            CSR_HW_IF_CONFIG_REG_BIT_RADIO_SI |
                            CSR_HW_IF_CONFIG_REG_BIT_MAC_SI   |
                            CSR_HW_IF_CONFIG_REG_D3_DEBUG,
                            reg_val);
    
    IWL_INFO(0, "Radio type=0x%x-0x%x-0x%x\n", radio_cfg_type,
             radio_cfg_step, radio_cfg_dash);
    
    /*
     * W/A : NIC is stuck in a reset state after Early PCIe power off
     * (PCIe power is lost before PERST# is asserted), causing ME FW
     * to lose ownership and not being able to obtain it back.
     */
    if (!trans->m_pDevice->cfg->apmg_not_supported)
        trans->iwlSetBitsMaskPRPH(APMG_PS_CTRL_REG,
                                  APMG_PS_CTRL_EARLY_PWR_OFF_RESET_DIS,
                                  ~APMG_PS_CTRL_EARLY_PWR_OFF_RESET_DIS);
}
