//
//  IWLInternal.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/6.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#ifndef APPLEINTELWIFIADAPTER_IWLINTERNAL_HPP_
#define APPLEINTELWIFIADAPTER_IWLINTERNAL_HPP_

#include <linux/types.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>

struct iwl_dma_ptr {
    dma_addr_t dma;
    void *addr;
    size_t size;
    
    void *bmd; // IOBufferMemoryDescriptor
    void *cmd; // IODMACommand
};

struct iwl_dma_ptr* allocate_dma_buf(size_t size, mach_vm_address_t physical_mask);
struct iwl_dma_ptr* allocate_dma_buf32(size_t size);

void free_dma_buf(struct iwl_dma_ptr *dma_ptr);

#endif  // APPLEINTELWIFIADAPTER_IWLINTERNAL_HPP_
