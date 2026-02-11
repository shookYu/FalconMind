
#include <iostream>
#include "falconmind/sdk/perception/RknnDetectorBackend.h"
using namespace falconmind::sdk;
int main(){
    std::cout<<"===07 RKNN后端==="<<std::endl;
    perception::RknnDetectorBackend backend;
    backend.setCoreMask(0x7);
    std::cout<<"核心掩码已设置"<<std::endl;
    return 0;
}
