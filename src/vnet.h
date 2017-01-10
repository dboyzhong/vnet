/*************************************************************************
	> File Name: vnet.h
	> Author: 
	> Mail: 
	> Created Time: Wed 21 Dec 2016 07:36:56 PM CST
 ************************************************************************/

#ifndef _VNET_H
#define _VNET_H

#include <inttypes.h>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>
#include <experimental/any>

#include "vnet_def.h"
#include "vnet_loop.h"

using namespace std;
using std::experimental::any;
using std::experimental::any_cast;


namespace vnet {

class VNetClient;
class VNetServer;

enum class ConnectionState : unsigned {

    E_DISCONNECTED = 0,
    E_CONNECTING = 1,
    E_CONNECTED = 2,
    E_CLOSED = 3
};

/*
 * connection interface should be obtain in the callback of 
 * TcpServer/UdxServer::start & TcpClient/UdxClient::connect
 * */
class VNetConnection : public enable_shared_from_this<VNetConnection> {

public:

    enum class ConnDirection : unsigned {
        DIRECTION_UNDEF = 0,
        DIRECTION_IN = 1, 
        DIRECTION_OUT = 2,
    };

    VNetConnection(VNetLoop *loop, shared_ptr<VNetClient> vnetClient);
    VNetConnection(VNetLoop *loop, shared_ptr<VNetServer> vnetServer);
    virtual ~VNetConnection(){}

    typedef shared_ptr<VNetConnection> VNetConnectionPtr;
    typedef function<void(VNetConnectionPtr, int, any ctx)> ConnectCb;
    typedef function<void(VNetConnectionPtr, int, any ctx)> AcceptCb;
    typedef function<void(VNetConnectionPtr, any ctx)> ReadCb;
    typedef function<void(VNetConnectionPtr, any ctx)> WriteCb;
    typedef function<void(VNetConnectionPtr, int, any ctx)> ErrorCb;

    shared_ptr<VNetClient> getVNetClient() const {
        return _vnetClient;
    }

    shared_ptr<VNetServer> getVNetServer() const {
        return _vnetServer;
    }

    inline uint64_t getIndex() const {
        return _index; 
    }

    inline ConnDirection getDirection() const {
        return _direction;
    }

    inline void setContext(const any &ctx) {
        _ctx = ctx; 
    }

    inline any getContext() const {
        return _ctx; 
    }

    inline ConnectionState getConnectionState() const {
        return _state;
    }

    virtual bool connect() = 0;
    virtual void close() = 0;
    virtual void release() = 0;

    /*
     * async send, append msg to output buf
     * see getOutputBufferLen()
     * thread safe
     * */
    virtual bool send(const void* buf, uint32_t size) = 0;

    /*
     * async read, recv msg from input buf
     * see getInputBufferLen()
     * warning: must read all data from ReadCb callback, or remain data may not be got 
     * thread safe
     * */
    virtual uint32_t read(void *buf, uint32_t buf_len) = 0;
    virtual uint32_t getInputBufferLen() const = 0;
    virtual uint32_t getOutputBufferLen() const = 0;

    virtual void setConnectCb(ConnectCb cb) = 0;
    virtual void setReadCb(ReadCb cb) = 0;
    virtual void setWriteCb(WriteCb cb) = 0;
    virtual void setErrorCb(ErrorCb cb) = 0;

protected:

    inline void setConnectionState(ConnectionState state) {
        _state = state; 
    }

protected:
    static atomic<uint64_t> ConnectionCount;

    shared_ptr<VNetClient> _vnetClient;
    shared_ptr<VNetServer> _vnetServer;
    VNetLoop *             _loop;
    uint64_t               _index;
    ConnDirection          _direction;
    any                    _ctx;
    ConnectionState        _state;
};

typedef shared_ptr<VNetConnection> VNetConnectionPtr;
typedef function<void(VNetConnectionPtr, int, any ctx)> ConnectCb;
typedef function<void(VNetConnectionPtr, int)> AcceptCb;
typedef function<void(VNetConnectionPtr, any ctx)> ReadCb;
typedef function<void(VNetConnectionPtr, any ctx)> WriteCb;
typedef function<void(VNetConnectionPtr, int, any ctx)> ErrorCb;


/*
 *client interface used for connect to remote server
 * */
class VNetClient {

public:

    VNetClient(VNetLoop *loop); 
    virtual ~VNetClient();

    inline VNetLoop *getVNetLoop() {
        return _loop; 
    }

    virtual bool connect(const string &serverIp, uint16_t serverPort, const any &ctx = nullptr) = 0;
    void setConnectCb(ConnectCb cb);
    void setReadCb(ReadCb cb);
    void setWriteCb(WriteCb cb);
    void setErrorCb(ErrorCb cb);

protected:
    VNetLoop                  *_loop;
    string                     _serverIp;
    uint16_t                   _serverPort;

    ConnectCb                  _connectCb;
    ReadCb                     _readCb;
    WriteCb                    _writeCb;
    ErrorCb                    _errorCb;

};

/*
 *Server interface used for listen and accept
 * */
class VNetServer {

public:
    VNetServer(VNetLoop *loop, const string &serverIp, uint16_t serverPort);
    virtual ~VNetServer();

    inline VNetLoop *getVNetLoop() {
        return _loop; 
    }

    virtual bool start(void) = 0;

    void setAcceptCb(AcceptCb cb);
    void setReadCb(ReadCb cb);
    void setWriteCb(WriteCb cb);
    void setErrorCb(ErrorCb cb);

protected:
    VNetLoop                  *_loop;
    string                     _serverIp;
    uint16_t                   _serverPort;

    AcceptCb                   _acceptCb;
    ReadCb                     _readCb;
    WriteCb                    _writeCb;
    ErrorCb                    _errorCb;
};

}

#endif
