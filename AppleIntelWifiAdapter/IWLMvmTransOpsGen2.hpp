//
//  IWLMvmTransOpsGen2.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/13.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLMvmTransOpsGen2_hpp
#define IWLMvmTransOpsGen2_hpp

#include "IWLTransOps.h"

class IWLMvmTransOpsGen2 : public IWLTransOps {
    
    
public:
    
    bool preparedCardHW() override;
    
    bool startFW() override;
};

#endif /* IWLMvmTransOpsGen2_hpp */
