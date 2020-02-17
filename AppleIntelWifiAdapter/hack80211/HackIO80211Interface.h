//
//  HackIO80211Interface.h
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/10.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#ifndef HackIO80211Interface_h
#define HackIO80211Interface_h

//#include <IOKit/network/IOEthernetInterface.h>
#include "HackIOEthernetInterface.h"
#include <net/if_var.h>
#include <sys/queue.h>

#define IFNAMSIZ 16

enum IO80211LinkState
{
    kIO80211NetworkLinkUndefined,            // Starting link state when an interface is created
    kIO80211NetworkLinkDown,                // Interface not capable of transmitting packets
    kIO80211NetworkLinkUp,                    // Interface capable of transmitting packets
};
typedef enum IO80211LinkState IO80211LinkState;

/*!    @defined kIO80211InterfaceClass
    @abstract The name of the IO80211Interface class.
    */

#define kHackIO80211InterfaceClass     "HackIO80211Interface"

typedef UInt64 IO80211FlowQueueHash;
class RSNSupplicant;
class IOTimerEventSource;
class IOGatedOutputQueue;
class IO80211Controller;
class IO80211Workloop;
class IO80211ScanManager;
class IO80211PeerManager;
class IO80211FlowQueueDatabase;
class IO80211InterfaceMonitor;
class IO80211AssociationJoinSnapshot;

struct apple80211_debug_command;
struct apple80211_txstats;
struct apple80211_chip_counters_tx;
struct apple80211_chip_error_counters_tx;
struct apple80211_chip_counters_rx;
struct apple80211_ManagementInformationBasedot11_counters;
struct apple80211_leaky_ap_stats;
struct apple80211_leaky_ap_ssid_metrics;
struct apple80211_interface_availability;
struct apple80211_pmk_cache_data;
struct apple80211_ap_cmp_data;
struct TxPacketRequest;
struct AWSRequest;
struct packet_info_tx;
struct userPrintCtx;

typedef int apple80211_postMessage_tlv_types;

#define IOCTL_GET 3224398281LL
#define IOCTL_SET 2150656456LL

struct apple80211req
{
    char        req_if_name[IFNAMSIZ];    // 16 bytes
    int            req_type;                // 4 bytes
    int            req_val;                // 4 bytes
    u_int32_t    req_len;                // 4 bytes
    void       *req_data;                // 4 bytes
};

class HackIO80211Interface : public IOEthernetInterface
{
    OSDeclareDefaultStructors( HackIO80211Interface );
    
public:
    virtual bool terminate(unsigned int) APPLE_KEXT_OVERRIDE;
    virtual bool attach(IOService*) APPLE_KEXT_OVERRIDE;
    virtual void detach(IOService*) APPLE_KEXT_OVERRIDE;
    virtual bool init(IONetworkController*) APPLE_KEXT_OVERRIDE;
    virtual UInt32 inputPacket(mbuf_t          packet,
                               UInt32          length  = 0,
                               IOOptionBits    options = 0,
                               void *          param   = 0) APPLE_KEXT_OVERRIDE;
    virtual bool inputEvent(unsigned int, void*) APPLE_KEXT_OVERRIDE;
    virtual SInt32 performCommand(IONetworkController*, unsigned long, void*, void*) APPLE_KEXT_OVERRIDE;
    virtual IOReturn attachToDataLinkLayer(IOOptionBits, void*) APPLE_KEXT_OVERRIDE;
    virtual void detachFromDataLinkLayer(unsigned int, void*) APPLE_KEXT_OVERRIDE;
    virtual void setPoweredOnByUser(bool);
    virtual void setEnabledBySystem(bool);
    virtual bool setLinkState(IO80211LinkState, unsigned int);
    virtual bool setLinkState(IO80211LinkState, int, unsigned int);
    virtual UInt32 outputPacket(mbuf_t, void*);

    bool configureInterface(IOEthernetController *controller);
    
    virtual bool setLinkQualityMetric(int);
    virtual void handleDebugCmd(apple80211_debug_command*);
    
    UInt64 IO80211InterfaceUserSpaceToKernelApple80211Request(void *arg, apple80211req *req, unsigned long ctl);
    
    int performGatedCommand(void *a2, void *a3, void *a4, void *a5, void *a6);
    
    int apple80211_ioctl(HackIO80211Interface *netif, UInt64 method, apple80211req *a6);
    
    int apple80211_ioctl_set(HackIO80211Interface *netif, apple80211req *a6);
    
    int apple80211_ioctl_get(HackIO80211Interface *netif, apple80211req *a6);
};

#endif /* HackIO80211Interface_h */
