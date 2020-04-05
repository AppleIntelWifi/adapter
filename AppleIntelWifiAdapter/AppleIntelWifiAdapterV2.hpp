//
//  AppleIntelWifiAdapterV2.hpp
//  AppleIntelWifiAdapter
//
//  Created by Harrison Ford on 4/4/20.
//  Copyright © 2020 IntelWifi for MacOS authors. All rights reserved.
//

#ifndef APPLEINTELWIFIADAPTER_APPLEINTELWIFIADAPTERV2_HPP_
#define APPLEINTELWIFIADAPTER_APPLEINTELWIFIADAPTERV2_HPP_

//#include "HackIO80211Interface.h"
#include "IO80211Controller.h"
#include "IO80211Interface.h"
#include <IOKit/network/IOEthernetController.h>
#include "IOKit/network/IOGatedOutputQueue.h"
#include <IOKit/IOFilterInterruptEventSource.h>
#include <libkern/c++/OSString.h>

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>
#include "IWLMvmDriver.hpp"

typedef enum {
    MEDIUM_TYPE_NONE = 0,
    MEDIUM_TYPE_AUTO,
    MEDIUM_TYPE_1MBIT,
    MEDIUM_TYPE_2MBIT,
    MEDIUM_TYPE_5MBIT,
    MEDIUM_TYPE_11MBIT,
    MEDIUM_TYPE_54MBIT,
    MEDIUM_TYPE_INVALID
} mediumType_t;


enum {
    kOffPowerState,
    kOnPowerState,
    kNumPowerStates
};

