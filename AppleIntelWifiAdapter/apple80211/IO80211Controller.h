#ifndef _IO80211CONTROLLER_H
#define _IO80211CONTROLLER_H

#if defined(KERNEL) && defined(__cplusplus)

#include <libkern/version.h>

#if VERSION_MAJOR > 8
#define _MODERN_BPF
#endif

#include <sys/kpi_mbuf.h>

#include <IOKit/network/IOEthernetController.h>
//#include "IOEthernetController.h"

#include <sys/param.h>
#include <net/bpf.h>

#include "apple80211_ioctl.h"
#include "IO80211SkywalkInterface.h"
#include "IO80211WorkLoop.h"

#define AUTH_TIMEOUT            15    // seconds

/*! @enum LinkSpeed.
 @abstract ???.
 @discussion ???.
 @constant LINK_SPEED_80211A 54 Mbps
 @constant LINK_SPEED_80211B 11 Mbps.
 @constant LINK_SPEED_80211G 54 Mbps.
 */
enum {
    LINK_SPEED_80211A    = 54000000ul,        // 54 Mbps
    LINK_SPEED_80211B    = 11000000ul,        // 11 Mbps
    LINK_SPEED_80211G    = 54000000ul,        // 54 Mbps
    LINK_SPEED_80211N    = 300000000ul,        // 300 Mbps (MCS index 15, 400ns GI, 40 MHz channel)
};

enum IO80211CountryCodeOp
{
    kIO80211CountryCodeReset,                // Reset country code to world wide default, and start
    // searching for 802.11d beacon
};
typedef enum IO80211CountryCodeOp IO80211CountryCodeOp;

enum IO80211SystemPowerState
{
    kIO80211SystemPowerStateUnknown,
    kIO80211SystemPowerStateAwake,
    kIO80211SystemPowerStateSleeping,
};
typedef enum IO80211SystemPowerState IO80211SystemPowerState;

enum IO80211FeatureCode
{
    kIO80211Feature80211n = 1,
};
typedef enum IO80211FeatureCode IO80211FeatureCode;


class IOSkywalkInterface;
class IO80211ScanManager;
enum CCStreamLogLevel
{
    LEVEL_1,
};

enum scanSource
{
    SOURCE_1,
};

enum joinStatus
{
    STATUS_1,
};

class IO80211Controller;
class IO80211Interface;
class IO82110WorkLoop;
class IO80211VirtualInterface;
class IO80211ControllerMonitor;
class CCLogPipe;
class CCIOReporterLogStream;
class CCLogStream;
class IO80211VirtualInterface;
class IO80211RangingManager;
class IO80211FlowQueue;
class IO80211FlowQueueLegacy;
class FlowIdMetadata;
class IOReporter;
extern void IO80211VirtualInterfaceNamerRetain();


struct apple80211_hostap_state;

struct apple80211_awdl_sync_channel_sequence;
struct ieee80211_ht_capability_ie;
struct apple80211_channel_switch_announcement;
struct apple80211_beacon_period_data;
struct apple80211_power_debug_sub_info;
struct apple80211_stat_report;
struct apple80211_frame_counters;
struct apple80211_leaky_ap_event;
struct apple80211_chip_stats;
struct apple80211_extended_stats;
struct apple80211_ampdu_stat_report;
struct apple80211_btCoex_report;
struct apple80211_cca_report;
struct apple80211_lteCoex_report;

//typedef int scanSource;
//typedef int joinStatus;
//typedef int CCStreamLogLevel;
typedef IOReturn (*IOCTL_FUNC)(IO80211Controller*, IO80211Interface*, IO80211VirtualInterface*, apple80211req*, bool);
extern IOCTL_FUNC gGetHandlerTable[];
extern IOCTL_FUNC gSetHandlerTable[];

#define __int64 int
#define ulong unsigned long
#define _QWORD UInt64
#define uint UInt

