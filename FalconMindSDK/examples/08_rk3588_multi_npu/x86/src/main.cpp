
#include <iostream>
#include "falconmind/sdk/perception/RknnDetectorBackend.h"
using namespace falconmind::sdk;
int main(){
    std::cout<<"===08 RK3588多NPU==="<<std::endl;
    perception::RknnDetectorBackend backend;
    backend.setCoreMask(0x7);
    std::cout<<"3核心模式启用"<<std::endl;
    return 0;
}
