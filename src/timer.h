/*************************************************************************
	> File Name: timer.h
	> Author: 
	> Mail: 
	> Created Time: Fri 30 Dec 2016 07:04:19 PM CST
 ************************************************************************/

#ifndef _TIMER_H
#define _TIMER_H

#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <chrono>
#include <functional>
#include <experimental/any>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>

#include "vnet_loop.h"

using namespace std;
using std::experimental::any;
using std::experimental::any_cast;

namespace vnet {

class Timer {

public:

    typedef function<void(Timer *timer, uint32_t interval, any ctx)> TimerCb;

    Timer(VNetLoop *loop, uint32_t interval, TimerCb cb, any ctx, bool isOneShot = false);
    ~Timer();

    bool start();
    void stop();

protected:

    VNetLoop *     _loop;
    uint32_t       _interval;
    TimerCb        _cb;
    any            _ctx;
    bool           _isOneShot;
    struct event   _timeout;
    bool           _status;

    static void timeoutCb(evutil_socket_t fd, short event, void *arg);
};

}

#endif
