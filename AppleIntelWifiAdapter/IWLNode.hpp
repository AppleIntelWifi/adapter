//
//  IWLNode.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 4/7/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#ifndef APPLEINTELWIFIADAPTER_IWLNODE_HPP_
#define APPLEINTELWIFIADAPTER_IWLNODE_HPP_

#include <libkern/c++/OSObject.h>

#include "IWLCachedScan.hpp"
#include "fw/api/phy-ctxt.h"

class IWLNode : public OSObject {
  OSDeclareDefaultStructors(IWLNode);

 public:
  bool init(IWLCachedScan* scan);
  void free() override;

  inline IWLCachedScan* getBeacon() { return this->beacon; }

  inline iwl_phy_ctx* getPhyCtx() { return this->phy_ctx; }

  inline void resetPhyCtx() { this->phy_ctx = NULL; }

  inline void setPhyCtx(iwl_phy_ctx* ctx) {
    if (!ctx) return;

    phy_ctx = ctx;
  }

  inline uint32_t getID() { return id; }

  inline uint32_t getColor() { return color; }

 private:
  IWLCachedScan* beacon;
  iwl_phy_ctx* phy_ctx;
  uint32_t id;
  uint32_t color;
};
#endif  // APPLEINTELWIFIADAPTER_IWLNODE_HPP_
