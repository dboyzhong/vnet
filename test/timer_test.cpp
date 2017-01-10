/*************************************************************************
	> File Name: timer_test.cpp
	> Author: 
	> Mail: 
	> Created Time: Fri 30 Dec 2016 08:27:17 PM CST
 ************************************************************************/

#include<iostream>
#include "vnet_loop.h"
#include "timer.h"
#include <chrono>

using namespace std;
using std::experimental::any;
using std::experimental::any_cast;

using namespace vnet;

static int count = 10;

int main() {

    VNetLoop loop;
    Timer timer(&loop, 100, [](Timer *t, uint32_t interval, any ctx) {

                                  cout << "timer triggered interval:" << interval;
                                  cout << "ctx:" << any_cast<string>(ctx) << endl;
                                  if(count-- <= 0) {
                                      t->stop();    
                                  }
                              }, string("test"));

    Timer timer2(&loop, 100, [](Timer *t, uint32_t interval, any ctx) {

                                  cout << "timer2 triggered interval:" << interval;
                                  cout << "ctx:" << any_cast<string>(ctx) << endl;
                                  if(count-- <= 0) {
                                      t->stop();    
                                  }
                              }, string("test2"));
    timer.start();
    timer2.start();
    loop.loop();
    return 0;
}

