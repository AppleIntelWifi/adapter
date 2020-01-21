//
//  IWLUcodeParse.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLUcodeParse_hpp
#define IWLUcodeParse_hpp

#include <linux/types.h>

#include "FWFile.h"
#include "FWImg.h"
#include "IWLDevice.hpp"

class IWLUcodeParse {
    
public:
    
    IWLUcodeParse(IWLDevice *drv);
    ~IWLUcodeParse();
    
    bool parseFW(const void *raw, size_t len, struct iwl_fw *fw, struct iwl_firmware_pieces *pieces);
    
    bool parseV1V2Firmware(const void *raw, size_t len, struct iwl_fw *fw, struct iwl_firmware_pieces *pieces);

    bool parseTLVFirmware(const void *raw, size_t len, struct iwl_fw *fw, struct iwl_firmware_pieces *pieces);
    
    /*
    * These functions are just to extract uCode section data from the pieces
    * structure.
    */
    struct fw_sec *getSec(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec);
    
    void alloccSecData(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec);
    
    void setSecSize(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec, size_t size);
    
    void setSecOffset(iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec, u32 offset);
    
    size_t getSecSize(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec);
    
    void setSecData(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type, int sec, const void *data);
    
    int storeUcodeSec(struct iwl_firmware_pieces *pieces, const void *data, enum iwl_ucode_type type, int size);
    
    void setUcodeApiFlags(const u8 *data, struct iwl_ucode_capabilities *capa);
    
    void setUcodeCapabilities(const u8 *data, struct iwl_ucode_capabilities *capa);
    
    int setDefaultCalib(const u8 *data);
    
    int storeCscheme(struct iwl_fw *fw, const u8 *data, const u32 len);
    
    const char *reducedFWName(const char *name);
    
    int allocUcode(struct iwl_firmware_pieces *pieces, enum iwl_ucode_type type);
    
    int allocFWDesc(struct fw_desc *desc, struct fw_sec *sec);
    
    void freeFWDesc(struct fw_desc *desc);
    
    void freeFWImg(struct fw_img *img);
    
    void deAllocUcode();
    
private:
    
    IWLDevice *drv;
};

#endif /* IWLUcodeParse_hpp */
