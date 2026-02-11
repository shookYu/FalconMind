
#include <iostream>
#include "falconmind/sdk/core/Bus.h"
using namespace falconmind::sdk::core;
int main(){
    std::cout<<"===04 Bus消息==="<<std::endl;
    auto bus=std::make_shared<Bus>();
    int id=bus->subscribe([](const BusMessage& m){std::cout<<"收到:"<<m.category<<std::endl;});
    std::cout<<"订阅ID:"<<id<<std::endl;
    bus->post(BusMessage{"msg","hello"});
    bus->unsubscribe(id);
    std::cout<<"完成"<<std::endl;
    return 0;
}
