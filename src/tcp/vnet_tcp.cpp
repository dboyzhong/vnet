/*************************************************************************
	> File Name: vnet_tcp.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 22 Dec 2016 08:23:56 PM CST
 ************************************************************************/

#include <stdio.h>
#include <cstring>
#include <cassert>
#include<iostream>
#include "vnet_tcp.h"
#include "vnet.h"
#include <event2/util.h>
#include <event2/buffer.h>
using namespace std;

namespace vnet {

TcpConnection::TcpConnection(VNetLoop *loop, struct event_base *base, const string &peerIp, uint16_t peerPort, 
        shared_ptr<VNetClient> vnetClient):VNetConnection(loop, vnetClient), _base(base), _bev(nullptr),
                                         _peerIp(peerIp), _peerPort(peerPort) {

    assert(base);
    _bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE|BEV_OPT_DEFER_CALLBACKS);
    _direction = ConnDirection::DIRECTION_OUT;
}

TcpConnection::TcpConnection(VNetLoop *loop, struct event_base *base, evutil_socket_t fd, 
        const string &peerIp, uint16_t peerPort, shared_ptr<VNetServer> vnetServer):
                VNetConnection(loop, vnetServer), _base(base), _bev(nullptr), 
                _peerIp(peerIp), _peerPort(peerPort) {

    assert(base);
    _bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE|BEV_OPT_DEFER_CALLBACKS);

    assert(_bev);    //consider how to handle _bev == nullptr case
    _direction = ConnDirection::DIRECTION_IN;
}

TcpConnection::~TcpConnection() {
    printf("tcp connection release with index:%lu\n", _index);
    if(_bev) {
        bufferevent_free(_bev);
        _bev = nullptr;
    }
}

bool TcpConnection::connect() {

    printf("_bev:%p, _direction:%d\n", _bev, static_cast<int>(_direction));
    if((!_bev) || (_direction != ConnDirection::DIRECTION_OUT)) {
        return false;
    }

    bool ret = true;
    struct sockaddr addr;
    int addrlen = 0;

    memset(&addr, 0, sizeof(addr));
    addrlen = sizeof(addr);

    if(evutil_parse_sockaddr_port((_peerIp + ':' + to_string(_peerPort)).c_str(),
                    &addr, &addrlen) < 0) {
        ret = false; 
    }

    if (bufferevent_socket_connect(_bev, &addr, addrlen) < 0) {
        ret = false;
    }

    bufferevent_setcb(_bev, readCb, writeCb, eventCb, this);
    bufferevent_enable(_bev, EV_READ|EV_WRITE);

    return ret;
}

bool TcpConnection::accept() {

    if((!_bev) || (_direction != ConnDirection::DIRECTION_IN)) {
        return false;
    }

    _state = ConnectionState::E_CONNECTED;
    bufferevent_setcb(_bev, readCb, writeCb, eventCb, this);
    bufferevent_enable(_bev, EV_READ|EV_WRITE);
    return true;
}

void TcpConnection::close() {

    assert(_bev);

    bufferevent_lock(_bev);

    if(_state != ConnectionState::E_CLOSED) { 

        _state = ConnectionState::E_CLOSED;
        bufferevent_flush(_bev, EV_READ|EV_WRITE, BEV_NORMAL);
        bufferevent_disable(_bev, EV_READ|EV_WRITE);
    }

    if(_loop) {
        _loop->getVNetLoopTcp()->closeVNetConnection(_index);
    }
    bufferevent_unlock(_bev);
}

void TcpConnection::release() {

}

bool TcpConnection::send(const void *buf, uint32_t size) {

    bool ret = true;

    assert(_bev);

    bufferevent_lock(_bev);
    if(_state != ConnectionState::E_CONNECTED) {
        ret = false;
    } else {

        evbuffer *out_buf = bufferevent_get_output(_bev);
        if(out_buf) {
             
            if(evbuffer_add(out_buf, buf, size)){
                ret = false; 
            }
        } else {
            ret = false; 
        }
    }
    bufferevent_unlock(_bev);

    return ret;
} 

uint32_t TcpConnection::read(void *buf, uint32_t buf_len) {

    uint32_t ret = 0;
    if(_bev) {
        bufferevent_lock(_bev);
        ret = evbuffer_remove(bufferevent_get_input(_bev), buf, buf_len);
        bufferevent_unlock(_bev);
    }
    return ret;
}

uint32_t TcpConnection::getInputBufferLen() const {

    uint32_t ret = 0;
    if(_bev) {
        bufferevent_lock(_bev);
        ret = static_cast<uint32_t>(evbuffer_get_length(bufferevent_get_input(_bev)));
        bufferevent_unlock(_bev);
    }
    return ret;
}

uint32_t TcpConnection::getOutputBufferLen() const {

    uint32_t ret = 0;
    if(_bev) {
        bufferevent_lock(_bev);
        ret = static_cast<uint32_t>(evbuffer_get_length(bufferevent_get_output(_bev)));
        bufferevent_unlock(_bev);
    }
    return ret;
}

void TcpConnection::setConnectCb(ConnectCb cb) {
    _connectCb = cb;
}

