//
//  IWLApple80211.cpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 3/18/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "IWLApple80211.hpp"

bool IWL80211Device::init(IWLMvmDriver* drv) {
    
    fDrv = drv;
    
    scanCacheLock = IOLockAlloc();
    scanCache = OSSet::withCapacity(50);
    scanCacheIterator = OSCollectionIterator::withCollection(scanCache);
    
    iface = fDrv->controller->getNetworkInterface();
    
    return true;
}

bool IWL80211Device::release() {
    
    if(scanCacheLock) {
        IOLockFree(scanCacheLock);
        scanCacheLock = NULL;
    }
    
    if(scanCache) {
        scanCache->release();
        scanCache = NULL;
    }
    
    if(scanCacheIterator) {
        scanCacheIterator->free();
        scanCacheIterator = NULL;
    }
    return true;
}

bool IWL80211Device::scanDone() {
    if(iface != NULL) {
        iface->postMessage(APPLE80211_M_SCAN_DONE);
        return true;
    } else {
        return false;
    }
}



