/* add your code here */
#include "AppleIntelWifiAdapterV2.hpp"

#include <IOKit/IOInterruptController.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/network/IONetworkMedium.h>
#include "IWLDebug.h"
#include "IO80211Interface.h"
#include "ioctl_dbg.h"

OSDefineMetaClassAndStructors(AppleIntelWifiAdapterV2, IO80211Controller)
#define super IO80211Controller


#define MBit 1000000


void AppleIntelWifiAdapterV2::releaseAll() {
    IWL_INFO(0, "Releasing everything\n");
    if(fInterrupt) {
        irqLoop->removeEventSource(fInterrupt);
        fInterrupt->disable();
        fInterrupt = NULL;
    }
    
    if(gate) {
        gate->release();
        gate = NULL;
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
        //this->drv->OSObject::release();
        drv->release();
        //drv->free();
        drv = NULL;
    }
}

void AppleIntelWifiAdapterV2::free() {
    IWL_INFO(0, "Driver free()\n");
    releaseAll();
    super::free();
}

bool AppleIntelWifiAdapterV2::init(OSDictionary *properties)
{
    IWL_INFO(0, "Driver init()\n");
    if(!super::init(properties)) {
        return false;
    }
    
    return true;
}

IO80211Interface* AppleIntelWifiAdapterV2::getInterface() {
    return netif;
}

IOService* AppleIntelWifiAdapterV2::probe(IOService *provider, SInt32 *score)
{
    IWL_INFO(0, "Driver Probe()\n");
    if(!super::probe(provider, score)) {
        return NULL;
    }
    
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
    bool valid = false;
    for (int i = 0; i < ARRAY_SIZE(iwl_hw_card_ids); i++) {
        pci_device_id dev = iwl_hw_card_ids[i];
        if (dev.device == deviceID) {
            //struct iwl_cfg* ptr = (struct iwl_cfg*
            //                       )&dev.driver_data;
            //this->cfg = iwl_cfg(*ptr);
            //IWL_INFO(0, "name? %s\n", ptr->name);
            //memcpy(&this->cfg, ptr, sizeof(iwl_cfg));
            valid = true;
            //this->cfg = *ptr;
            break;
        }
    }
    
    if(!valid) {
        return NULL;
    }
    
    IWL_INFO(0, "find pci device====>vendorID=0x%04x, deviceID=0x%04x, subSystemVendorID=0x%04x, subSystemDeviceID=0x%04x, revision=0x%02x\n", vendorID, deviceID, subSystemVendorID, subSystemDeviceID, revision);
    
    pciDevice->retain();
    this->drv = new IWLMvmDriver();
    this->drv->m_pDevice = new IWLDevice();
    this->drv->m_pDevice->pciDevice = pciDevice;
    this->drv->m_pDevice->state = APPLE80211_S_INIT;
    
    //this->drv->m_pDevice->controller = (IO80211Controller*)this;
    return this;
}

bool AppleIntelWifiAdapterV2::createWorkLoop() {
    if(!irqLoop) {
        irqLoop = IO80211WorkLoop::workLoop();
    }
    
    return (irqLoop != NULL);
}

IOWorkLoop* AppleIntelWifiAdapterV2::getWorkLoop() const {
    return irqLoop;
}