void TcpConnection::setReadCb(ReadCb cb) {
    _readCb = cb;
}

void TcpConnection::setWriteCb(WriteCb cb) {
    _writeCb = cb;
}

void TcpConnection::setErrorCb(ErrorCb cb) {
    _errorCb = cb;
}

void TcpConnection::readCb(struct bufferevent *bev, void *ctx) {

    TcpConnection *conn = static_cast<TcpConnection*>(ctx);
    conn->readCbImp(bev);
}

void TcpConnection::writeCb(struct bufferevent *bev, void *ctx) {

    TcpConnection *conn = static_cast<TcpConnection*>(ctx);
    conn->writeCbImp(bev);
}

void TcpConnection::eventCb(struct bufferevent *bev, short what, void *ctx) {

    TcpConnection *conn = static_cast<TcpConnection*>(ctx);

    if (what & (BEV_EVENT_EOF|BEV_EVENT_ERROR | BEV_EVENT_READING |
                BEV_EVENT_WRITING | BEV_EVENT_TIMEOUT)) {
        conn->errorCbImp(bev, what);
    } else if(what & BEV_EVENT_CONNECTED) {
        conn->connectCbImp(bev);
    } 
}

void TcpConnection::readCbImp(struct bufferevent *bev) {

    if(_readCb) {
        _readCb(shared_from_this(), _ctx); 
    }
}

void TcpConnection::writeCbImp(struct bufferevent *bev) {

    if(_writeCb) {
        _writeCb(shared_from_this(), _ctx); 
    }
}

void TcpConnection::connectCbImp(struct bufferevent *bev) {

    _state = ConnectionState::E_CONNECTED;
    if(_connectCb) {
        _connectCb(shared_from_this(), 0, _ctx); 
    }
}

void TcpConnection::errorCbImp(struct bufferevent *bev, int error) {

    _state = ConnectionState::E_DISCONNECTED;
    if(_errorCb) {
        _errorCb(shared_from_this(), error, _ctx); 
    }
}

struct bufferevent *TcpConnection::getBufferEvent() {
    return _bev;
}

TcpClient::TcpClient(VNetLoop *loop):
        VNetClient(loop) {
    
}

bool TcpClient::connect(const string & peerIp, uint16_t peerPort, const any &ctx) {

    auto tcpConnectionPtr = make_shared<TcpConnection>(_loop, 
            _loop->getVNetLoopTcp()->getEventBase(), peerIp, peerPort,
            shared_from_this());

    tcpConnectionPtr->setContext(ctx);
    tcpConnectionPtr->setConnectCb(_connectCb);
    tcpConnectionPtr->setReadCb(_readCb);
    tcpConnectionPtr->setWriteCb(_writeCb);
    tcpConnectionPtr->setErrorCb(_errorCb);
    tcpConnectionPtr->setConnectionState(ConnectionState::E_CONNECTING);

    if(_loop->addVNetConnection(tcpConnectionPtr->getIndex(), tcpConnectionPtr)) {
        return tcpConnectionPtr->connect();
    } else {
        return false; 
    }
}

TcpServer::TcpServer(VNetLoop *loop, const string & serverIp, uint16_t serverPort):
        VNetServer(loop, serverIp, serverPort), _listener(nullptr) {

}

TcpServer::~TcpServer() {

    if(_listener) {
        evconnlistener_free(_listener);
    }
}

bool TcpServer::start(void) {

    bool ret = false;
    struct sockaddr addr;
    int addrlen = 0;

    memset(&addr, 0, sizeof(addr));
    addrlen = sizeof(addr);

    if(evutil_parse_sockaddr_port((_serverIp + ':' + to_string(_serverPort)).c_str(),
                    &addr, &addrlen) < 0) {
        ret = false; 
    } else {

        _listener = evconnlistener_new_bind(_loop->getVNetLoopTcp()->getEventBase(), acceptCb, this,
                        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE,
                                -1, static_cast<struct sockaddr*>(&addr), addrlen);

        if(!_listener) {
            ret = false; 
        } else {
            ret = true; 
        }
    }

    return ret;
}

void TcpServer::acceptCb(struct evconnlistener *listener, evutil_socket_t fd,
                                struct sockaddr *a, int slen, void *p) {

    assert(p);
    TcpServer *server = static_cast<TcpServer *>(p);
    auto tcpConnectionPtr = make_shared<TcpConnection>(server->_loop,
            server->_loop->getVNetLoopTcp()->getEventBase(),fd,  
            server->_serverIp, server->_serverPort, server->shared_from_this());

    tcpConnectionPtr->accept();
    tcpConnectionPtr->setReadCb(server->_readCb);
    tcpConnectionPtr->setWriteCb(server->_writeCb);
    tcpConnectionPtr->setErrorCb(server->_errorCb);

    if(server->_loop->addVNetConnection(tcpConnectionPtr->getIndex(), tcpConnectionPtr)) {
        if(server->_acceptCb) {
            server->_acceptCb(tcpConnectionPtr, 0); 
        } else {
            server->_acceptCb(tcpConnectionPtr, 1); 
        }
    }
}

}

