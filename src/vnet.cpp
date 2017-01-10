/*************************************************************************
	> File Name: vnet.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 22 Dec 2016 08:35:59 PM CST
 ************************************************************************/

#include<iostream>
#include "vnet.h"

using namespace std;

namespace vnet {

atomic<uint64_t> VNetConnection::ConnectionCount(0);

VNetConnection::VNetConnection(VNetLoop *loop, shared_ptr<VNetClient> vnetClient):
    _vnetClient(vnetClient), _loop(loop),_index(0), _direction(ConnDirection::DIRECTION_OUT),
    _state(ConnectionState::E_DISCONNECTED) {

        _index = ConnectionCount++;
}

VNetConnection::VNetConnection(VNetLoop *loop, shared_ptr<VNetServer> vnetServer):
    _vnetServer(vnetServer), _loop(loop), _index(0), _direction(ConnDirection::DIRECTION_IN),
    _state(ConnectionState::E_DISCONNECTED) {

        _index = ConnectionCount++;
}

VNetClient::VNetClient(VNetLoop *loop):_loop(loop) {

}

VNetClient::~VNetClient() {

}

void VNetClient::setConnectCb(ConnectCb cb) {
    _connectCb = cb;
}

void VNetClient::setReadCb(ReadCb cb) {
    _readCb = cb;
}

void VNetClient::setWriteCb(WriteCb cb) {
    _writeCb = cb;
}

void VNetClient::setErrorCb(ErrorCb cb) {
    _errorCb = cb;
}

VNetServer::VNetServer(VNetLoop *loop, const string &serverIp, uint16_t serverPort):
    _loop(loop), _serverIp(serverIp), _serverPort(serverPort) {

}

VNetServer::~VNetServer() {

}

void VNetServer::setAcceptCb(AcceptCb cb) {
    _acceptCb = cb;
}

void VNetServer::setReadCb(ReadCb cb) {
    _readCb = cb;
}

void VNetServer::setWriteCb(WriteCb cb) {
    _writeCb = cb;
}

void VNetServer::setErrorCb(ErrorCb cb) {
    _errorCb = cb;
}

}
