//
//  HackIO80211Interface.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/2/10.
//  Copyright © 2020 钟先耀. All rights reserved.
//

#include "HackIO80211Interface.h"
#include <IOKit/IOReturn.h>
#include <IOKit/IOLib.h>
#include <IOKit/network/IOOutputQueue.h>
#include <net/if_types.h>

#define super IOEthernetInterface

bool HackIO80211Interface::terminate(unsigned int options)
{
    return super::terminate(options);
}

bool HackIO80211Interface::attach(IOService* provider)
{
    return super::attach(provider);
}

void HackIO80211Interface::detach(IOService* provider)
{
    super::detach(provider);
}

bool HackIO80211Interface::init(IONetworkController* provider)
{
    return super::init(provider);
}

UInt32 HackIO80211Interface::inputPacket(mbuf_t          packet,
                                         UInt32          length,
                                         IOOptionBits    options,
                                         void *          param)
{
    
    return super::inputPacket(packet, length, options, param);
}

bool HackIO80211Interface::configureInterface(IOEthernetController *controller)
{
//    #define asm_cast(var,addr)  \
//    {                           \
//        __asm{                  \
//            mov var,offset addr \
//        }                       \
//    }
//    char *p4 = 0;
//    asm_cast(p4, m);
//    p4 += 8;
//    __asm {
//        push rax
//        push rbx
////        push 3
////        push controller
//        mov rdi, controller
//        mov esi, 3
//        mov rbx, offset m
//        add rbx, 8
//        call rbx
////        add rsp, 16
//        pop rbx
//        pop rax
//    }
    setInterfaceSubType(3);
    return true;
}

bool HackIO80211Interface::inputEvent(unsigned int type, void* data)
{
    return super::inputEvent(type, data);
}

SInt32 HackIO80211Interface::performCommand(IONetworkController* controller, unsigned long cmd, void* arg0, void* arg1)
{
    IOLog("AppleIntel cmd=%ld\n", cmd);
    switch (cmd) {
        case 2149607880LL:
        case 2150132168LL:
        case 3223873993LL:
            return controller->executeCommand(this, OSMemberFunctionCast(IONetworkController::Action, this, &HackIO80211Interface::performGatedCommand), this, controller, &cmd, arg0, arg1);
            break;
        default:
            if (cmd <= 3223349704LL) {
                return controller->executeCommand(this, OSMemberFunctionCast(IONetworkController::Action, this, &HackIO80211Interface::performGatedCommand), this, controller, &cmd, arg0, arg1);
            }
            break;
    }
    return super::performCommand(controller, cmd, arg0, arg1);
}

UInt64 HackIO80211Interface::IO80211InterfaceUserSpaceToKernelApple80211Request(void *arg, apple80211req *req, unsigned long ctl)
{
    UInt64 result;
    UInt32 v5;
    UInt64 *a1 = (UInt64 *)arg;
    if ( ctl != 3223873993LL && ctl != 2150132168LL ) {
        *(UInt64 *)&req->req_if_name[8] = a1[1];
        *(UInt64 *)req->req_if_name = *a1;
        req->req_type = *((UInt32 *)a1 + 4);
        req->req_val = *((UInt32 *)a1 + 5);
        req->req_len = *((UInt32 *)a1 + 6);
        result = *((unsigned int *)a1 + 7);
        v5 = 4;
    } else {
        *(UInt64 *)&req->req_if_name[8] = a1[1];
        *(UInt64 *)req->req_if_name = *a1;
        req->req_type = *((UInt32 *)a1 + 4);
        req->req_val = *((UInt32 *)a1 + 5);
        req->req_len = *((UInt32 *)a1 + 6);
        result = a1[4];
        v5 = 8;
    }
    req->req_data = (void *)result;
    *(UInt32 *)req[1].req_if_name = v5;
    return result;
}

int HackIO80211Interface::performGatedCommand(void *a2, void *a3, void *a4, void *a5, void *a6)
{
    IOLog("AppleIntel performGatedCommand\n");
    apple80211req req;
    UInt64 method;
    if (!a2) {
        return 22LL;
    }
    UInt64 ctl = *(UInt64 *)a3;
    bzero(&req, sizeof(apple80211req));
    IO80211InterfaceUserSpaceToKernelApple80211Request(a5, &req, ctl);
    if ((ctl | 0x80000) == 2150132168LL) {
        method = IOCTL_SET;
    } else {
        method = IOCTL_GET;
    }
    return apple80211_ioctl(this, method, &req);
}

int HackIO80211Interface::apple80211_ioctl(HackIO80211Interface *netif, UInt64 method, apple80211req *a6)
{
    if (method == IOCTL_GET) {
        return apple80211_ioctl_get(netif, a6);
    } else {
        return apple80211_ioctl_set(netif, a6);
    }
    return 102LL;
}

int HackIO80211Interface::apple80211_ioctl_set(HackIO80211Interface *netif, apple80211req *a6)
{
    IOLog("AppleIntel apple80211_ioctl_set\n");
    uint32_t index = a6->req_type - 1;
    if (index > 0x160) {
        return 102;
    }
    return 102;
}

int HackIO80211Interface::apple80211_ioctl_get(HackIO80211Interface *netif, apple80211req *a6)
{
    IOLog("AppleIntel apple80211_ioctl_get\n");
    uint32_t index = a6->req_type - 1;
    if (index > 0x160) {
        return 102;
    }
    return 102;
}

IOReturn HackIO80211Interface::attachToDataLinkLayer(IOOptionBits, void*)
{
    IOLog("AppleIntel attachToDataLinkLayer\n");
    return kIOReturnSuccess;
}

void HackIO80211Interface::detachFromDataLinkLayer(unsigned int, void*)
{
    IOLog("AppleIntel detachFromDataLinkLayer\n");
}

void HackIO80211Interface::setPoweredOnByUser(bool)
{
    IOLog("AppleIntel setPoweredOnByUser\n");
}

void HackIO80211Interface::setEnabledBySystem(bool)
{
    IOLog("AppleIntel setEnabledBySystem\n");
}

bool HackIO80211Interface::setLinkState(IO80211LinkState, unsigned int)
{
    IOLog("AppleIntel setLinkState1\n");
    return true;
}

bool HackIO80211Interface::setLinkState(IO80211LinkState, int, unsigned int)
{
    IOLog("AppleIntel setLinkState2\n");
    return true;
}

UInt32 HackIO80211Interface::outputPacket(mbuf_t m, void* param)
{
    if (!getController()) {
        return kIOReturnOutputDropped;
    }
    return getController()->outputPacket(m, param);
}

bool HackIO80211Interface::setLinkQualityMetric(int)
{
    IOLog("AppleIntel setLinkQualityMetric\n");
    return true;
}

void HackIO80211Interface::handleDebugCmd(apple80211_debug_command* )
{
    
}