SInt32 AppleIntelWifiAdapterV2::apple80211Request(unsigned int request_type,
                                            int request_number,
                                            IO80211Interface* interface,
                                            void* data) {
    if (request_type != SIOCGA80211 && request_type != SIOCSA80211) {
        IWL_ERR(0, "Invalid IOCTL request type: %u", request_type);
        IWL_ERR(0, "Expected either %lu or %lu", SIOCGA80211, SIOCSA80211);
        return kIOReturnError;
    }

    IOReturn ret = 0;
    
    bool isGet = (request_type == SIOCGA80211);
    
#define IOCTL(REQ_TYPE, REQ, DATA_TYPE) \
if (REQ_TYPE == SIOCGA80211) { \
ret = get##REQ(interface, (struct DATA_TYPE* )data); \
} else { \
ret = set##REQ(interface, (struct DATA_TYPE* )data); \
}
    
#define IOCTL_GET(REQ_TYPE, REQ, DATA_TYPE) \
if (REQ_TYPE == SIOCGA80211) { \
    ret = get##REQ(interface, (struct DATA_TYPE* )data); \
}
#define IOCTL_SET(REQ_TYPE, REQ, DATA_TYPE) \
if (REQ_TYPE == SIOCSA80211) { \
    ret = set##REQ(interface, (struct DATA_TYPE* )data); \
}
    
    IWL_INFO(0, "IOCTL %s(%d) %s\n",
          isGet ? "get" : "set",
          request_number,
          IOCTL_NAMES[request_number]);
    
    switch(request_number) {
        case APPLE80211_IOC_SSID: // 1
            IOCTL(request_type, SSID, apple80211_ssid_data);
            break;
        case APPLE80211_IOC_AUTH_TYPE: // 2
            IOCTL_GET(request_type, AUTH_TYPE, apple80211_authtype_data);
            break;
        case APPLE80211_IOC_CHANNEL: // 4
            IOCTL_GET(request_type, CHANNEL, apple80211_channel_data);
            break;
        case APPLE80211_IOC_TXPOWER: // 7
            IOCTL_GET(request_type, TXPOWER, apple80211_txpower_data);
            break;
        case APPLE80211_IOC_RATE: // 8
            IOCTL_GET(request_type, RATE, apple80211_rate_data);
            break;
        case APPLE80211_IOC_BSSID: // 9
            IOCTL_GET(request_type, BSSID, apple80211_bssid_data);
            break;
        case APPLE80211_IOC_SCAN_REQ: // 10
            IOCTL_SET(request_type, SCAN_REQ, apple80211_scan_data);
            break;
        case APPLE80211_IOC_SCAN_RESULT: // 11
            IOCTL_GET(request_type, SCAN_RESULT, apple80211_scan_result*);
            break;
        case APPLE80211_IOC_CARD_CAPABILITIES: // 12
            IOCTL_GET(request_type, CARD_CAPABILITIES, apple80211_capability_data);
            break;
        case APPLE80211_IOC_STATE: // 13
            IOCTL_GET(request_type, STATE, apple80211_state_data);
            break;
        case APPLE80211_IOC_PHY_MODE: // 14
            IOCTL_GET(request_type, PHY_MODE, apple80211_phymode_data);
            break;
        case APPLE80211_IOC_OP_MODE: // 15
            IOCTL_GET(request_type, OP_MODE, apple80211_opmode_data);
            break;
        case APPLE80211_IOC_RSSI: // 16
            IOCTL_GET(request_type, RSSI, apple80211_rssi_data);
            break;
        case APPLE80211_IOC_NOISE: // 17
            IOCTL_GET(request_type, NOISE, apple80211_noise_data);
            break;
        case APPLE80211_IOC_INT_MIT: // 18
            IOCTL_GET(request_type, INT_MIT, apple80211_intmit_data);
            break;
        case APPLE80211_IOC_POWER: // 19
            IOCTL(request_type, POWER, apple80211_power_data);
            break;
        case APPLE80211_IOC_ASSOCIATE: // 20
            IOCTL_SET(request_type, ASSOCIATE, apple80211_assoc_data);
            break;
        case APPLE80211_IOC_SUPPORTED_CHANNELS: // 27
            IOCTL_GET(request_type, SUPPORTED_CHANNELS, apple80211_sup_channel_data);
            break;
        case APPLE80211_IOC_LOCALE: // 28
            IOCTL_GET(request_type, LOCALE, apple80211_locale_data);
            break;
        case APPLE80211_IOC_TX_ANTENNA: // 37
            IOCTL_GET(request_type, TX_ANTENNA, apple80211_antenna_data);
            break;
        case APPLE80211_IOC_ANTENNA_DIVERSITY: // 39
            IOCTL_GET(request_type, ANTENNA_DIVERSITY, apple80211_antenna_data);
            break;
        case APPLE80211_IOC_DRIVER_VERSION: // 43
            IOCTL_GET(request_type, DRIVER_VERSION, apple80211_version_data);
            break;
        case APPLE80211_IOC_HARDWARE_VERSION: // 44
            IOCTL_GET(request_type, HARDWARE_VERSION, apple80211_version_data);
            break;
        case APPLE80211_IOC_COUNTRY_CODE: // 51
            IOCTL_GET(request_type, COUNTRY_CODE, apple80211_country_code_data);
            break;
        case APPLE80211_IOC_RADIO_INFO:
            IOCTL_GET(request_type, RADIO_INFO, apple80211_radio_info_data);
            break;
        case APPLE80211_IOC_MCS: // 57
            IOCTL_GET(request_type, MCS, apple80211_mcs_data);
            break;
        case APPLE80211_IOC_WOW_PARAMETERS: // 69
            break;
        case APPLE80211_IOC_ROAM_THRESH:
            IOCTL_GET(request_type, ROAM_THRESH, apple80211_roam_threshold_data);
            break;
        case APPLE80211_IOC_TX_CHAIN_POWER: // 108
            break;
        case APPLE80211_IOC_THERMAL_THROTTLING: // 111
            break;
        case APPLE80211_IOC_POWERSAVE:
            break;
        case APPLE80211_IOC_IE:
            break;
        default:
            IWL_ERR(0, "Unhandled IOCTL %s (%d)\n", IOCTL_NAMES[request_number], request_number);
            ret = kIOReturnError;
            break;
    }
    
    return ret;
}