class IO80211Controller : public IOEthernetController {
    OSDeclareAbstractStructors( IO80211Controller )
public:
    virtual IO80211SkywalkInterface* getInfraInterface(void);
    virtual IO80211ScanManager* getPrimaryInterfaceScanManager(void);
    virtual IO80211ControllerMonitor* getInterfaceMonitor(void);
    virtual IOReturn updateReport(IOReportChannelList *,uint,void *,void *) override;
    virtual IOReturn configureReport(IOReportChannelList *,uint,void *,void *) override;
    virtual IOReturn addReporterLegend(IOService *,IOReporter *,char const*,char const*);
    virtual IOReturn removeReporterFromLegend(IOService *,IOReporter *,char const*,char const*);
    virtual IOReturn unlockIOReporterLegend(void);
    virtual void lockIOReporterLegend(void);//怀疑对象，之前是返回int
    virtual IOReturn logIOReportLogStreamSubscription(ulong long);
    virtual IOReturn addIOReportLogStreamForProvider(IOService *,ulong long *);
    virtual IOReturn addSubscriptionForThisReporterFetchedOnTimer(IOReporter *,char const*,char const*,IOService *);
    virtual IOReturn addSubscriptionForProviderFetchedOnTimer(IOService *);
    virtual void handleIOReporterTimer(IOTimerEventSource *);//怀疑对象，之前是返回int
    virtual void setIOReportersStreamFlags(ulong long);//怀疑对象，之前是返回int
    virtual void updateIOReportersStreamFrequency(void);//怀疑对象，之前是返回int
    virtual void setIOReportersStreamLevel(CCStreamLogLevel);//怀疑对象，之前是返回int
    virtual SInt32 apple80211SkywalkRequest(uint,int,IO80211SkywalkInterface *,void *);
    virtual SInt32 apple80211VirtualRequest(uint,int,IO80211VirtualInterface *,void *);
    virtual SInt32 disableVirtualInterface(IO80211VirtualInterface *);
    virtual SInt32 enableVirtualInterface(IO80211VirtualInterface *);
    virtual SInt32 setVirtualHardwareAddress(IO80211VirtualInterface *,ether_addr *);
    virtual UInt32 getDataQueueDepth(OSObject *);
    virtual void powerChangeGated(OSObject *,void *,void *,void *,void *);//怀疑对象，之前是返回int
    virtual int copyOut(void const*,ulong long,ulong);
    virtual int bpfOutputPacket(OSObject *,uint,mbuf_t);
    virtual void requestPacketTx(void *,uint);//怀疑对象，之前是返回int
    virtual int outputActionFrame(IO80211Interface *,mbuf_t);
    virtual int outputRaw80211Packet(IO80211Interface *,mbuf_t);
    virtual IOReturn getHardwareAddressForInterface(IO80211Interface *,IOEthernetAddress *);
    virtual bool useAppleRSNSupplicant(IO80211VirtualInterface *);
    virtual bool useAppleRSNSupplicant(IO80211Interface *);
    virtual void dataLinkLayerAttachComplete(IO80211Interface *);//怀疑对象，之前是返回int
    virtual IO80211VirtualInterface* createVirtualInterface(ether_addr *,uint);
    virtual bool detachVirtualInterface(IO80211VirtualInterface *,bool);
    virtual bool attachVirtualInterface(IO80211VirtualInterface **,ether_addr *,uint,bool);
    virtual bool attachInterface(IOSkywalkInterface *,IOService *);
    virtual void detachInterface(IONetworkInterface *,bool) override;
    virtual bool attachInterface(IONetworkInterface **,bool) override;
    virtual IOService* getProvider(void);
    virtual SInt32 getASSOCIATE_RESULT(IO80211Interface *,IO80211VirtualInterface *,IO80211SkywalkInterface *,apple80211_assoc_result_data *);
    virtual int errnoFromReturn(int) override;
    virtual const char* stringFromReturn(int) override;
    virtual SInt32 apple80211_ioctl_set(IO80211SkywalkInterface *,void *);
    virtual SInt32 apple80211_ioctl_set(IO80211Interface *,IO80211VirtualInterface *,ifnet_t,void *);
    virtual SInt32 apple80211_ioctl_set(IO80211Interface *,IO80211VirtualInterface *,IO80211SkywalkInterface *,void *);
    virtual SInt32 apple80211_ioctl_get(IO80211SkywalkInterface *,void *);
    virtual SInt32 apple80211_ioctl_get(IO80211Interface *,IO80211VirtualInterface *,ifnet_t,void *);
    virtual SInt32 apple80211_ioctl_get(IO80211Interface *,IO80211VirtualInterface *,IO80211SkywalkInterface *,void *);
    virtual IOReturn copyIn(ulong long,void *,ulong);
    virtual void logIOCTL(apple80211req *);
    virtual bool isIOCTLLoggingRestricted(apple80211req *);
    virtual void inputMonitorPacket(mbuf_t,uint,void *,ulong);//怀疑对象，之前是返回int
    virtual SInt32 apple80211_ioctl(IO80211SkywalkInterface *,ulong,void *);
    virtual SInt32 apple80211_ioctl(IO80211Interface *,IO80211VirtualInterface *,ifnet_t,ulong,void *);
    virtual IONetworkInterface* createInterface(void) override;
    virtual IOReturn getHardwareAddress(IOEthernetAddress *) override;
    virtual IO80211SkywalkInterface* getPrimarySkywalkInterface(void);
    virtual IO80211Interface* getNetworkInterface(void);
    virtual IOWorkLoop* getWorkLoop(void);
    virtual bool createWorkLoop(void) override;
    virtual mbuf_flags_t inputPacket(mbuf_t);
    virtual IOReturn outputStart(IONetworkInterface *,uint);
    virtual IOReturn setChanNoiseFloorLTE(apple80211_stat_report *,int);
    virtual IOReturn setChanNoiseFloor(apple80211_stat_report *,int);
    virtual IOReturn setChanCCA(apple80211_stat_report *,int);
    virtual IOReturn setChanExtendedCCA(apple80211_stat_report *,apple80211_cca_report *);
    virtual bool setLTECoexstat(apple80211_stat_report *,apple80211_lteCoex_report *);
    virtual bool setBTCoexstat(apple80211_stat_report *,apple80211_btCoex_report *);
    virtual bool setAMPDUstat(apple80211_stat_report *,apple80211_ampdu_stat_report *,apple80211_channel *);
    virtual UInt32 getCountryCode(apple80211_country_code_data *);
    virtual IOReturn setCountryCode(apple80211_country_code_data *);
    virtual bool getInfraExtendedStats(apple80211_extended_stats *);
    virtual bool getChipCounterStats(apple80211_chip_stats *);
    virtual bool setExtendedChipCounterStats(apple80211_stat_report *,void *);
    virtual bool setChipCounterStats(apple80211_stat_report *,apple80211_chip_stats *,apple80211_channel *);
    virtual bool setLeakyAPStats(apple80211_leaky_ap_event *);
    virtual bool setFrameStats(apple80211_stat_report *,apple80211_frame_counters *,apple80211_channel *);
    virtual bool setPowerStats(apple80211_stat_report *,apple80211_power_debug_sub_info *);
    virtual bool getBeaconPeriod(apple80211_beacon_period_data *);
    virtual SInt32 apple80211VirtualRequestIoctl(uint,int,IO80211VirtualInterface *,void *);
    virtual bool getBSSIDData(OSObject *,apple80211_bssid_data *);
    virtual bool getSSIDData(apple80211_ssid_data *);
    virtual IOOutputQueue* getOutputQueue(void);
    virtual bool inputInfraPacket(mbuf_t);
    virtual void notifyHostapState(apple80211_hostap_state *);
    virtual bool isAwdlAssistedDiscoveryEnabled(void);
    virtual void joinDone(scanSource,joinStatus);//怀疑对象，之前是返回int
    virtual void joinStarted(scanSource,joinStatus);//怀疑对象，之前是返回int
    virtual void handleChannelSwitchAnnouncement(apple80211_channel_switch_announcement *);
    virtual void scanDone(scanSource,int);
    virtual void scanStarted(scanSource,apple80211_scan_data *);
    virtual void printChannels(void);
    virtual void updateInterfaceCoexRiskPct(ulong long);
    virtual SInt32 getInfraChannel(apple80211_channel_data *);
    virtual void calculateInterfacesAvaiability(void);//怀疑对象，之前是返回int
    virtual void setChannelSequenceList(apple80211_awdl_sync_channel_sequence *);//怀疑对象，之前是返回int
    virtual void setPrimaryInterfaceDatapathState(bool);
    virtual UInt32 getPrimaryInterfaceLinkState(void);
    virtual void setCurrentChannel(apple80211_channel *);//怀疑对象，之前是返回int
    virtual void setHtCapability(ieee80211_ht_capability_ie *);//怀疑对象，之前是返回int
    virtual UInt32 getHtCapability(void);//之前是IO80211Controller
    virtual UInt32 getHtCapabilityLength(void);
    virtual bool io80211isDebuggable(bool *);
    virtual UInt32 selfDiagnosticsReport(int,char const*,uint);
    virtual void logDebug(ulong long,char const*,...);//怀疑对象，之前是返回int
    virtual void vlogDebug(ulong long,char const*,va_list);//怀疑对象，之前是返回char
    virtual void logDebug(char const*,...);//怀疑对象，之前是返回int
    virtual void releaseFlowQueue(IO80211FlowQueue *);//怀疑对象，之前是返回char
    virtual IO80211FlowQueueLegacy* requestFlowQueue(FlowIdMetadata const*);
    virtual bool calculateInterfacesCoex(void);
    virtual void setInfraChannel(apple80211_channel *);//怀疑对象，之前是返回char
    virtual bool configureInterface(IONetworkInterface *) override;
    virtual IOReturn disable(IO80211SkywalkInterface *);
    virtual IOReturn enable(IO80211SkywalkInterface *);
    virtual IOReturn disable(IONetworkInterface *) override;
    virtual void configureAntennae(void);
    virtual SInt32 apple80211RequestIoctl(uint,int,IO80211Interface *,void *);
    virtual UInt32 radioCountForInterface(IO80211Interface *);
    virtual IOReturn enable(IONetworkInterface *) override;
    virtual void releaseIOReporters(void);//怀疑对象，之前是返回int
    virtual void stop(IOService *) override;
    virtual void free(void) override;
    virtual bool init(OSDictionary *) override;
    virtual bool findAndAttachToFaultReporter(void);
    virtual UInt32 setupControlPathLogging(void);
    virtual IOReturn createIOReporters(IOService *);
    virtual IOReturn powerChangeHandler(void *,void *,uint,IOService *,void *,ulong);
    virtual bool start(IOService *) override;
    
