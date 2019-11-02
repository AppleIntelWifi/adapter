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
    
    bool init(OSDictionary *properties) override;
    void free() override;
    IOService* probe(IOService* provider, SInt32* score) override;
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
    SInt32 apple80211Request(unsigned int, int, IO80211Interface*, void*) override;
    SInt32 apple80211VirtualRequest(uint,int,IO80211VirtualInterface *,void *) override;
    const OSString* newVendorString() const override;
    const OSString* newModelString() const override;
    IOReturn getHardwareAddressForInterface(IO80211Interface* netif, IOEthernetAddress* addr) override;
    IOReturn getHardwareAddress(IOEthernetAddress* addrP) override;
    IOReturn enable(IONetworkInterface *netif) override;
    IOReturn disable(IONetworkInterface *netif) override;
    bool configureInterface(IONetworkInterface *netif) override;
    IO80211Interface *getNetworkInterface() override;
    IOReturn setPromiscuousMode(bool active) override;
    IOReturn setMulticastMode(bool active) override;
    SInt32 monitorModeSetEnabled(IO80211Interface*, bool, unsigned int) {
        return kIOReturnSuccess;
    }
    bool createWorkLoop() override;
    IOWorkLoop* getWorkLoop() const override;
    int apple80211SkywalkRequest(uint,int, IO80211SkywalkInterface *,void *) override;
    SInt32 disableVirtualInterface(IO80211VirtualInterface *) override;
    SInt32 enableVirtualInterface(IO80211VirtualInterface *) override;
    UInt32 getDataQueueDepth(OSObject *) override;
    SInt32 setVirtualHardwareAddress(IO80211VirtualInterface *,ether_addr *) override;
    IO80211ScanManager* getPrimaryInterfaceScanManager(void) override;
    IO80211SkywalkInterface* getInfraInterface(void) override;
    IO80211ControllerMonitor* getInterfaceMonitor(void) override;
    
    
public:
    
    
private:
    IOPCIDevice *pciDevice;
    
    IO80211WorkLoop *fWorkLoop;
};

#endif
