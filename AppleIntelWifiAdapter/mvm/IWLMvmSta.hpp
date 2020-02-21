//
//  IWLMvmSta.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 2/19/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmSta_hpp
#define IWLMvmSta_hpp

#include "../trans/IWLTransport.hpp"
#include "../mvm/IWLMvmDriver.hpp"

/*
 * New version of ADD_STA_sta command added new fields at the end of the
 * structure, so sending the size of the relevant API's structure is enough to
 * support both API versions.
 */
static inline int iwl_mvm_add_sta_cmd_size(IWLDevice *mvm)
{
    if (iwl_mvm_has_new_rx_api(mvm) ||
        fw_has_api(&mvm->fw.ucode_capa, IWL_UCODE_TLV_API_STA_TYPE))
        return sizeof(struct iwl_mvm_add_sta_cmd);
    else
        return sizeof(struct iwl_mvm_add_sta_cmd_v7);
}

int iwl_mvm_add_aux_sta(IWLMvmDriver* drv);

int iwl_mvm_sta_send_to_fw(IWLMvmDriver* drv, struct iwm_node *in, bool update, unsigned int flags);

#endif /* IWLMvmSta_hpp */
