/*************************************************************************
	> File Name: vnet_loop.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 22 Dec 2016 08:17:29 PM CST
 ************************************************************************/

#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include<iostream>
#include "vnet_loop.h"
#include "vnet.h"
#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/thread.h>
using namespace std;

namespace vnet {

VNetLoop::VNetLoop() {
    _loopTcpPtr = make_shared<VNetLoopTcp>(this);
    _loopUdxPtr = make_shared<VNetLoopUdx>(this);
}

VNetLoop::~VNetLoop() {

}

shared_ptr<VNetLoopTcp> VNetLoop::getVNetLoopTcp() {
    return _loopTcpPtr; 
}

shared_ptr<VNetLoopUdx> VNetLoop::getVNetLoopUdx() {
    return _loopUdxPtr;
}

bool VNetLoop::loop() {
    
    bool ret = false;
    if(_loopTcpPtr) {
        ret = _loopTcpPtr->loop(); 
    } else {
        ret = true; 
    }
    if(ret && _loopUdxPtr) {
        ret = _loopUdxPtr->loop(); 
    }
    return ret;
}

bool VNetLoop::addVNetConnection(uint64_t index, const std::shared_ptr<VNetConnection> & connPtr) {

    lock_guard<mutex> lck(_connsMtx);

    if(_conns.find(index) != _conns.end()) {
        return false; 
    } else {
        _conns[index] = connPtr; 
        return true;
    }
}

shared_ptr<VNetConnection> VNetLoop::getVNetConnection(uint64_t index) {

    lock_guard<mutex> lck(_connsMtx);

    if(_conns.find(index) != _conns.end()) {
        return nullptr; 
    } else {
        return _conns[index];
    }
}


void VNetLoop::removeVNetConnection(uint64_t index) {

    lock_guard<mutex> lck(_connsMtx);

    cout << "vnet remove conn:" << index << endl;
    if(_conns.find(index) != _conns.end()) {

        _conns[index]->release();
        _conns.erase(index); 
    }
}

void VNetLoop::exit() {

    if(_loopTcpPtr) {
        _loopTcpPtr->exit(); 
    }

    if(_loopUdxPtr) {
        _loopUdxPtr->exit(); 
    }
}

VNetLoopTcp::VNetLoopTcp(VNetLoop * loop):_base(nullptr), _connIdxBufEnd(0), _loop(loop) {

    _bevPair[0] = nullptr;
    _bevPair[1] = nullptr;

    memset(_connIdxBuf, 0, sizeof(_connIdxBuf));
}


VNetLoopTcp::~VNetLoopTcp() {
    exit();
}


struct event_base *VNetLoopTcp::getEventBase() {

    if(!_base) {
        evthread_use_pthreads();
        _base = event_base_new(); 
        if(bufferevent_pair_new(_base,
                    BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS,
                    _bevPair)) {
            event_base_free(_base); 
            _base = nullptr;
        } else {
            assert(_bevPair[0]);
            assert(_bevPair[1]);
            bufferevent_setcb(_bevPair[0], readCb, nullptr, eventCb, this);
            bufferevent_setcb(_bevPair[1], nullptr, writeCb, eventCb, this);
            bufferevent_enable(_bevPair[0], EV_READ);
            bufferevent_enable(_bevPair[1], EV_WRITE);
        }
    }

    return _base;
}


bool VNetLoopTcp::loop() {

    if(_base) {
        event_base_dispatch(_base); 
    } 
    return true;
}



bool VNetLoopTcp::addVNetConnection(uint64_t index, const std::shared_ptr<VNetConnection> & connPtr) {
    return _loop->addVNetConnection(index, connPtr);
}

void VNetLoopTcp::removeVNetConnection(uint64_t index) {
    return _loop->removeVNetConnection(index);
}

bool VNetLoopTcp::closeVNetConnection(uint64_t index) {

    bool ret = true;

    uint64_t buf = index;

    bufferevent_lock(_bevPair[1]);

    evbuffer *out_buf = bufferevent_get_output(_bevPair[1]);
    if(out_buf) {
         
        if(evbuffer_add(out_buf, static_cast<void *>(&buf), sizeof(buf))) {
            ret = false; 
        }
    } else {
        ret = false; 
    }

    bufferevent_unlock(_bevPair[1]);
    return ret;
}


void VNetLoopTcp::readCb(struct bufferevent *bev, void *ctx) {


    VNetLoopTcp *loop  = static_cast<VNetLoopTcp*>(ctx);
    evbuffer * input_buf = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input_buf);
    size_t byte_read = 0;
    uint64_t *p = static_cast<uint64_t *>(loop->_connIdxBuf);

    if(0 == len) {
        return; 
    }

    while((byte_read = evbuffer_remove(input_buf, static_cast<void *>(loop->_connIdxBuf),
                                       sizeof(loop->_connIdxBuf) - loop->_connIdxBufEnd))) {
         
        for(unsigned int i = 0; i < byte_read / (sizeof(uint64_t)); i++) {
            loop->removeVNetConnection((loop->_connIdxBuf[i]));
        }
        loop->_connIdxBufEnd = (byte_read % sizeof(uint64_t));
        memmove(reinterpret_cast<char *>(p), 
                (&(reinterpret_cast<char *>(p))[byte_read - loop->_connIdxBufEnd]), 
                loop->_connIdxBufEnd);
    }
}


void VNetLoopTcp::writeCb(struct bufferevent *bev, void *ctx) {

}


void VNetLoopTcp::eventCb(struct bufferevent *bev, short what, void *ctx) {

}

void VNetLoopTcp::exit() {
    if (_base) {
        event_base_loopbreak(_base); 
        event_base_free(_base); 
        _base = nullptr;
    }
}

VNetLoopUdx::VNetLoopUdx(VNetLoop * loop): _loop(loop) {
}

bool VNetLoopUdx::loop() {
    bool ret = true;
    future<int> fut = _pro.get_future();
    int res = fut.get();
    printf("loop exit with err_code:%d\n", res);
    return ret;
}

void VNetLoopUdx::exit() {

    try {
        _pro.set_value(0);
    } catch (const std::future_error & e) {
        printf("promise set error:%s", e.what()); 
    }
}


}
