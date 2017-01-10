/*************************************************************************
	> File Name: tcp_client_test.cpp
	> Author: 
	> Mail: 
	> Created Time: Fri 23 Dec 2016 02:14:05 PM CST
 ************************************************************************/

#include <stdio.h>
#include <cstring>
#include<iostream>
#include <experimental/any>

#include "tcp/vnet_tcp.h"
#include "udx/vnet_udx.h"
#include "vnet_loop.h"
using namespace std;
using std::experimental::any;
using std::experimental::any_cast;

using namespace vnet;

int main() {


    VNetLoop loop;
    shared_ptr<TcpClient> client = make_shared<TcpClient>(&loop);
    shared_ptr<UdxClient> client2 = make_shared<UdxClient>(&loop);
    client->setConnectCb([](VNetConnectionPtr connPtr, int error, any ctx) {
                printf("connected error:%d, context:%s\n", error, any_cast<string>(ctx).c_str());

                if(error) {
                    connPtr->close(); 
                } else {
                    char buf[] = "my name is xxx";
                    if(!connPtr->send(buf, static_cast<uint32_t>(strlen(buf)))) {
                        cout << "sending error" << endl; 
                        connPtr->close();
                    }
                }
            });
    client->setReadCb([](VNetConnectionPtr connPtr, any ctx) {
                
                uint32_t len = connPtr->getInputBufferLen();
                char *buf = new char[len + 1];
                buf[len + 1] = '\0';
                connPtr->read(buf, len);
                printf("receive msg:%s, context:%s\n", buf, any_cast<string>(ctx).c_str());
            });
    client->setWriteCb([](VNetConnectionPtr connPtr, any ctx) {
                printf("write finished, context:%s\n", any_cast<string>(ctx).c_str());
            });
    client->setErrorCb([](VNetConnectionPtr connPtr, int error, any ctx) {
                printf("error occure: error:%d, context:%s\n", error, any_cast<string>(ctx).c_str());

                uint32_t len = connPtr->getInputBufferLen();
                char *buf = new char[len + 1];
                buf[len + 1] = '\0';
                connPtr->read(buf, len);
                cout << "drain msg :" << buf << "len:" <<len << endl;

                connPtr->close();
            });

    client2->setConnectCb([](VNetConnectionPtr connPtr, int error, any ctx) {
                printf("client2 connected error:%d, context:%s\n", error, any_cast<string>(ctx).c_str());

                if(error) {
                    connPtr->close(); 
                } else {
                    char buf[] = "my name is xxx";
                    if(!connPtr->send(buf, static_cast<uint32_t>(strlen(buf)))) {
                        cout << "sending error" << endl; 
                        connPtr->close();
                    }
                }
            });
    client2->setReadCb([](VNetConnectionPtr connPtr, any ctx) {
                
                uint32_t len = connPtr->getInputBufferLen();
                char *buf = new char[len + 1];
                buf[len + 1] = '\0';
                connPtr->read(buf, len);
                printf("client2 receive msg:%s, context:%s\n", buf, any_cast<string>(ctx).c_str());
            });
    client2->setWriteCb([](VNetConnectionPtr connPtr, any ctx) {
                printf("client2 write finished, context:%s\n", any_cast<string>(ctx).c_str());
            });
    client2->setErrorCb([](VNetConnectionPtr connPtr, int error, any ctx) {
                printf("client2 error occure: error:%d, context:%s\n", error, any_cast<string>(ctx).c_str());

                uint32_t len = connPtr->getInputBufferLen();
                char *buf = new char[len + 1];
                buf[len + 1] = '\0';
                connPtr->read(buf, len);
                cout << "drain msg :" << buf << "len:" <<len << endl;

                connPtr->close();
            });
    for(int i = 0; i < 4; i++) {
        bool ret = client->connect("127.0.0.1", 448, string("my context"));
        bool ret2 = client2->connect("127.0.0.1", 449, string("my context2"));
        if(ret && ret2) {
            cout << "true" << endl; 
        } else {
            cout << "false" << endl; 
        }
    }

    loop.loop();

    return 0;
}