static IOPMPowerState gPowerStates[kNumPowerStates] = {
    // kOffPowerState
    {kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // kOnPowerState
    {kIOPMPowerStateVersion1, (kIOPMPowerOn | kIOPMDeviceUsable), kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};



class AppleIntelWifiAdapterV2 : public IO80211Controller
{
    OSDeclareDefaultStructors( AppleIntelWifiAdapterV2 )
public:
    bool init(OSDictionary* parameters) override;
    void free() override;
    bool start(IOService* provider) override;
    bool startGated(IOService* provider);
    void stop(IOService* provider) override;
    IOService* probe(IOService* provider, SInt32* score) override;
    
    SInt32 apple80211Request(unsigned int request_type, int request_number, IO80211Interface* interface, void* data) override;
    UInt32 outputPacket (mbuf_t m, void* param) override;
    IOReturn getMaxPacketSize(UInt32* maxSize) const;
    const OSString* newVendorString() const;
    const OSString* newModelString() const;
    const OSString* newRevisionString() const;
    IOReturn enable(IONetworkInterface *netif) override;
    IOReturn disable(IONetworkInterface *netif) override;
    bool configureInterface(IONetworkInterface *netif) override;
    IO80211Interface* getNetworkInterface() override;
    IOReturn getHardwareAddressForInterface(IO80211Interface* netif, IOEthernetAddress* addr) override;
    IOReturn getHardwareAddress(IOEthernetAddress* addr) override;
    IOReturn setPromiscuousMode(IOEnetPromiscuousMode mode) override;
    IOReturn setMulticastMode(IOEnetMulticastMode mode) override;
    IOReturn setMulticastList(IOEthernetAddress* addr, UInt32 len) override;
    SInt32 monitorModeSetEnabled(IO80211Interface* interface, bool enabled, UInt32 dlt) override;
    
    bool createWorkLoop() override;
    IOWorkLoop* getWorkLoop() const override;
    
    //IOOutputQueue * createOutputQueue() override;

    
protected:
    IO80211Interface* getInterface();
    
private:
    
    static IOReturn _doCommand(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3);
    
    static IOReturn scanAction(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3);
    
    // 1 - SSID
    IOReturn getSSID(IO80211Interface* interface, struct apple80211_ssid_data* sd);
    IOReturn setSSID(IO80211Interface* interface, struct apple80211_ssid_data* sd);
    // 2 - AUTH_TYPE
    IOReturn getAUTH_TYPE(IO80211Interface* interface, struct apple80211_authtype_data* ad);
    // 4 - CHANNEL
    IOReturn getCHANNEL(IO80211Interface* interface, struct apple80211_channel_data* cd);
    // 6 - PROTMODE
    IOReturn getPROTMODE(IO80211Interface* interface, struct apple80211_protmode_data* pd);
    IOReturn setPROTMODE(IO80211Interface* interface, struct apple80211_protmode_data* pd);
    // 7 - TXPOWER
    IOReturn getTXPOWER(IO80211Interface* interface, struct apple80211_txpower_data* txd);
    // 8 - RATE
    IOReturn getRATE(IO80211Interface* interface, struct apple80211_rate_data* rd);
    // 9 - BSSID
    IOReturn getBSSID(IO80211Interface* interface, struct apple80211_bssid_data* bd);
    // 10 - SCAN_REQ
    IOReturn setSCAN_REQ(IO80211Interface* interface, struct apple80211_scan_data* sd);
    // 11 - SCAN_RESULT
    IOReturn getSCAN_RESULT(IO80211Interface* interface, apple80211_scan_result* *sr);
    // 12 - CARD_CAPABILITIES
    IOReturn getCARD_CAPABILITIES(IO80211Interface* interface, struct apple80211_capability_data* cd);
    // 13 - STATE
    IOReturn getSTATE(IO80211Interface* interface, struct apple80211_state_data* sd);
    IOReturn setSTATE(IO80211Interface* interface, struct apple80211_state_data* sd);
    // 14 - PHY_MODE
    IOReturn getPHY_MODE(IO80211Interface* interface, struct apple80211_phymode_data* pd);
    // 15 - OP_MODE
    IOReturn getOP_MODE(IO80211Interface* interface, struct apple80211_opmode_data* od);
    // 16 - RSSI
    IOReturn getRSSI(IO80211Interface* interface, struct apple80211_rssi_data* rd);
    // 17 - NOISE
    IOReturn getNOISE(IO80211Interface* interface,struct apple80211_noise_data* nd);
    // 18 - INT_MIT
    IOReturn getINT_MIT(IO80211Interface* interface, struct apple80211_intmit_data* imd);
    // 19 - POWER
    IOReturn getPOWER(IO80211Interface* interface, struct apple80211_power_data* pd);
    IOReturn setPOWER(IO80211Interface* interface, struct apple80211_power_data* pd);
    // 20 - ASSOCIATE
    IOReturn setASSOCIATE(IO80211Interface* interface,struct apple80211_assoc_data* ad);
    // 27 - SUPPORTED_CHANNELS
    IOReturn getSUPPORTED_CHANNELS(IO80211Interface* interface, struct apple80211_sup_channel_data* ad);
    // 28 - LOCALE
    IOReturn getLOCALE(IO80211Interface* interface, struct apple80211_locale_data* ld);
    // 37 - TX_ANTENNA
    IOReturn getTX_ANTENNA(IO80211Interface* interface, apple80211_antenna_data* ad);
    // 39 - ANTENNA_DIVERSITY
    IOReturn getANTENNA_DIVERSITY(IO80211Interface* interface, apple80211_antenna_data* ad);
    // 43 - DRIVER_VERSION
    IOReturn getDRIVER_VERSION(IO80211Interface* interface, struct apple80211_version_data* hv);
    // 44 - HARDWARE_VERSION
    IOReturn getHARDWARE_VERSION(IO80211Interface* interface, struct apple80211_version_data* hv);
    // 50 - ASSOCIATION_STATUS
    IOReturn getASSOCIATION_STATUS(IO80211Interface* interface, struct apple80211_assoc_status_data* hv);
    // 51 - COUNTRY_CODE
    IOReturn getCOUNTRY_CODE(IO80211Interface* interface, struct apple80211_country_code_data* cd);
    // 57 - MCS
    IOReturn getMCS(IO80211Interface* interface, struct apple80211_mcs_data* md);
    IOReturn getROAM_THRESH(IO80211Interface* interface, struct apple80211_roam_threshold_data* md);
    IOReturn getRADIO_INFO(IO80211Interface* interface, struct apple80211_radio_info_data* md);
    
    void releaseAll();
    static void intrOccured(OSObject *object, IOInterruptEventSource *, int count);
    static bool intrFilter(OSObject *object, IOFilterInterruptEventSource *src);
    bool addMediumType(UInt32 type, UInt32 speed, UInt32 code, char* name = 0);
    IWLMvmDriver *drv;

    IOGatedOutputQueue*    fOutputQueue;
    IOInterruptEventSource* fInterrupt;
    IO80211Interface *netif;
    IOCommandGate *gate;
    IO80211WorkLoop* workLoop;
    IO80211WorkLoop* irqLoop;
    OSDictionary* mediumDict;
    IONetworkMedium* mediumTable[MEDIUM_TYPE_INVALID];
    //IO80211WorkLoop* workLoop;
    

};

#endif  // APPLEINTELWIFIADAPTER_APPLEINTELWIFIADAPTERV2_HPP_
