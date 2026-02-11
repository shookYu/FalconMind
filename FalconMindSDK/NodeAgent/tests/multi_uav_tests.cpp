// NodeAgent - MultiUavManager unit tests
#include <gtest/gtest.h>
#include "nodeagent/MultiUavManager.h"
#include "nodeagent/NodeAgent.h"

using namespace nodeagent;

void registerMultiUavTests() {
    // Tests are registered via TEST macros
}

TEST(MultiUavManagerTest, BasicInitialization) {
    MultiUavManager manager;
    EXPECT_TRUE(true);
}

TEST(MultiUavManagerTest, AddUav) {
    MultiUavManager manager;
    
    MultiUavManager::UavConfig config;
    config.uavId = "uav1";
    config.centerAddress = "127.0.0.1";
    config.centerPort = 8888;
    
    bool result = manager.addUav(config);
    EXPECT_TRUE(result);
    
    auto uavList = manager.getUavList();
    EXPECT_EQ(uavList.size(), 1);
    EXPECT_EQ(uavList[0], "uav1");
}

TEST(MultiUavManagerTest, AddDuplicateUav) {
    MultiUavManager manager;
    
    MultiUavManager::UavConfig config;
    config.uavId = "uav1";
    config.centerAddress = "127.0.0.1";
    config.centerPort = 8888;
    
    manager.addUav(config);
    
    // Adding duplicate should fail
    bool result = manager.addUav(config);
    EXPECT_FALSE(result);
    
    auto uavList = manager.getUavList();
    EXPECT_EQ(uavList.size(), 1);  // Still only one
}

TEST(MultiUavManagerTest, RemoveUav) {
    MultiUavManager manager;
    
    MultiUavManager::UavConfig config;
    config.uavId = "uav1";
    config.centerAddress = "127.0.0.1";
    config.centerPort = 8888;
    
    manager.addUav(config);
    EXPECT_EQ(manager.getUavList().size(), 1);
    
    bool result = manager.removeUav("uav1");
    EXPECT_TRUE(result);
    EXPECT_EQ(manager.getUavList().size(), 0);
}

TEST(MultiUavManagerTest, RemoveNonExistentUav) {
    MultiUavManager manager;
    
    bool result = manager.removeUav("nonexistent");
    EXPECT_FALSE(result);
}

TEST(MultiUavManagerTest, AddMultipleUavs) {
    MultiUavManager manager;
    
    MultiUavManager::UavConfig config1;
    config1.uavId = "uav1";
    config1.centerAddress = "127.0.0.1";
    config1.centerPort = 8888;
    
    MultiUavManager::UavConfig config2;
    config2.uavId = "uav2";
    config2.centerAddress = "127.0.0.1";
    config2.centerPort = 8889;
    
    manager.addUav(config1);
    manager.addUav(config2);
    
    auto uavList = manager.getUavList();
    EXPECT_EQ(uavList.size(), 2);
}

TEST(MultiUavManagerTest, IsUavRunning) {
    MultiUavManager manager;
    
    MultiUavManager::UavConfig config;
    config.uavId = "uav1";
    config.centerAddress = "127.0.0.1";
    config.centerPort = 8888;
    
    manager.addUav(config);
    
    // Before starting, should not be running
    EXPECT_FALSE(manager.isUavRunning("uav1"));
    
    // Note: startUav() requires actual connection, so we just test the interface
    // In real tests, you might want to mock the connection
}

TEST(MultiUavManagerTest, GetUavListWhenEmpty) {
    MultiUavManager manager;
    
    auto uavList = manager.getUavList();
    EXPECT_TRUE(uavList.empty());
}
