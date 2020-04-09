//
//  IWLNode.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 4/7/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#include "IWLNode.hpp"

#define super OSObject
OSDefineMetaClassAndStructors(IWLNode, OSObject);

bool IWLNode::init(IWLCachedScan *scan) {
  scan->retain();

  beacon = scan;
  return true;
}

void IWLNode::free() { beacon->release(); }
