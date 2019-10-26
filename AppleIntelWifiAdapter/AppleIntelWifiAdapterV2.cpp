/* add your code here */
#include "AppleIntelWifiAdapterV2.hpp"

#include <IOKit/IOInterruptController.h>
#include <IOKit/IOCommandGate.h>

#define super IO80211Controller

OSDefineMetaClassAndStructors(AppleIntelWifiAdapterV2, IO80211Controller)

//AppleIntelWifiAdapterV2::AppleIntelWifiAdapterV2()
//{
//
//}
//
//
//AppleIntelWifiAdapterV2::~AppleIntelWifiAdapterV2()
//{
//
//}
//
//AppleIntelWifiAdapterV2::AppleIntelWifiAdapterV2(OSMetaClass const* metaclass)
//{
//
//}

void AppleIntelWifiAdapterV2::free() {
    super::free();
    IOLog("Driver free()");
}

bool AppleIntelWifiAdapterV2::init(OSDictionary *properties)
{
    super::init(properties);
    IOLog("Driver init()");
    return true;
}

IOService* AppleIntelWifiAdapterV2::probe(IOService *provider, SInt32 *score)
{
    IOLog("Driver Probe()");
    
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (pciDevice == nullptr) {
        IOLog("Not pci device");
        return NULL;
    }
    UInt16 vendorID = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceID = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subSystemVendorID = pciDevice->configRead16(kIOPCIConfigSubSystemVendorID);
    UInt16 subSystemDeviceID = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    UInt8 revision = pciDevice->configRead8(kIOPCIConfigRevisionID);
    IOLog("find pci device====>vendorID=0x%04x, deviceID=0x%04x, subSystemVendorID=0x%04x, subSystemDeviceID=0x%04x, revision=0x%02x", vendorID, deviceID, subSystemVendorID, subSystemDeviceID, revision);
    return NULL;
}

bool AppleIntelWifiAdapterV2::start(IOService *provider)
{
    IOLog("Driver Start()");
    super::start(provider);
    return false;
}

void AppleIntelWifiAdapterV2::stop(IOService *provider)
{
    IOLog("Driver Stop()");
    super::stop(provider);
}

SInt32 AppleIntelWifiAdapterV2::apple80211Request(unsigned int, int, IO80211Interface*, void*)
{
    
    return kIOReturnSuccess;
}

//
//IOReturn AppleIntelWifiAdapterV2::enable(IONetworkInterface *netif)
//{
//    IOLog("Driver Enable()");
//    return kIOReturnSuccess;
//}
//
//IOReturn AppleIntelWifiAdapterV2::disable(IONetworkInterface *netif)
//{
//    IOLog("Driver Disable()");
//    return kIOReturnSuccess;
//}
//

//
//IOReturn AppleIntelWifiAdapterV2::getHardwareAddressForInterface(IO80211Interface *netif, IOEthernetAddress *addr)
//{
//    return kIOReturnSuccess;
//}
//
//IOReturn AppleIntelWifiAdapterV2::setPromiscuousMode(bool active)
//{
//    return kIOReturnSuccess;
//}
//
//IOReturn AppleIntelWifiAdapterV2::setMulticastMode(bool active)
//{
//    return kIOReturnSuccess;
//}
//
//bool AppleIntelWifiAdapterV2::configureInterface(IONetworkInterface *netif)
//{
//    return kIOReturnSuccess;
//}
//
//IO80211Interface* AppleIntelWifiAdapterV2::getNetworkInterface()
//{
//    return NULL;
//}
//
//const OSString* AppleIntelWifiAdapterV2::newVendorString() const
//{
//    return OSString::withCString("Intel");
//}
//
//const OSString* AppleIntelWifiAdapterV2::newModelString() const
//{
//    return OSString::withCString("test model");
//}
//
//UInt32 AppleIntelWifiAdapterV2::apple80211SkywalkRequest(uint,int, IO80211SkywalkInterface *,void *)
//{
//    return 0;
//}
//
//SInt32 AppleIntelWifiAdapterV2::disableVirtualInterface(IO80211VirtualInterface *)
//{
//    return 0;
//}
//
//SInt32 AppleIntelWifiAdapterV2::enableVirtualInterface(IO80211VirtualInterface *)
//{
//    return 0;
//}
//
//UInt32 AppleIntelWifiAdapterV2::getDataQueueDepth(OSObject *)
//{
//    return 1024;
//}
//
//SInt32 AppleIntelWifiAdapterV2::setVirtualHardwareAddress(IO80211VirtualInterface *, ether_addr *)
//{
//    return 0;
//}
