/* add your code here */
#include "AppleIntelWifiAdapterV2.hpp"

#include <IOKit/IOInterruptController.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/network/IONetworkMedium.h>
#include "IWLDebug.h"

#define super IOEthernetController

OSDefineMetaClassAndStructors(AppleIntelWifiAdapterV2, IOEthernetController)
OSDefineMetaClassAndStructors(HackIO80211Interface, IOEthernetInterface)

enum
{
    MEDIUM_INDEX_AUTO = 0,
    MEDIUM_INDEX_10HD,
    MEDIUM_INDEX_10FD,
    MEDIUM_INDEX_100HD,
    MEDIUM_INDEX_100FD,
    MEDIUM_INDEX_100FDFC,
    MEDIUM_INDEX_1000FD,
    MEDIUM_INDEX_1000FDFC,
    MEDIUM_INDEX_1000FDEEE,
    MEDIUM_INDEX_1000FDFCEEE,
    MEDIUM_INDEX_100FDEEE,
    MEDIUM_INDEX_100FDFCEEE,
    MEDIUM_INDEX_COUNT
};

#define MBit 1000000

static IOMediumType mediumTypeArray[MEDIUM_INDEX_COUNT] = {
    kIOMediumEthernetAuto,
    (kIOMediumEthernet10BaseT | kIOMediumOptionHalfDuplex),
    (kIOMediumEthernet10BaseT | kIOMediumOptionFullDuplex),
    (kIOMediumEthernet100BaseTX | kIOMediumOptionHalfDuplex),
    (kIOMediumEthernet100BaseTX | kIOMediumOptionFullDuplex),
    (kIOMediumEthernet100BaseTX | kIOMediumOptionFullDuplex | kIOMediumOptionFlowControl),
    (kIOMediumEthernet1000BaseT | kIOMediumOptionFullDuplex),
    (kIOMediumEthernet1000BaseT | kIOMediumOptionFullDuplex | kIOMediumOptionFlowControl),
    (kIOMediumEthernet1000BaseT | kIOMediumOptionFullDuplex | kIOMediumOptionEEE),
    (kIOMediumEthernet1000BaseT | kIOMediumOptionFullDuplex | kIOMediumOptionFlowControl | kIOMediumOptionEEE),
    (kIOMediumEthernet100BaseTX | kIOMediumOptionFullDuplex | kIOMediumOptionEEE),
    (kIOMediumEthernet100BaseTX | kIOMediumOptionFullDuplex | kIOMediumOptionFlowControl | kIOMediumOptionEEE)
};

static UInt32 mediumSpeedArray[MEDIUM_INDEX_COUNT] = {
    0,
    10 * MBit,
    10 * MBit,
    100 * MBit,
    100 * MBit,
    100 * MBit,
    1000 * MBit,
    1000 * MBit,
    1000 * MBit,
    1000 * MBit,
    100 * MBit,
    100 * MBit
};

static struct MediumTable
{
    IOMediumType type;
    UInt32 speed;
} mediumTable[] = {
    {kIOMediumIEEE80211None, 0},
    {kIOMediumIEEE80211Auto, 0}
};

void AppleIntelWifiAdapterV2::free() {
    super::free();
    if (drv) {
        drv->release();
        delete drv;
        drv = NULL;
    }
    IOLog("Driver free()");
}

bool AppleIntelWifiAdapterV2::init(OSDictionary *properties)
{
    IOLog("Driver init()");
    drv = new IWLMvmDriver();
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
    if (!drv->init(pciDevice)) {
        return NULL;
    }
    return drv->probe() ? this : NULL;
}

