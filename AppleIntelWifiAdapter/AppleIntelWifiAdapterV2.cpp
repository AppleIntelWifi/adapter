/* add your code here */
#include "AppleIntelWifiAdapterV2.hpp"

#include <IOKit/IOInterruptController.h>
#include <IOKit/IOCommandGate.h>

#define super IOEthernetController

OSDefineMetaClassAndStructors(AppleIntelWifiAdapterV2, IOEthernetController)


void AppleIntelWifiAdapterV2::free() {
    super::free();
    OSSafeReleaseNULL(m_pDevice);
    IOLog("Driver free()");
}

bool AppleIntelWifiAdapterV2::init(OSDictionary *properties)
{
    IOLog("Driver init()");
    m_pDevice = new IWLDevice();
    m_pDevice->init();
    return super::init(properties);
}

IOService* AppleIntelWifiAdapterV2::probe(IOService *provider, SInt32 *score)
{
    IOLog("Driver Probe()");
    super::probe(provider, score);
    
    IOPCIDevice *pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        IOLog("Not pci device");
        return NULL;	
    }
    UInt16 vendorID = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceID = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subSystemVendorID = pciDevice->configRead16(kIOPCIConfigSubSystemVendorID);
    UInt16 subSystemDeviceID = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    UInt8 revision = pciDevice->configRead8(kIOPCIConfigRevisionID);
    IOLog("find pci device====>vendorID=0x%04x, deviceID=0x%04x, subSystemVendorID=0x%04x, subSystemDeviceID=0x%04x, revision=0x%02x", vendorID, deviceID, subSystemVendorID, subSystemDeviceID, revision);
    int s = m_pDevice->probe(pciDevice);
    IOLog("%d", s);
    return NULL;
}

bool AppleIntelWifiAdapterV2::start(IOService *provider)
{
    IOLog("Driver Start()");
    if (!super::start(provider)) {
        return false;
    }
    return true;
}

void AppleIntelWifiAdapterV2::stop(IOService *provider)
{
    IOLog("Driver Stop()");
    super::stop(provider);
}

IOReturn AppleIntelWifiAdapterV2::enable(IONetworkInterface *netif)
{
    IOLog("Driver Enable()");
    return super::enable(netif);
}

IOReturn AppleIntelWifiAdapterV2::disable(IONetworkInterface *netif)
{
    IOLog("Driver Disable()");
    return super::enable(netif);
}

IOReturn AppleIntelWifiAdapterV2::setPromiscuousMode(bool active)
{
    return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setMulticastMode(bool active)
{
    return kIOReturnSuccess;
}

bool AppleIntelWifiAdapterV2::configureInterface(IONetworkInterface *netif)
{
    return kIOReturnSuccess;
}

bool AppleIntelWifiAdapterV2::createWorkLoop() {
    if (!fWorkLoop) {
        fWorkLoop = IOWorkLoop::workLoop();
    }
    return (fWorkLoop != NULL);
}

IOWorkLoop* AppleIntelWifiAdapterV2::getWorkLoop() const {
    return fWorkLoop;
}

IOReturn AppleIntelWifiAdapterV2::getHardwareAddress(IOEthernetAddress *addrP) {
//    memcpy(addrP->bytes, &hw->wiphy->addresses[0], ETHER_ADDR_LEN);
    return kIOReturnSuccess;
}
