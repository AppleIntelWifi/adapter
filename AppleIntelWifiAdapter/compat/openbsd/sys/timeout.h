/*	$OpenBSD: timeout.h,v 1.29 2019/07/12 00:04:59 cheloha Exp $	*/
/*
 * Copyright (c) 2000-2001 Artur Grabowski <art@openbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_TIMEOUT_H_
#define _SYS_TIMEOUT_H_

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>            /* _Q_INVALIDATE */
#include <sys/sysctl.h>
#include <sys/_buf.h>
#include <sys/kernel.h>

#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOLocks.h>

#define _KERNEL

class timeout : public OSObject {
    OSDeclareDefaultStructors(timeout)
public:
    
    static void initTimeout(IOWorkLoop *workloop)
    {
        fWorkloop = workloop;
    }
    
    static void releaseTimeout()
    {
        fWorkloop = NULL;
    }
    
    void timeoutOccurred(OSObject* owner, IOTimerEventSource* timer)
    {
        timeout *tm = (timeout*)owner;
        //callback
        tm->to_func(tm->to_arg);
        fWorkloop->removeEventSource(tm->tm);
        OSSafeReleaseNULL(tm->tm);
    }
    
    static void timeout_set(timeout **t, void (*fn)(void *), void *arg)
    {
        if (*t == NULL) {
            *t = new timeout();
        }
        ((timeout*)*t)->to_func = fn;
        ((timeout*)*t)->to_arg = arg;
    }
    
    static int timeout_add_msec(timeout **to, int msecs)
    {
        if (*to == NULL) {
            *to = new timeout();
        }
        ((timeout*)*to)->tm = IOTimerEventSource::timerEventSource(((timeout*)*to), OSMemberFunctionCast(IOTimerEventSource::Action, ((timeout*)*to), &timeout::timeoutOccurred));
        if (((timeout*)*to)->tm == 0)
            return;
        fWorkloop->addEventSource(((timeout*)*to)->tm);
        ((timeout*)*to)->tm->enable();
        ((timeout*)*to)->tm->setTimeoutMS(msecs);
        return 1;
    }
    
    static int timeout_add_sec(timeout **to, int secs)
    {
        return timeout_add_msec(to, secs * 1000);
    }
    
    static int timeout_add_usec(timeout **to, int usecs)
    {
        return timeout_add_msec(to, (int) usecs / 1000);
    }
    
    static int timeout_del(timeout **to)
    {
        if (!((timeout*)*to) || !((timeout*)*to)->tm) {
            return;
        }
        ((timeout*)*to)->tm->cancelTimeout();
        if (fWorkloop) {
            fWorkloop->removeEventSource(((timeout*)*to)->tm);
        }
        OSSafeReleaseNULL(((timeout*)*to)->tm);
    }
    
    static int timeout_pending(timeout **to)
    {
        if (!((timeout*)*to) || !((timeout*)*to)->tm || !((timeout*)*to)->tm->isEnabled()) {
            return 0;
        }
        return 1;
    }
    
    static int splnet()
    {
        fWorkloop->disableAllInterrupts();
        fWorkloop->disableAllEventSources();
        return 1;
    }
    
    static void splx(int s)
    {
        fWorkloop->enableAllInterrupts();
        fWorkloop->enableAllEventSources();
    }
    
public:
    IOTimerEventSource* tm;
    void (*to_func)(void *);        /* function to call */
    void *to_arg;                /* function argument */
private:
    static IOWorkLoop *fWorkloop;
};

#endif	/* _SYS_TIMEOUT_H_ */
