/*************************************************************************
	> File Name: timer.cpp
	> Author: 
	> Mail: 
	> Created Time: Fri 30 Dec 2016 07:04:25 PM CST
 ************************************************************************/

#include <cassert>
#include<iostream>
#include "timer.h"
using namespace std;

namespace vnet {

Timer::Timer(VNetLoop *loop, uint32_t interval, TimerCb cb, any ctx, bool isOneShot):
                _loop(loop), _interval(interval), _cb(cb), _ctx(ctx),
                _isOneShot(isOneShot), _status(false)  {


}

Timer::~Timer() {
    stop();
}

bool Timer::start() {

    bool ret = true;
    if(_loop) {

        struct timeval tv;
        
        if(event_assign(&_timeout, 
                    _loop->getVNetLoopTcp()->getEventBase(), -1, 0 , timeoutCb, static_cast<void*>(this))) {
            ret = false;
        }

        evutil_timerclear(&tv);
        tv.tv_sec = _interval / 1000;
        tv.tv_usec = (_interval % 1000) * 1000;
        if(event_add(&_timeout, &tv)) {
            ret = false;
        }
    }

    if(true == ret) {
        _status = true; 
    }

    return ret;
}

void Timer::stop() {

    if(_status) {
        event_del(&_timeout);
        _status = false;
    }
}

void Timer::timeoutCb(evutil_socket_t fd, short event, void *arg) {

    assert(arg);
    Timer *timer = static_cast<Timer *>(arg);
    if(!timer->_status) {
        return; 
    }
    if(timer->_cb) {
        timer->_cb(timer, timer->_interval, timer->_ctx);
    }
    if(!timer->_isOneShot) {

        struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_sec = timer->_interval / 1000;
        tv.tv_usec = (timer->_interval % 1000) * 1000;
        if(event_add(&timer->_timeout, &tv)) {
            timer->_status = false; 
        } 
    } else {
        timer->_status = false; 
    }
}

}

