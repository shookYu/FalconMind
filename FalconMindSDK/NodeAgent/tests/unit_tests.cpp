// NodeAgent - Main unit test entry point
#include <gtest/gtest.h>

// 声明各个测试模块
extern void registerCommandHandlerTests();
extern void registerMissionHandlerTests();
extern void registerMessageAckTests();
extern void registerMultiUavTests();
extern void registerLoggerTests();
extern void registerErrorStatisticsTests();
extern void registerReconnectManagerTests();

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // 注册所有测试模块
    registerCommandHandlerTests();
    registerMissionHandlerTests();
    registerMessageAckTests();
    registerMultiUavTests();
    registerLoggerTests();
    registerErrorStatisticsTests();
    registerReconnectManagerTests();
    
    return RUN_ALL_TESTS();
}