bool AppleIntelWifiAdapterV2::start(IOService *provider)
{
    IWL_INFO(0, "Driver Start()\n");
    if (!super::start(provider)) {
        return false;
    }
    
    
    if(!this->drv) {
        IWL_ERR(0, "Missing this->drv\n");
        releaseAll();
        return false;
    }
    
    this->drv->controller = static_cast<IO80211Controller*>(this);
    if (!this->drv->init()) {
        return false;
    }
    
    if(!this->drv->probe()) {
        return false;
    }
    
    initTimeout(getWorkLoop());
    
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
    
    gate = IOCommandGate::commandGate(this);
    if(!gate) {
        IWL_ERR(0, "Failed to create command gate\n");
        releaseAll();
        return false;
    }
    
    int status = 0;
    
    getCommandGate()->runAction(&_doCommand, (void*)16, &status);
    
    
    
    return true;
}

IOReturn AppleIntelWifiAdapterV2::_doCommand(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3) {
    return kIOReturnSuccess;
}

bool AppleIntelWifiAdapterV2::startGated(IOService *provider) {
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
    
    PMinit();
    provider->joinPMtree(this);
    changePowerStateTo(kOffPowerState);
    registerPowerDriver(this, gPowerStates, kNumPowerStates);
    
    //setIdleTimerPeriod(iwl_mod_params.d0i3_timeout);

    mediumDict = OSDictionary::withCapacity(MEDIUM_TYPE_INVALID + 1);
    if (!mediumDict) {
        IWL_ERR(0, "start fail, can not create mediumdict\n");
        releaseAll();
        return false;
    }
    addMediumType(kIOMediumIEEE80211None,  0,  MEDIUM_TYPE_NONE);
    addMediumType(kIOMediumIEEE80211Auto,  0,  MEDIUM_TYPE_AUTO);
    addMediumType(kIOMediumIEEE80211DS1,   1000000, MEDIUM_TYPE_1MBIT);
    addMediumType(kIOMediumIEEE80211DS2,   2000000, MEDIUM_TYPE_2MBIT);
    addMediumType(kIOMediumIEEE80211DS5,   5500000, MEDIUM_TYPE_5MBIT);
    addMediumType(kIOMediumIEEE80211DS11, 11000000, MEDIUM_TYPE_11MBIT);
    addMediumType(kIOMediumIEEE80211,     54000000, MEDIUM_TYPE_54MBIT, "OFDM54");
    
    if (!publishMediumDictionary(mediumDict)) {
        IWL_ERR(0, "start fail, can not publish mediumdict\n");
        releaseAll();
        return false;
    }
    
    if (!setCurrentMedium(mediumTable[MEDIUM_TYPE_AUTO])) {
        IWL_CRIT(0, "Failed to set current medium!\n");
        releaseAll();
        return false;
    }
    
    if (!setSelectedMedium(mediumTable[MEDIUM_TYPE_AUTO])){
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
    
    if (!attachInterface((IONetworkInterface**)&netif)) {
        IWL_ERR(0, "start failed, can not attach interface\n");
        releaseAll();
        return false;
    }
    
    drv->m_pDevice->interface = netif;
    
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
    
    if (!drv->start()) {
        IWL_ERR(0, "start failed\n");
        releaseAll();
        return false;
    }
    
    
    netif->registerService();
    registerService();
    
    netif->setEnabledBySystem(true);
    
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
    return super::configureInterface(interface);
}

void AppleIntelWifiAdapterV2::stop(IOService *provider)
{
    IWL_INFO(0, "Driver Stop()\n");
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
    IWL_INFO(0, "Driver Enable()\n");
    IOMediumType mediumType = kIOMediumIEEE80211Auto;
    IONetworkMedium *medium = IONetworkMedium::getMediumWithType(mediumDict, mediumType);
    setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid, medium);
    if(drv) {
        //for test
        if(!drv->drvStart()) {
            IWL_ERR(0, "Driver failed to start\n");
            releaseAll();
            return false;
        }
        
        if(!drv->enableDevice()) {
            return kIOReturnBusy;
        }
        
        //netif->postMessage(1);
        this->netif->postMessage(1);
        return kIOReturnSuccess;
    } else {
        return kIOReturnError;
    }
}

