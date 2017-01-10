/*************************************************************************
	> File Name: vnet_udx.cpp
	> Author: 
	> Mail: 
	> Created Time: Wed 04 Jan 2017 12:41:09 PM CST
 ************************************************************************/

#include<iostream>
#include "vnet_udx.h"
using namespace std;

namespace vnet {

UdxConnection::UdxConnection(VNetLoop *loop, IFastUdx * udx, IUdxTcp * udxTcp, const string &peerIp, uint16_t peerPort, 
              shared_ptr<VNetClient> vnetClient): VNetConnection(loop, vnetClient), _udx(udx), _udxTcp(udxTcp),
              _writeCbTrigged(false), _peerIp(peerIp), _peerPort(peerPort) {

    assert(loop);
    assert(udx);
    _readBuffer = evbuffer_new();
    _writeBuffer = evbuffer_new();
    _direction = ConnDirection::DIRECTION_OUT;
}

UdxConnection::UdxConnection(VNetLoop *loop, IFastUdx * udx, IUdxTcp * udxTcp, const string &peerIp, uint16_t peerPort, 
              shared_ptr<VNetServer> vnetServer): VNetConnection(loop, vnetServer), _udx(udx), _udxTcp(udxTcp),
              _writeCbTrigged(false), _peerIp(peerIp), _peerPort(peerPort) {

    assert(loop);
    assert(udx);
    _readBuffer = evbuffer_new();
    _writeBuffer = evbuffer_new();
    _direction = ConnDirection::DIRECTION_IN;
}

UdxConnection::~UdxConnection(){

    if(_readBuffer) {
        evbuffer_free(_readBuffer); 
        _readBuffer = nullptr;
    }
    if(_writeBuffer) {
        evbuffer_free(_writeBuffer); 
        _writeBuffer = nullptr;
    }
}

bool UdxConnection::connect() {
    
    IUdxTcp* conn = _udx->Connect(const_cast<char *>(_peerIp.c_str()), _peerPort);
    if(conn) {
        _state = ConnectionState::E_CONNECTING;
        _udxTcp = conn;
        return true;
    } else {
        return false;
    }
}

void UdxConnection::close() {

    lock_guard<mutex> lck(_writeBufLock);
    _state = ConnectionState::E_CLOSED;
    if(_udxTcp) {
        _udxTcp->Close(); 
        _udxTcp->ReleaseLife(); 
    }
}

void UdxConnection::release() {

};

bool UdxConnection::send(const void*buf, uint32_t size) {

    bool ret = false;
    _writeBufLock.lock();

    if(_udxTcp && (_state == ConnectionState::E_CONNECTED)) {

        if(evbuffer_add(_writeBuffer, buf, size)) {
            ret = false;
        } else {
            ret = true; 
        }
    } else {
        ret = false; 
    }
    _writeBufLock.unlock();
    return ret;
}

uint32_t UdxConnection::read(void *buf, uint32_t buf_len) {

    return evbuffer_remove(_readBuffer, buf, buf_len);
}

uint32_t UdxConnection::getInputBufferLen() const {

    uint32_t data_size = static_cast<uint32_t>(evbuffer_get_length(_readBuffer));
    return data_size;
}

uint32_t UdxConnection::getOutputBufferLen() const {

    uint32_t data_size = static_cast<uint32_t>(evbuffer_get_length(_writeBuffer));
    return data_size;
}

void UdxConnection::setConnectCb(ConnectCb cb) {
    _connectCb = cb;
}

void UdxConnection::setReadCb(ReadCb cb) {
    _readCb = cb;
}

void UdxConnection::setWriteCb(WriteCb cb) {
    _writeCb = cb;
}

void UdxConnection::setErrorCb(ErrorCb cb) {
    _errorCb = cb;
}

void UdxConnection::handleUdxStreamRead(BYTE* pData, int len){

    evbuffer_add(_readBuffer, pData, len);

    if(_readCb) {
        _readCb(shared_from_this(), _ctx);
    }
}

void UdxConnection::handleUdxStreamWrite(){

    _writeBufLock.lock();
    size_t data_size = evbuffer_get_length(_writeBuffer);
    char * send_buf = new char[WRITE_CHUNK_SIZE];

    if(data_size) {
        _writeCbTrigged = false; 
    } else {
        _writeCbTrigged = true; 
    }

    while(data_size) {
        size_t write_len = (WRITE_CHUNK_SIZE > data_size ? data_size : WRITE_CHUNK_SIZE);             
        size_t len = evbuffer_copyout(_writeBuffer, send_buf, write_len);
        if(_udxTcp->SendBuff(reinterpret_cast<BYTE*>(send_buf), static_cast<int>(len))) {
            evbuffer_drain(_writeBuffer, len); 
            data_size = evbuffer_get_length(_writeBuffer);
        } else {
            break; 
        }
    }
    _writeBufLock.unlock();
    delete[] send_buf;
    if(0 == data_size) {
         
        if(!_writeCbTrigged && _writeCb) {
            _writeCb(shared_from_this(), _ctx);
        }
    }
}

void UdxConnection::handleUdxStreamError(){

    _state = ConnectionState::E_DISCONNECTED;

    if(_errorCb) {
        _errorCb(shared_from_this(), 0, _ctx);        
    }
}

void UdxConnection::handleUdxStreamRelease(){

    lock_guard<mutex> lck(_writeBufLock);

    _udxTcp->SetUserData(0);
    _loop->removeVNetConnection(getIndex());
    _udxTcp = nullptr;
}

UdxClient::UdxClient(VNetLoop *loop):
           VNetClient(loop), _udx(nullptr) {
    _udx = CreateFastUdx();
    if(_udx && _udx->Create()) {
        _udx->SetSink(this);
    }
}

UdxClient::~UdxClient() {
    /*if(_udx) {
        delete _udx; 
        _udx = nullptr;
    }*/
}

bool UdxClient::connect(const string & peerIp, uint16_t peerPort, const any &ctx) {

    auto udxConnectionPtr = make_shared<UdxConnection>(_loop, 
            _udx, nullptr, peerIp, peerPort,
            shared_from_this());
    IUdxTcp* conn = _udx->Connect(const_cast<char *>(peerIp.c_str()), peerPort, false,
                                  reinterpret_cast<INT64>(udxConnectionPtr.get()),
                                  0, 50, nullptr, nullptr, nullptr, FALSE);

    if(!conn) {
        return false; 
    }

    udxConnectionPtr->setContext(ctx);
    udxConnectionPtr->setUdxTcp(conn);

    udxConnectionPtr->setConnectCb(_connectCb);
    udxConnectionPtr->setReadCb(_readCb);
    udxConnectionPtr->setWriteCb(_writeCb);
    udxConnectionPtr->setErrorCb(_errorCb);

    if(_loop->addVNetConnection(udxConnectionPtr->getIndex(), udxConnectionPtr)) {
        return true;
    } else {
        return false; 
    }
}

void UdxClient::OnStreamConnect(IUdxTcp * pTcp,int erro){

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        if(!erro) {
            conn->setConnectionState(ConnectionState::E_CONNECTED);
        } else {
            conn->setConnectionState(ConnectionState::E_DISCONNECTED);
        }

        if(!erro && _connectCb) {
            _connectCb(conn->shared_from_this(), 0, conn->getContext()); 
        } else if (erro && _errorCb) {
            _errorCb(conn->shared_from_this(), erro, conn->getContext()); 
        }
    }
}

