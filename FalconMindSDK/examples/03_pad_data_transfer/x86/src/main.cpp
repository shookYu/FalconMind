
#include <iostream>
#include "falconmind/sdk/core/Pad.h"
using namespace falconmind::sdk::core;
int main(){
    std::cout<<"===03 Pad数据传输==="<<std::endl;
    auto p=std::make_shared<Pad>("out",PadType::Source);
    std::cout<<"Pad名称:"<<p->name()<<std::endl;
    std::cout<<"类型:Source="<<(p->type()==PadType::Source)<<std::endl;
    return 0;
}
