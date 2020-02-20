//
//  IWLCoex.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/8.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLMvmDriver.hpp"

int IWLMvmDriver::sendBTInitConf()
{
    iwl_bt_coex_cmd cmd;
    u32 mode;
    IOLockLock(trans->mutex);
    
    if(unlikely(m_pDevice->btForceAntMode != BT_FORCE_ANT_DIS))
    {
        switch(m_pDevice->btForceAntMode) {
            case BT_FORCE_ANT_BT:
                mode = BT_COEX_BT;
                break;
            case BT_FORCE_ANT_WIFI:
                mode = BT_COEX_WIFI;
                break;
            default:
                WARN_ON(1);
                mode = 0;
                break;
        }
    }
    else {
        mode = m_pDevice->iwlwifi_mod_params.bt_coex_active ? BT_COEX_NW : BT_COEX_DISABLE;
        //  if(IWL_MVM_BT_COEX_SYNC2SCO)
    }
    cmd.mode = cpu_to_le32(BT_COEX_WIFI);
    //cmd.enabled_modules |= cpu_to_le32(BT_COEX_SYNC2SCO_ENABLED);
    
    /*
    if(iwl_mvm_is_mplut_supported(m_pDevice)) {
        cmd.enabled_modules |= cpu_to_le32(BT_COEX_MPLUT_ENABLED);
    }
    */
    cmd.enabled_modules = cpu_to_le32(BT_COEX_HIGH_BAND_RET);

    
    memset(&m_pDevice->lastBtNotif, 0, sizeof(m_pDevice->lastBtNotif));
    memset(&m_pDevice->lastBtCiCmd, 0, sizeof(m_pDevice->lastBtCiCmd));
    
    IOLockUnlock(trans->mutex);

    return sendCmdPdu(BT_CONFIG, 0, sizeof(cmd), &cmd);
}
