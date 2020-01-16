//
//  IWLTransOps.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLTransOps_h
#define IWLTransOps_h

class IWLTransOps {
    
    
public:
    
    virtual bool preparedCardHW();
    
    virtual bool startFW();
};

#endif /* IWLTransOps_h */
