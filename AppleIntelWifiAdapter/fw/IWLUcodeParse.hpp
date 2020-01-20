//
//  IWLUcodeParse.hpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/20.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef IWLUcodeParse_hpp
#define IWLUcodeParse_hpp

class IWLUcodeParse {
    
public:
    
    static bool parseFW(const void *raw);
    
private:
    
    static bool parseV1V2Firmware(const void *raw);

    static bool parseTLVFirmware(const void *raw);
};

#endif /* IWLUcodeParse_hpp */
