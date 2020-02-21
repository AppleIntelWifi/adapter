/* add your code here */
#include "AppleIntelWifiAdapterV2.hpp"

#include <IOKit/IOInterruptController.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/network/IONetworkMedium.h>
#include "IWLDebug.h"

#define super IOEthernetController

OSDefineMetaClassAndStructors(AppleIntelWifiAdapterV2, IOEthernetController)
OSDefineMetaClassAndStructors(HackIO80211Interface, IOEthernetInterface)
OSDefineMetaClassAndStructors(IWLMvmDriver, OSObject)

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

void AppleIntelWifiAdapterV2::releaseAll() {
    IWL_INFO(0, "Releasing everything");
    if(fInterrupt) {
        irqLoop->removeEventSource(fInterrupt);
        fInterrupt->disable();
        fInterrupt = NULL;
    }
    if(irqLoop) {
        irqLoop->release();
        irqLoop = NULL;
    }
    if(netif) {
        netif->release();
        netif = NULL;
    }
    if (drv) {
        drv->release();
        drv = NULL;
    }
}

void AppleIntelWifiAdapterV2::free() {
    IWL_INFO(0, "Driver free()");
    releaseAll();
    super::free();
}

bool AppleIntelWifiAdapterV2::init(OSDictionary *properties)
{
    IWL_INFO(0, "Driver init()");
    drv = new IWLMvmDriver();
    return super::init(properties);
    
    
}

IOService* AppleIntelWifiAdapterV2::probe(IOService *provider, SInt32 *score)
{
    IWL_INFO(0, "Driver Probe()");
    super::probe(provider, score);
    IOPCIDevice *pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        IWL_ERR(0, "Not pci device");
        return NULL;
    }
    UInt16 vendorID = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceID = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subSystemVendorID = pciDevice->configRead16(kIOPCIConfigSubSystemVendorID);
    UInt16 subSystemDeviceID = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    UInt8 revision = pciDevice->configRead8(kIOPCIConfigRevisionID);
    IWL_INFO(0, "find pci device====>vendorID=0x%04x, deviceID=0x%04x, subSystemVendorID=0x%04x, subSystemDeviceID=0x%04x, revision=0x%02x", vendorID, deviceID, subSystemVendorID, subSystemDeviceID, revision);
    this->drv->controller = static_cast<IOEthernetController*>(this);
    
    if (!drv->init(pciDevice)) {
        return NULL;
    }
    
    return drv->probe() ? this : NULL;
}