const OSString* AppleIntelWifiAdapterV2::newModelString() const {
    if(drv) {
        return OSString::withCString(drv->m_pDevice->name);
    }
    
    return OSString::withCString("Wireless Card");
}

const OSString* AppleIntelWifiAdapterV2::newVendorString() const {
    return OSString::withCString("Intel");
}

const OSString* AppleIntelWifiAdapterV2::newRevisionString() const {
    return OSString::withCString("1.0");
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

IO80211Interface* AppleIntelWifiAdapterV2::getNetworkInterface() {
    return this->netif;
}

IOReturn AppleIntelWifiAdapterV2::getHardwareAddress(IOEthernetAddress *addrP) {
    if(!this->drv->m_pDevice->rfkill_safe_init_done) {
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

IOReturn AppleIntelWifiAdapterV2::getHardwareAddressForInterface(IO80211Interface* netif, IOEthernetAddress* addr) {
    return getHardwareAddress(addr);
}

IOReturn AppleIntelWifiAdapterV2::setMulticastList(IOEthernetAddress* addr, UInt32 len) {
    return kIOReturnSuccess;
}

SInt32 AppleIntelWifiAdapterV2::monitorModeSetEnabled(IO80211Interface* interface,
                                                bool enabled,
                                                UInt32 dlt) {
    return kIOReturnSuccess;
}

/*
IOOutputQueue *AppleIntelWifiAdapterV2::createOutputQueue()
{
    if (fOutputQueue == 0) {
        fOutputQueue = IOGatedOutputQueue::withTarget(this, getWorkLoop());
    }
    return fOutputQueue;
}
*/

UInt32 AppleIntelWifiAdapterV2::outputPacket(mbuf_t m, void *param)
{
    freePacket(m);
    return kIOReturnOutputDropped;
}

IOReturn AppleIntelWifiAdapterV2::getMaxPacketSize(UInt32* maxSize) const {
    *maxSize = 1500;
    return kIOReturnSuccess;
}

bool AppleIntelWifiAdapterV2::addMediumType(UInt32 type, UInt32 speed, UInt32 code, char* name) {
    bool ret = false;
    
    IONetworkMedium* medium = IONetworkMedium::medium(type, speed, 0, code, name);
    if (medium) {
        ret = IONetworkMedium::addMedium(mediumDict, medium);
        if (ret)
            mediumTable[code] = medium;
        medium->release();
    }
    return ret;
}
