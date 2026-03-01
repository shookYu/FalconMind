/**
 * @file SdkLoader.h
 * @brief SDK 动态加载器
 * 
 * 负责在运行时加载 SDK 共享库，实现编译时和运行时的完全解耦
 */

#pragma once

#include "nodeagent/sdk/SdkInterface.h"
#include <string>
#include <functional>

namespace nodeagent {

/**
 * @brief SDK 加载错误码
 */
enum class SdkLoadError {
    None,
    LibraryNotFound,        // 找不到 SDK 库文件
    SymbolNotFound,         // 找不到导出函数
    VersionMismatch,        // 接口版本不匹配
    InitFailed              // SDK 初始化失败
};

/**
 * @brief SDK 动态加载器
 * 
 * 使用 dlopen/dlsym (Linux) 或 LoadLibrary/GetProcAddress (Windows)
 * 在运行时加载 SDK 共享库
 */
class SdkLoader {
public:
    SdkLoader();
    ~SdkLoader();
    
    // 禁止拷贝
    SdkLoader(const SdkLoader&) = delete;
    SdkLoader& operator=(const SdkLoader&) = delete;
    
    /**
     * @brief 加载 SDK 共享库
     * 
     * @param libraryPath SDK 库文件路径（如 "./libfalconmind_sdk.so"）
     * @return true 加载成功
     * @return false 加载失败，可通过 getLastError() 获取错误
     */
    bool load(const char* libraryPath);
    
    /**
     * @brief 卸载 SDK
     */
    void unload();
    
    /**
     * @brief 检查是否已加载
     */
    bool isLoaded() const { return loaded_; }
    
    /**
     * @brief 获取 SDK 接口版本
     */
    int getInterfaceVersion();
    
    /**
     * @brief 获取 SDK 版本字符串
     */
    const char* getSdkVersion();
    
    /**
     * @brief 创建服务工厂
     * 
     * 通过工厂创建各种服务实例
     */
    sdk::ISdkServiceFactory* createServiceFactory();
    
    /**
     * @brief 初始化 SDK
     */
    bool initializeSdk(const char* pluginDir);
    
    /**
     * @brief 关闭 SDK
     */
    void shutdownSdk();
    
    /**
     * @brief 获取最后错误信息
     */
    SdkLoadError getLastError() const { return lastError_; }
    
    /**
     * @brief 获取最后错误描述
     */
    const char* getLastErrorString() const;
    
private:
    void* libraryHandle_;           // 动态库句柄
    bool loaded_;
    SdkLoadError lastError_;
    char errorBuffer_[256];
    
    // 函数指针类型
    using GetInterfaceVersionFunc = int (*)();
    using GetVersionFunc = const char* (*)();
    using CreateFactoryFunc = sdk::ISdkServiceFactory* (*)();
    using DestroyFactoryFunc = void (*)(sdk::ISdkServiceFactory*);
    using InitializeFunc = bool (*)(const char*);
    using ShutdownFunc = void (*)();
    
    // 函数指针
    GetInterfaceVersionFunc fnGetInterfaceVersion_;
    GetVersionFunc fnGetVersion_;
    CreateFactoryFunc fnCreateFactory_;
    DestroyFactoryFunc fnDestroyFactory_;
    InitializeFunc fnInitialize_;
    ShutdownFunc fnShutdown_;
    
    sdk::ISdkServiceFactory* factory_;
};

/**
 * @brief 全局 SDK 加载器访问函数
 */
SdkLoader& getSdkLoader();

/**
 * @brief 初始化 SDK 连接（在应用启动时调用）
 * 
 * @param sdkLibraryPath SDK 库路径，nullptr 则使用默认路径
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool initializeSdkConnection(const char* sdkLibraryPath = nullptr);

/**
 * @brief 关闭 SDK 连接（在应用退出时调用）
 */
void shutdownSdkConnection();

/**
 * @brief 获取 SDK 服务工厂
 */
sdk::ISdkServiceFactory* getSdkFactory();

} // namespace nodeagent
