/* add your code here */
#ifndef macroAppleIntelWifiAdapter_hpp
#define AppleIntelWifiAdapter_hpp

#include "HackIO80211Interface.h"
#include <IOKit/network/IOEthernetController.h>
#include "IOKit/network/IOGatedOutputQueue.h"
#include <IOKit/IOFilterInterruptEventSource.h>
#include <libkern/c++/OSString.h>

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>
#include "IWLMvmDriver.hpp"

OSDefineMetaClassAndStructors(CTimeout, OSObject)

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
    IOReturn setPromiscuousMode(bool active) override;
    IOReturn setMulticastMode(bool active) override;
    IOOutputQueue * createOutputQueue() override;
    UInt32 outputPacket(mbuf_t, void * param) override;
    int intrOccured(OSObject *object, IOInterruptEventSource *, int count);
    bool intrFilter(OSObject *object, IOFilterInterruptEventSource *src);
    IONetworkInterface * createInterface() override;
    bool configureInterface(IONetworkInterface * interface) override;
    
public:
    
    
private:
    IWLMvmDriver *drv;
    IOGatedOutputQueue*    fOutputQueue;
    IOInterruptEventSource* fInterrupt;
    IOEthernetInterface *netif;
    IOCommandGate *gate;
};

#endif
