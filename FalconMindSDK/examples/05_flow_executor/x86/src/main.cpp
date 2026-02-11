
#include <iostream>
#include "falconmind/sdk/core/FlowExecutor.h"
using namespace falconmind::sdk::core;
int main(){
    std::cout<<"===05 FlowExecutor==="<<std::endl;
    auto exec=std::make_shared<FlowExecutor>();
    std::cout<<"FlowExecutor创建"<<std::endl;
    return 0;
}