    OSMetaClassDeclareReservedUnused( IO80211Controller,  0);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  1);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  2);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  3);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  4);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  5);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  6);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  7);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  8);
    OSMetaClassDeclareReservedUnused( IO80211Controller,  9);
    OSMetaClassDeclareReservedUnused( IO80211Controller, 10);
    OSMetaClassDeclareReservedUnused( IO80211Controller, 11);
    OSMetaClassDeclareReservedUnused( IO80211Controller, 12);
    OSMetaClassDeclareReservedUnused( IO80211Controller, 13);
    OSMetaClassDeclareReservedUnused( IO80211Controller, 14);
    OSMetaClassDeclareReservedUnused( IO80211Controller, 15);
    
    
    
//    virtual IOReturn outputStart(IONetworkInterface* interface,
//                                 unsigned int options);
//    virtual IO80211Interface * getNetworkInterface() ;
//    virtual IOReturn getHardwareAddressForInterface(IO80211Interface*, IOEthernetAddress*) ;
//    virtual SInt32 apple80211Request(unsigned int, int, IO80211Interface*, void*) = 0;
//    virtual UInt32 apple80211SkywalkRequest(uint,int, IO80211SkywalkInterface *,void *);
//    virtual SInt32 setVirtualHardwareAddress(IO80211VirtualInterface*, ether_addr*);
//    virtual SInt32 enableVirtualInterface(IO80211VirtualInterface*);
//    virtual SInt32 disableVirtualInterface(IO80211VirtualInterface*);
//    virtual UInt32 getDataQueueDepth(OSObject*) ;
  
    
    
    
    
//    virtual IOReturn getHardwareAddress(void *   addr,
//                                        UInt32 * inOutAddrBytes) override;
    
//    virtual IOReturn getHardwareAddress(IOEthernetAddress *);
    
protected:
    static IORegistryPlane gIO80211Plane;
    
    
};

#endif /* defined(KERNEL) && defined(__cplusplus) */

#endif /* !_IO80211CONTROLLER_H */