bool AppleIntelWifiAdapterV2::start(IOService *provider)
{
    IOLog("Driver Start()");
    if (!super::start(provider)) {
        return false;
    }
    initTimeout(getWorkLoop());
    int msiIntrIndex = 0;
    for (int index = 0; ; index++)
    {
        int interruptType;
        int ret = provider->getInterruptType(index, &interruptType);
        if (ret != kIOReturnSuccess)
            break;
        if (interruptType & kIOInterruptTypePCIMessaged)
        {
            msiIntrIndex = index;
            break;
        }
    }
    fInterrupt = IOFilterInterruptEventSource::filterInterruptEventSource(this,
                                                             OSMemberFunctionCast(IOInterruptEventSource::Action, this, &AppleIntelWifiAdapterV2::intrOccured),
                                                             OSMemberFunctionCast(IOFilterInterruptAction, this, &AppleIntelWifiAdapterV2::intrFilter),
                                                             provider,
                                                             msiIntrIndex);
    if (getWorkLoop()->addEventSource(fInterrupt) != kIOReturnSuccess) {
        IWL_ERR(0, "add interrupt event soure fail\n");
        return false;
    }
    fInterrupt->enable();
    IONetworkMedium *medium;
    IONetworkMedium *autoMedium;
    OSDictionary *mediumDict = OSDictionary::withCapacity(MEDIUM_INDEX_COUNT + 1);
    if (!mediumDict) {
        IOLog("start fail, can not create mediumdict\n");
        return false;
    }
    bool result;
    for (int i = MEDIUM_INDEX_AUTO; i < MEDIUM_INDEX_COUNT; i++) {
        medium = IONetworkMedium::medium(mediumTypeArray[i], mediumSpeedArray[i], 0, i);
        if (!medium) {
            IOLog("start fail, can not create mediumdict\n");
            return false;
        }
        result = IONetworkMedium::addMedium(mediumDict, medium);
        medium->release();
        if (!result) {
            IOLog("start fail, can not addMedium\n");
            return false;
        }
        if (i == MEDIUM_INDEX_AUTO) {
            autoMedium = medium;
        }
    }
    if (!publishMediumDictionary(mediumDict)) {
        IOLog("start fail, can not publish mediumdict\n");
        return false;
    }
    if (!setSelectedMedium(autoMedium)){
        IOLog("start fail, can not set current medium\n");
        return false;
    }
    
//    UInt32 capacity = sizeof(mediumTable) / sizeof(struct MediumTable);
//
//    OSDictionary *mediumDict = OSDictionary::withCapacity(capacity);
//    if (mediumDict == 0) {
//        return false;
//    }
//
//    for (UInt32 i = 0; i < capacity; i++) {
//        IONetworkMedium* medium = IONetworkMedium::medium(mediumTable[i].type, mediumTable[i].speed);
//        if (medium) {
//            IONetworkMedium::addMedium(mediumDict, medium);
//            medium->release();
//        }
//    }
//
//    if (!publishMediumDictionary(mediumDict)) {
//        return false;
//    }
//
//    IONetworkMedium *m = IONetworkMedium::getMediumWithType(mediumDict, kIOMediumIEEE80211Auto);
//    setSelectedMedium(m);
    if (!attachInterface((IONetworkInterface**)&netif)) {
        IOLog("start failed, can not attach interface\n");
        return false;
    }
    if (!drv->start()) {
        IOLog("start failed\n");
        return false;
    }
    netif->registerService();
    this->registerService();
    return true;
}

bool AppleIntelWifiAdapterV2::intrFilter(OSObject *object, IOFilterInterruptEventSource *src)
{
    return true;
}

bool AppleIntelWifiAdapterV2::configureInterface(IONetworkInterface *interface)
{
    return true;
    bool result = true;
    result = super::configureInterface(interface);
    HackIO80211Interface *inf = (HackIO80211Interface*)interface;
    inf->configureInterface(this);
    return result;
}

int AppleIntelWifiAdapterV2::intrOccured(OSObject *object, IOInterruptEventSource *, int count)
{
    IOLog("interrupt!!!\n");
    drv->irqHandler(0, NULL);
    return kIOReturnSuccess;
}

void AppleIntelWifiAdapterV2::stop(IOService *provider)
{
    IOLog("Driver Stop()");
    releaseTimeout();
    if (fInterrupt) {
        fInterrupt->disable();
        getWorkLoop()->removeEventSource(fInterrupt);
        fInterrupt->release();
        fInterrupt = NULL;
    }
    if (netif) {
        detachInterface(netif);
        netif = NULL;
    }
    super::stop(provider);
}

IOReturn AppleIntelWifiAdapterV2::enable(IONetworkInterface *netif)
{
    IOLog("Driver Enable()");
    //    setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid);
    return super::enable(netif);
}

IOReturn AppleIntelWifiAdapterV2::disable(IONetworkInterface *netif)
{
    IOLog("Driver Disable()");
    return super::disable(netif);
}

IOReturn AppleIntelWifiAdapterV2::setPromiscuousMode(bool active)
{
    return kIOReturnSuccess;
}

IOReturn AppleIntelWifiAdapterV2::setMulticastMode(bool active)
{
    return kIOReturnSuccess;
}

IONetworkInterface *AppleIntelWifiAdapterV2::createInterface()
{
    return super::createInterface();
    IONetworkInterface *inf = new HackIO80211Interface();
    inf->init(this);
    return inf;
}

IOReturn AppleIntelWifiAdapterV2::getHardwareAddress(IOEthernetAddress *addrP) {
    addrP->bytes[0] = 0x29;
    addrP->bytes[1] = 0xC2;
    addrP->bytes[2] = 0xdd;
    addrP->bytes[3] = 0x8F;
    addrP->bytes[4] = 0x93;
    addrP->bytes[5] = 0x4D;
    return kIOReturnSuccess;
}

IOOutputQueue *AppleIntelWifiAdapterV2::createOutputQueue()
{
    if (fOutputQueue == 0) {
        fOutputQueue = IOGatedOutputQueue::withTarget(this, getWorkLoop());
    }
    return fOutputQueue;
}

UInt32 AppleIntelWifiAdapterV2::outputPacket(mbuf_t m, void *param)
{
    freePacket(m);
    return kIOReturnOutputDropped;
}
