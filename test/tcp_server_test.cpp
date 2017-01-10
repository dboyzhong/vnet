/*************************************************************************
	> File Name: tcp_server_test.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 29 Dec 2016 07:05:02 PM CST
 ************************************************************************/

#include <cstring>
#include<iostream>
#include <experimental/any>

#include "tcp/vnet_tcp.h"
#include "vnet_loop.h"
using namespace std;
using std::experimental::any;
using std::experimental::any_cast;

using namespace vnet;

int main() {


    VNetLoop loop;
    shared_ptr<TcpServer> server = make_shared<TcpServer>(&loop, "127.0.0.1", 448);
    shared_ptr<TcpServer> server2 = make_shared<TcpServer>(&loop, "127.0.0.1", 449);

    server->setAcceptCb([](VNetConnectionPtr conn, int error){
            
                cout << "enter accept cb errno:" << error << endl;
            });

    server->setErrorCb([](VNetConnectionPtr conn, int error, any ctx){
            
                cout << "error occure : errno:" << error << endl;
                conn->close();
            });

    server->setWriteCb([](VNetConnectionPtr conn, any ctx){
            
            });

    server->setReadCb([](VNetConnectionPtr conn, any ctx){
            
            uint32_t len = conn->getInputBufferLen();
            char *buf1 = new char[len + 1];
            buf1[len + 1] = '\0';
            conn->read(buf1, len/2);
            cout << "read :" << buf1 << endl;
            char buf[] = "my name is yyy";

            conn->setWriteCb([](VNetConnectionPtr conn1, any ctx1){
                uint32_t len1 = conn1->getOutputBufferLen(); 
                cout << "enter close write cb len:" << len1 << endl;
                if(0 == len1) {
                    //conn1->close(); 
                }
            });

            if(!conn->send(buf, static_cast<uint32_t>(strlen(buf)))) {
                cout << "sending error" << endl; 
            }

            });

    server2->setAcceptCb([](VNetConnectionPtr conn, int error){
            
                cout << "server2 enter accept cb errno:" << error << endl;
            });

    server2->setErrorCb([](VNetConnectionPtr conn, int error, any ctx){
            
                cout << "server2 error occure : errno:" << error << endl;
                conn->close();
            });

    server2->setWriteCb([](VNetConnectionPtr conn, any ctx){
            
            });

    server2->setReadCb([](VNetConnectionPtr conn, any ctx){
            
            uint32_t len = conn->getInputBufferLen();
            char *buf1 = new char[len + 1];
            buf1[len + 1] = '\0';
            conn->read(buf1, len/2);
            cout << "server 2read :" << buf1 << endl;
            char buf[] = "my name is yyy";

            conn->setWriteCb([](VNetConnectionPtr conn1, any ctx1){
                uint32_t len1 = conn1->getOutputBufferLen(); 
                cout << "server 2enter close write cb len:" << len1 << endl;
                if(0 == len1) {
                    //conn1->close(); 
                }
            });

            if(!conn->send(buf, static_cast<uint32_t>(strlen(buf)))) {
                cout << "sending error" << endl; 
            }

            });

    if(server->start() && server2->start()) {
        cout << "server start success" << endl; 
        loop.loop();
    } else {
        cout << "server start failed" << endl; 
    }
    return 0;
}

