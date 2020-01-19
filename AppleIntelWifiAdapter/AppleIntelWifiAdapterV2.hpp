/* add your code here */
#ifndef macroAppleIntelWifiAdapter_hpp
#define AppleIntelWifiAdapter_hpp

#include "apple80211/IO80211Controller.h"
#include "apple80211/IO80211Interface.h"
#include <IOKit/network/IOEthernetController.h>

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>
#include "IWLMvmDriver.hpp"

class AppleIntelWifiAdapterV2 : public IOEthernetController
{
    OSDeclareDefaultStructors( AppleIntelWifiAdapterV2 )
public:
    
    bool init(OSDictionary *properties) override;
    void free() override;
    IOService* probe(IOService* provider, SInt32* score) override;
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
    IOReturn getHardwareAddress(IOEthernetAddress* addrP) override;
    IOReturn enable(IONetworkInterface *netif) override;
    IOReturn disable(IONetworkInterface *netif) override;
    bool configureInterface(IONetworkInterface *netif) override;
    IOReturn setPromiscuousMode(bool active) override;
    IOReturn setMulticastMode(bool active) override;
    
    
public:
    
    
private:
    IWLMvmDriver *drv;
    
    IOWorkLoop *fWorkLoop;
};

#endif
