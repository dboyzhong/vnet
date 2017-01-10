/*************************************************************************
	> File Name: vnet_udx.h
	> Author: 
	> Mail: 
	> Created Time: Thu 22 Dec 2016 06:53:54 PM CST
 ************************************************************************/

#ifndef _VNET_UDX_H
#define _VNET_UDX_H

#include <inttypes.h>
#include <string>
#include <mutex>
#include "udx/udxos.h"
#include "udx/FastUdx.h"

#include "vnet.h"
#include "vnet_udx_loop.h"
#include <event2/buffer.h>

using namespace std;

namespace vnet {

static const uint32_t WRITE_CHUNK_SIZE = 1024;

class UdxConnection : public VNetConnection {

public:

    friend class UdxServer;
    friend class UdxClient;

    UdxConnection(VNetLoop *loop, IFastUdx * udx, IUdxTcp *udxTcp, const string &peerIp, uint16_t peerPort, 
                  shared_ptr<VNetClient> vnetClient);
    UdxConnection(VNetLoop *loop, IFastUdx * udx, IUdxTcp *udxTcp, const string &peerIp, uint16_t peerPort, 
                  shared_ptr<VNetServer> vnetServer);
    ~UdxConnection();

    bool connect() override;
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

    void handleUdxStreamRead(BYTE* pData, int len);
    void handleUdxStreamWrite();
    void handleUdxStreamError();
    void handleUdxStreamRelease();

    inline ConnectCb getConnectCb() {
        return _connectCb;
    }

    inline ReadCb    getReadCb() {
        return _readCb;
    }

    inline WriteCb   getWriteCb() {
        return _writeCb;
    }

    inline ErrorCb   getErrorCb() {
        return _errorCb;
    }

    inline struct evbuffer *getReadBuffer() {
        return _readBuffer; 
    }

    inline struct evbuffer *getWriteBuffer() {
        return _writeBuffer; 
    }

    inline void setUdxTcp(IUdxTcp *udxTcp) {
        _udxTcp = udxTcp; 
    }


protected:
    IFastUdx *           _udx;
    IUdxTcp  *           _udxTcp;
    struct evbuffer *    _readBuffer;
    struct evbuffer *    _writeBuffer;
    mutex                _readBufLock;
    mutex                _writeBufLock;
    bool                 _writeCbTrigged;

private:
    string               _peerIp;
    uint16_t             _peerPort;

    ConnectCb            _connectCb;
    ReadCb               _readCb;
    WriteCb              _writeCb;
    ErrorCb              _errorCb;
};

class UdxClient : public VNetClient, public IUdxTcpSink, public enable_shared_from_this<UdxClient> {

public:
    UdxClient(VNetLoop *loop);
    ~UdxClient();

    bool connect(const string & peerIp, uint16_t peerPort, const any &ctx = nullptr) override;

protected:

    void OnStreamConnect(IUdxTcp * pTcp,int erro);
    void OnStreamBroken(IUdxTcp * pTcp);
    void OnStreamRead(IUdxTcp * pTcp,BYTE* pData,int len);
    void OnStreamWrite(IUdxTcp * pTcp,BYTE* pData,int len);
    void OnStreamNeedMoreData(IUdxTcp *pTcp,int needdata);
    void OnStreamChancetoFillBuff(IUdxTcp *pTcp);
    void OnStreamFinalRelease(IUdxTcp * pTcp);

protected:
    IFastUdx * _udx;
};

class UdxServer : public VNetServer, public IUdxTcpSink, public enable_shared_from_this<UdxServer> {

public:
    UdxServer(VNetLoop *loop, const string &serverIP, uint16_t serverPort);
    virtual ~UdxServer();
    
    bool start(void) override;

protected:

    void OnStreamConnect(IUdxTcp * pTcp,int erro);
    void OnStreamBroken(IUdxTcp * pTcp);
    void OnStreamRead(IUdxTcp * pTcp,BYTE* pData,int len);
    void OnStreamWrite(IUdxTcp * pTcp,BYTE* pData,int len);
    void OnStreamNeedMoreData(IUdxTcp *pTcp,int needdata);
    void OnStreamChancetoFillBuff(IUdxTcp *pTcp);
    void OnStreamFinalRelease(IUdxTcp * pTcp);
    void HandleStreamWrite(IUdxTcp *pTcp);

protected:
    IFastUdx * _udx;
};

}

#endif