void UdxClient::OnStreamBroken(IUdxTcp * pTcp){

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamError();
    }
}

void UdxClient::OnStreamRead(IUdxTcp * pTcp,BYTE* pData,int len){

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamRead(pData, len);
    }
}

void UdxClient::OnStreamWrite(IUdxTcp * pTcp,BYTE* pData,int len){

}

void UdxClient::OnStreamNeedMoreData(IUdxTcp *pTcp,int needdata){

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamWrite();
    }
}

void UdxClient::OnStreamChancetoFillBuff(IUdxTcp *pTcp){

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamWrite();
    }
}

void UdxClient::OnStreamFinalRelease(IUdxTcp * pTcp){

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamRelease();
    }
}

UdxServer::UdxServer(VNetLoop *loop, const string &serverIp, uint16_t serverPort):
           VNetServer(loop, serverIp, serverPort), _udx(nullptr) {

    _udx = CreateFastUdx();
}

UdxServer::~UdxServer(){
    /*if(_udx) {
        delete _udx; 
        _udx = nullptr;
    }*/
}

bool UdxServer::start(void) {

    if(_udx && _udx->Create(const_cast<char *>(_serverIp.c_str()), _serverPort)) {
        _udx->SetSink(this);
        return true;
    } else {
        return false;
    }
}

void UdxServer::OnStreamConnect(IUdxTcp * pTcp,int erro){

    assert(pTcp);
    pTcp->AddLife();
    auto udxConnectionPtr = make_shared<UdxConnection>(_loop, _udx, pTcp, _serverIp, _serverPort,
                                                       shared_from_this());
    pTcp->SetUserData(reinterpret_cast<INT64>(udxConnectionPtr.get()));

    udxConnectionPtr->setReadCb(_readCb);
    udxConnectionPtr->setWriteCb(_writeCb);
    udxConnectionPtr->setErrorCb(_errorCb);

    if(!erro) {
        udxConnectionPtr->setConnectionState(ConnectionState::E_CONNECTED);
    } else {
        udxConnectionPtr->setConnectionState(ConnectionState::E_DISCONNECTED);
    }

    if(_loop->addVNetConnection(udxConnectionPtr->getIndex(), udxConnectionPtr)) {
        if(_acceptCb) {
            _acceptCb(udxConnectionPtr, erro); 
        }
    }
}

void UdxServer::OnStreamBroken(IUdxTcp * pTcp){

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamError();
    }
}

void UdxServer::OnStreamRead(IUdxTcp * pTcp,BYTE* pData,int len) {

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamRead(pData, len);
    }
}

void UdxServer::OnStreamWrite(IUdxTcp * pTcp,BYTE* pData,int len){

}

void UdxServer::OnStreamNeedMoreData(IUdxTcp *pTcp,int needdata){
    HandleStreamWrite(pTcp);
}

void UdxServer::OnStreamChancetoFillBuff(IUdxTcp *pTcp){
    HandleStreamWrite(pTcp);
}

void UdxServer::OnStreamFinalRelease(IUdxTcp * pTcp){

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamRelease();
    }
}

void UdxServer::HandleStreamWrite(IUdxTcp *pTcp) {

    assert(pTcp);
    UdxConnection * conn = reinterpret_cast<UdxConnection*>(pTcp->GetUserData());
    if(conn) {
        conn->handleUdxStreamWrite();
    }
}

}

