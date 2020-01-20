//
//  IWLUcodeParse.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLUcodeParse.hpp"
#include "FWFile.h"


bool IWLUcodeParse::parseFW(const void *raw)
{
    /* Data from ucode file:  header followed by uCode images */
    iwl_ucode_header *ucode = (struct iwl_ucode_header*)raw;
    if (ucode->ver) {
        return parseV1V2Firmware(raw);
    } else {
        return parseTLVFirmware(raw);
    }
    return false;
}

bool IWLUcodeParse::parseV1V2Firmware(const void *raw)
{
    
    return true;
}

bool IWLUcodeParse::parseTLVFirmware(const void *raw)
{
    
    return false;
}
