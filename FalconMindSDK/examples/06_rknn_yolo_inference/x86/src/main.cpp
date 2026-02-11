
#include <iostream>
#include "falconmind/sdk/perception/RknnDetectorBackend.h"
using namespace falconmind::sdk;
int main(){
    std::cout<<"===06 RKNN YOLO==="<<std::endl;
    perception::RknnDetectorBackend backend;
    std::cout<<"RKNN后端创建"<<std::endl;
    return 0;
}
