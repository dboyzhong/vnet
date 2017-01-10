/*************************************************************************
	> File Name: vnet_tcp.h
	> Author: 
	> Mail: 
	> Created Time: Wed 21 Dec 2016 09:07:52 PM CST
 ************************************************************************/

#ifndef _VNET_TCP_H
#define _VNET_TCP_H

#include <inttypes.h>
#include <string>
#include <functional>
#include "vnet_tcp_loop.h"
#include "vnet.h"
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/util.h>

using namespace std;

namespace vnet {

class TcpConnection : public VNetConnection {

public:

    friend class TcpClient;
    friend class TcpServer;

    TcpConnection(VNetLoop *loop, struct event_base *base, 
            const string &peerIp, uint16_t peerPort, shared_ptr<VNetClient> vnetClient);
    TcpConnection(VNetLoop *loop, struct event_base *base, evutil_socket_t fd,  
            const string &peerIp, uint16_t peerPort, shared_ptr<VNetServer> vnetServer);
    ~TcpConnection();

    bool connect() override;
    bool accept();
    void close() override;
    void release() override;
    bool send(const void*buf, uint32_t size) override;
    uint32_t read(void *buf, uint32_t buf_len) override;
    uint32_t getInputBufferLen() const override;
    uint32_t getOutputBufferLen() const override;

    void setConnectCb(ConnectCb cb) override;
    void setReadCb(ReadCb cb) override;
    void setWriteCb(WriteCb cb) override;
    void setErrorCb(ErrorCb cb) override;
    struct bufferevent *getBufferEvent();

protected:
    
    static void readCb(struct bufferevent *bev, void *ctx);
    static void writeCb(struct bufferevent *bev, void *ctx);
    static void eventCb(struct bufferevent *bev, short what, void *ctx);

    void readCbImp(struct bufferevent *bev);
    void writeCbImp(struct bufferevent *bev);
    void connectCbImp(struct bufferevent *bev);
    void errorCbImp(struct bufferevent *bev, int error);

private:
    struct event_base  * _base;
    struct bufferevent * _bev;
    string               _peerIp;
    uint16_t             _peerPort;

    ConnectCb            _connectCb;
    ReadCb               _readCb;
    WriteCb              _writeCb;
    ErrorCb              _errorCb;
};

class TcpClient : public VNetClient ,public enable_shared_from_this<TcpClient> {

public:
    TcpClient(VNetLoop *loop);

    bool connect(const string & peerIp, uint16_t peerPort, const any &ctx = nullptr) override;

protected:

};

class TcpServer : public VNetServer, public enable_shared_from_this<TcpServer> {

public:
    TcpServer(VNetLoop *loop, const string &serverIP, uint16_t serverPort);
    virtual ~TcpServer();
    
    bool start(void) override;

protected:

    static void acceptCb(struct evconnlistener *listener, evutil_socket_t fd,
                         struct sockaddr *a, int slen, void *p);

private:
    struct evconnlistener * _listener;
};

}

#endif
