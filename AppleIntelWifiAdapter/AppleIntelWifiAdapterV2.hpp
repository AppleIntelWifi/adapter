/* add your code here */
#ifndef macroAppleIntelWifiAdapter_hpp
#define AppleIntelWifiAdapter_hpp

#include "apple80211/IO80211Controller.h"
#include "apple80211/IO80211Interface.h"
#include <IOKit/network/IOEthernetController.h>

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>

class AppleIntelWifiAdapterV2 : public IO80211Controller
{
    OSDeclareDefaultStructors( AppleIntelWifiAdapterV2 )
public:
//    AppleIntelWifiAdapterV2();
//    ~AppleIntelWifiAdapterV2();
//    AppleIntelWifiAdapterV2(OSMetaClass const*);
    
    bool init(OSDictionary *properties) override;
    void free() override;
    IOService* probe(IOService* provider, SInt32* score) override;
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
    SInt32 apple80211Request(unsigned int, int, IO80211Interface*, void*);
//    const OSString* newVendorString() const;
//    const OSString* newModelString() const;
//    IOReturn getHardwareAddressForInterface(IO80211Interface* netif, IOEthernetAddress* addr);
    IOReturn getHardwareAddress(IOEthernetAddress* addrP) override;
//    IOReturn enable(IONetworkInterface *netif);
//    IOReturn disable(IONetworkInterface *netif);
//    bool configureInterface(IONetworkInterface *netif);
//    IO80211Interface *getNetworkInterface();
//    IOReturn setPromiscuousMode(bool active);
//    IOReturn setMulticastMode(bool active);
//    SInt32 monitorModeSetEnabled(IO80211Interface*, bool, unsigned int) {
//        return kIOReturnSuccess;
//    }
    bool createWorkLoop() override;
    IOWorkLoop* getWorkLoop() override;
//    UInt32 apple80211SkywalkRequest(uint,int, IO80211SkywalkInterface *,void *);
//    SInt32 disableVirtualInterface(IO80211VirtualInterface*);
//    SInt32 enableVirtualInterface(IO80211VirtualInterface*);
//    UInt32 getDataQueueDepth(OSObject*) override;
//    SInt32 setVirtualHardwareAddress(IO80211VirtualInterface*, ether_addr*);
    IOService* getProvider(void) override;
    IOOutputQueue* getOutputQueue(void) override;
    
public:
//    static AppleIntelWifiAdapterV2* gMetaClass;
    
private:
    IOPCIDevice *pciDevice;
    
    IO80211WorkLoop *fWorkLoop;
};

#endif