bool AppleIntelWifiAdapterV2::start(IOService *provider)
{
    IWL_INFO(0, "Driver Start()");
    if (!super::start(provider)) {
        return false;
    }
    initTimeout(getWorkLoop());
    irqLoop = IOWorkLoop::workLoop();
    int msiIntrIndex = 0;
    
    
    if(!this->drv) {
        IWL_ERR(0, "Missing this->drv\n");
        releaseAll();
        return false;
    }
    
    if(!this->drv->m_pDevice) {
        IWL_ERR(0, "Missing this->m_pDevice\n");
        releaseAll();
        return false;
    }
    
    if(!this->drv->m_pDevice->pciDevice) {
        IWL_ERR(0, "Missing this->m_pDevice->pciDevice\n");
        releaseAll();
        return false;
    }
    
    
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
    
    
    IWL_INFO(0, "MSI interrupt index: %d\n", msiIntrIndex);
    fInterrupt = IOFilterInterruptEventSource::filterInterruptEventSource(this,
                                                                          (IOInterruptEventAction) &AppleIntelWifiAdapterV2::intrOccured,
                                                                          (IOFilterInterruptAction)&AppleIntelWifiAdapterV2::intrFilter,
                                                                          provider,
                                                                          msiIntrIndex);
    if (irqLoop->addEventSource(fInterrupt) != kIOReturnSuccess) {
        IWL_ERR(0, "add interrupt event soure fail\n");
        releaseAll();
        return false;
    }
    
    fInterrupt->enable();
    
    
    //    PMinit();
    //    provider->joinPMtree(this);
    //    changePowerStateTo(kOffPowerState);
    //    registerPowerDriver(this, gPowerStates, kNumPowerStates);
    //setIdleTimerPeriod(iwl_mod_params.d0i3_timeout);
    IONetworkMedium *medium;
    IONetworkMedium *autoMedium;
    OSDictionary *mediumDict = OSDictionary::withCapacity(MEDIUM_INDEX_COUNT + 1);
    if (!mediumDict) {
        IWL_ERR(0, "start fail, can not create mediumdict\n");
        releaseAll();
        return false;
    }
    bool result;
    for (int i = MEDIUM_INDEX_AUTO; i < MEDIUM_INDEX_COUNT; i++) {
        medium = IONetworkMedium::medium(mediumTypeArray[i], mediumSpeedArray[i], 0, i);
        if (!medium) {
            IWL_ERR(0, "start fail, can not create mediumdict\n");
            releaseAll();
            return false;
        }
        result = IONetworkMedium::addMedium(mediumDict, medium);
        medium->release();
        if (!result) {
            IWL_ERR(0, "start fail, can not addMedium\n");
            releaseAll();
            return false;
        }
        if (i == MEDIUM_INDEX_AUTO) {
            autoMedium = medium;
        }
    }
    if (!publishMediumDictionary(mediumDict)) {
        IWL_ERR(0, "start fail, can not publish mediumdict\n");
        releaseAll();
        return false;
    }
    if (!setSelectedMedium(autoMedium)){
        IWL_ERR(0, "start fail, can not set current medium\n");
        releaseAll();
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
    
    this->registerService();
    if (!attachInterface((IONetworkInterface**)&netif)) {
        IWL_ERR(0, "start failed, can not attach interface\n");
        releaseAll();
        return false;
    }
    
    drv->m_pDevice->interface = netif;
    
    if (!drv->start()) {
        IWL_ERR(0, "start failed\n");
        releaseAll();
        return false;
    }
    
    //for test
    if(!drv->drvStart()) {
        IWL_ERR(0, "Driver failed to start\n");
        releaseAll();
        return true;
    }
    
    //    if(!drv->m_pDevice->firmwareLoadToBuf) {
    //        IOLog("firmware not loaded to buf");
    //        fInterrupt->disable();
    //        if(irqLoop) {
    //            irqLoop->release();
    //            irqLoop = NULL;
    //        }
    //
    //        if(drv) {
    //            drv->release();
    //            delete drv;
    //            drv = NULL;
    //        }
    //
    //        if(netif) {
    //            netif->release();
    //        }
    //
    //        return false;
    //    }
    //    else {
    //        if(!drv->drvStart()) {
    //            IWL_ERR(0, "Driver failed to start\n");
    //            IOSleep(5000);
    //        }
    //        //return true;
    //    }
    
    netif->registerService();
    
    drv->enableDevice();
    
    
    
    return true;
}

bool AppleIntelWifiAdapterV2::intrFilter(OSObject *object, IOFilterInterruptEventSource *src)
{
    AppleIntelWifiAdapterV2* me = (AppleIntelWifiAdapterV2*)object;
    
    if(me == 0) {
        return false;
    }
    
    kprintf("interrupt filter ran\n");
    me->drv->trans->iwlWrite32(CSR_INT_MASK, 0x00000000);
    return true;
}

void AppleIntelWifiAdapterV2::intrOccured(OSObject *object, IOInterruptEventSource* sender, int count)
{
    AppleIntelWifiAdapterV2* o = (AppleIntelWifiAdapterV2*)object;
    if(o == 0) {
        return;
    }
    
    kprintf("interrupt!!!\n");
    o->drv->irqHandler(0, NULL);
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

void AppleIntelWifiAdapterV2::stop(IOService *provider)
{
    IWL_INFO(0, "Driver Stop()");
    releaseTimeout();
    if (fInterrupt) {
        fInterrupt->disable();
        irqLoop->removeEventSource(fInterrupt);
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
    if(drv) {
        setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid);
        return super::enable(netif);
    } else {
        return kIOReturnError;
    }
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
    if(!this->drv->trans->is_down) {
        if(this->drv->m_pDevice->nvm_data) {
            memcpy(addrP->bytes, &this->drv->m_pDevice->nvm_data->hw_addr[0], ETHER_ADDR_LEN);
            IWL_INFO(0, "Got request for hw addr: returning %02x:%02x:%02x\n", drv->m_pDevice->nvm_data->hw_addr[0],
                                                                            drv->m_pDevice->nvm_data->hw_addr[1],
                                                                            drv->m_pDevice->nvm_data->hw_addr[2]);
        }
        else {
            addrP->bytes[0] = 0x29;
            addrP->bytes[1] = 0xC2;
            addrP->bytes[2] = 0xdd;
            addrP->bytes[3] = 0x8F;
            addrP->bytes[4] = 0x93;
            addrP->bytes[5] = 0x4D;
        }
    }
    else {
        return kIOReturnError;
    }
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
