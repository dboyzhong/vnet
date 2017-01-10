/*************************************************************************
	> File Name: vnet_loop.h
	> Author: 
	> Mail: 
	> Created Time: Wed 21 Dec 2016 08:57:03 PM CST
 ************************************************************************/

#ifndef _VNET_LOOP_H
#define _VNET_LOOP_H

#include <event2/event.h>
#include <event2/bufferevent.h>

#include <map>
#include <memory>
#include <mutex>
#include <future>
#include <condition_variable>

#include <udx/udxos.h>
#include <udx/FastUdx.h>
#include "vnet_def.h"

namespace vnet {

class VNetConnection;
class VNetLoopTcp;
class VNetLoopUdx;

/*
* thread safe network library
* base loop for event
* support tcp and udx
* tcp:implement using libevent, udx:implement using fastUdx
* */
class VNetLoop {

public:

    VNetLoop();
    ~VNetLoop();
    shared_ptr<VNetLoopTcp> getVNetLoopTcp();
    shared_ptr<VNetLoopUdx> getVNetLoopUdx();
    
    /*
    * start loop, thread safe
    * */
    bool loop();


    /*
    * break loop, not thread safe
    * */
    void exit();

    /*
    * get certain VNetConnection obj, thread safe
    * */
    shared_ptr<VNetConnection> getVNetConnection(uint64_t index);

    /*
    * called by TcpServer/UdxServer or UdxServer/UdxClient, do not call directlly thread safe
    * */
    bool addVNetConnection(uint64_t index, const std::shared_ptr<VNetConnection> & connPtr);

    /*
    * called by TcpServer/UdxServer or UdxServer/UdxClient, do not call directlly thread safe
    * */
    void removeVNetConnection(uint64_t index);

protected:
    std::map<uint64_t, std::shared_ptr<VNetConnection> > _conns;
    shared_ptr<VNetLoopTcp>                              _loopTcpPtr;
    shared_ptr<VNetLoopUdx>                              _loopUdxPtr;
    mutex                                                _connsMtx;
};

class VNetLoopTcp {

public:

    VNetLoopTcp(VNetLoop * loop);
    ~VNetLoopTcp();
    struct event_base * getEventBase();
    
    bool loop();
    void exit();
    bool addVNetConnection(uint64_t index, const std::shared_ptr<VNetConnection> & connPtr);
    bool closeVNetConnection(uint64_t index);
    void removeVNetConnection(uint64_t index);

protected:

    static void readCb(struct bufferevent *bev, void *ctx);
    static void writeCb(struct bufferevent *bev, void *ctx);
    static void eventCb(struct bufferevent *bev, short what, void *ctx);

protected:
    struct event_base *                                  _base;
    struct bufferevent*                                  _bevPair[2];
    uint64_t                                             _connIdxBuf[256];
    uint32_t                                             _connIdxBufEnd;
    VNetLoop *                                           _loop;
};

class VNetLoopUdx {

public:
    VNetLoopUdx(VNetLoop * loop);
    bool loop();
    void exit();

protected:
    VNetLoop  *             _loop;
    std::promise<int>       _pro;
};

}


#endif
