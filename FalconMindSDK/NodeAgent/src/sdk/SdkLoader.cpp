/**
 * @file SdkLoader.cpp
 * @brief SDK 动态加载器实现
 * 
 * 使用 dlopen/dlsym (Linux) 或 LoadLibrary/GetProcAddress (Windows)
 * 在运行时动态加载 SDK 共享库
 */

#include "nodeagent/sdk/SdkLoader.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace nodeagent {

// 默认 SDK 库路径（可配置）
#ifdef _WIN32
    const char* DEFAULT_SDK_LIBRARY = "falconmind_sdk.dll";
#elif defined(__APPLE__)
    const char* DEFAULT_SDK_LIBRARY = "libfalconmind_sdk.dylib";
#else
    const char* DEFAULT_SDK_LIBRARY = "libfalconmind_sdk.so";
#endif

SdkLoader::SdkLoader()
    : libraryHandle_(nullptr)
    , loaded_(false)
    , lastError_(SdkLoadError::None)
    , fnGetInterfaceVersion_(nullptr)
    , fnGetVersion_(nullptr)
    , fnCreateFactory_(nullptr)
    , fnDestroyFactory_(nullptr)
    , fnInitialize_(nullptr)
    , fnShutdown_(nullptr)
    , factory_(nullptr)
{
    errorBuffer_[0] = '\0';
}

SdkLoader::~SdkLoader()
{
    unload();
}

bool SdkLoader::load(const char* libraryPath)
{
    if (loaded_) {
        return true;
    }

    const char* path = libraryPath ? libraryPath : DEFAULT_SDK_LIBRARY;
    
    std::cout << "[SdkLoader] Loading SDK library: " << path << std::endl;

#ifdef _WIN32
    libraryHandle_ = LoadLibraryA(path);
    if (!libraryHandle_) {
        lastError_ = SdkLoadError::LibraryNotFound;
        snprintf(errorBuffer_, sizeof(errorBuffer_), 
                 "Failed to load library: %lu", GetLastError());
        std::cerr << "[SdkLoader] Error: " << errorBuffer_ << std::endl;
        return false;
    }

    // 加载函数指针
    fnGetInterfaceVersion_ = (GetInterfaceVersionFunc)GetProcAddress(
        (HMODULE)libraryHandle_, "FalconMindSdk_GetInterfaceVersion");
    fnGetVersion_ = (GetVersionFunc)GetProcAddress(
        (HMODULE)libraryHandle_, "FalconMindSdk_GetVersion");
    fnCreateFactory_ = (CreateFactoryFunc)GetProcAddress(
        (HMODULE)libraryHandle_, "FalconMindSdk_CreateServiceFactory");
    fnDestroyFactory_ = (DestroyFactoryFunc)GetProcAddress(
        (HMODULE)libraryHandle_, "FalconMindSdk_DestroyServiceFactory");
    fnInitialize_ = (InitializeFunc)GetProcAddress(
        (HMODULE)libraryHandle_, "FalconMindSdk_Initialize");
    fnShutdown_ = (ShutdownFunc)GetProcAddress(
        (HMODULE)libraryHandle_, "FalconMindSdk_Shutdown");
#else
    // Linux/macOS
    libraryHandle_ = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!libraryHandle_) {
        lastError_ = SdkLoadError::LibraryNotFound;
        const char* dlError = dlerror();
        snprintf(errorBuffer_, sizeof(errorBuffer_), 
                 "Failed to load library: %s", dlError ? dlError : "unknown error");
        std::cerr << "[SdkLoader] Error: " << errorBuffer_ << std::endl;
        return false;
    }

    // 加载函数指针
    fnGetInterfaceVersion_ = (GetInterfaceVersionFunc)dlsym(
        libraryHandle_, "FalconMindSdk_GetInterfaceVersion");
    fnGetVersion_ = (GetVersionFunc)dlsym(
        libraryHandle_, "FalconMindSdk_GetVersion");
    fnCreateFactory_ = (CreateFactoryFunc)dlsym(
        libraryHandle_, "FalconMindSdk_CreateServiceFactory");
    fnDestroyFactory_ = (DestroyFactoryFunc)dlsym(
        libraryHandle_, "FalconMindSdk_DestroyServiceFactory");
    fnInitialize_ = (InitializeFunc)dlsym(
        libraryHandle_, "FalconMindSdk_Initialize");
    fnShutdown_ = (ShutdownFunc)dlsym(
        libraryHandle_, "FalconMindSdk_Shutdown");
#endif

    // 检查所有函数是否加载成功
    if (!fnGetInterfaceVersion_ || !fnGetVersion_ || !fnCreateFactory_ ||
        !fnInitialize_ || !fnShutdown_) {
        lastError_ = SdkLoadError::SymbolNotFound;
        snprintf(errorBuffer_, sizeof(errorBuffer_), 
                 "Failed to load one or more required functions");
        std::cerr << "[SdkLoader] Error: " << errorBuffer_ << std::endl;
        
        unload();
        return false;
    }

    // 检查接口版本
    int interfaceVersion = fnGetInterfaceVersion_();
    if (interfaceVersion != FALCONMIND_SDK_INTERFACE_VERSION) {
        lastError_ = SdkLoadError::VersionMismatch;
        snprintf(errorBuffer_, sizeof(errorBuffer_), 
                 "Interface version mismatch: expected %d, got %d",
                 FALCONMIND_SDK_INTERFACE_VERSION, interfaceVersion);
        std::cerr << "[SdkLoader] Error: " << errorBuffer_ << std::endl;
        
        unload();
        return false;
    }

    loaded_ = true;
    lastError_ = SdkLoadError::None;
    
    std::cout << "[SdkLoader] SDK library loaded successfully" << std::endl;
    std::cout << "[SdkLoader] SDK Version: " << fnGetVersion_() << std::endl;
    std::cout << "[SdkLoader] Interface Version: " << interfaceVersion << std::endl;
    
    return true;
}

void SdkLoader::unload()
{
    if (!loaded_ || !libraryHandle_) {
        return;
    }

    std::cout << "[SdkLoader] Unloading SDK library" << std::endl;

    // 关闭 SDK
    if (fnShutdown_) {
        fnShutdown_();
    }

    // 销毁工厂
    if (factory_ && fnDestroyFactory_) {
        fnDestroyFactory_(factory_);
        factory_ = nullptr;
    }

#ifdef _WIN32
    FreeLibrary((HMODULE)libraryHandle_);
#else
    dlclose(libraryHandle_);
#endif

    libraryHandle_ = nullptr;
    loaded_ = false;
    
    fnGetInterfaceVersion_ = nullptr;
    fnGetVersion_ = nullptr;
    fnCreateFactory_ = nullptr;
    fnDestroyFactory_ = nullptr;
    fnInitialize_ = nullptr;
    fnShutdown_ = nullptr;
}

int SdkLoader::getInterfaceVersion()
{
    if (!loaded_ || !fnGetInterfaceVersion_) {
        return -1;
    }
    return fnGetInterfaceVersion_();
}

const char* SdkLoader::getSdkVersion()
{
    if (!loaded_ || !fnGetVersion_) {
        return "unknown";
    }
    return fnGetVersion_();
}

sdk::ISdkServiceFactory* SdkLoader::createServiceFactory()
{
    if (!loaded_ || !fnCreateFactory_) {
        return nullptr;
    }
    
    if (!factory_) {
        factory_ = fnCreateFactory_();
    }
    
    return factory_;
}

bool SdkLoader::initializeSdk(const char* pluginDir)
{
    if (!loaded_ || !fnInitialize_) {
        return false;
    }
    
    return fnInitialize_(pluginDir);
}

void SdkLoader::shutdownSdk()
{
    if (!loaded_ || !fnShutdown_) {
        return;
    }
    
    fnShutdown_();
}

const char* SdkLoader::getLastErrorString() const
{
    if (errorBuffer_[0] != '\0') {
        return errorBuffer_;
    }
    
    switch (lastError_) {
        case SdkLoadError::None:
            return "No error";
        case SdkLoadError::LibraryNotFound:
            return "Library not found";
        case SdkLoadError::SymbolNotFound:
            return "Required symbol not found";
        case SdkLoadError::VersionMismatch:
            return "Interface version mismatch";
        case SdkLoadError::InitFailed:
            return "SDK initialization failed";
        default:
            return "Unknown error";
    }
}

//==============================================================================
// 全局单例实现
//==============================================================================

static SdkLoader& getSdkLoaderInstance()
{
    static SdkLoader instance;
    return instance;
}

SdkLoader& getSdkLoader()
{
    return getSdkLoaderInstance();
}

bool initializeSdkConnection(const char* sdkLibraryPath)
{
    SdkLoader& loader = getSdkLoader();
    
    if (loader.isLoaded()) {
        return true;
    }
    
    // 加载 SDK 库
    if (!loader.load(sdkLibraryPath)) {
        std::cerr << "[NodeAgent] Failed to load SDK: " 
                  << loader.getLastErrorString() << std::endl;
        return false;
    }
    
    // 初始化 SDK
    if (!loader.initializeSdk("./plugins")) {
        std::cerr << "[NodeAgent] Failed to initialize SDK" << std::endl;
        loader.unload();
        return false;
    }
    
    std::cout << "[NodeAgent] SDK connection initialized successfully" << std::endl;
    return true;
}

void shutdownSdkConnection()
{
    SdkLoader& loader = getSdkLoader();
    
    if (loader.isLoaded()) {
        loader.shutdownSdk();
        loader.unload();
        std::cout << "[NodeAgent] SDK connection shutdown" << std::endl;
    }
}

sdk::ISdkServiceFactory* getSdkFactory()
{
    SdkLoader& loader = getSdkLoader();
    
    if (!loader.isLoaded()) {
        return nullptr;
    }
    
    return loader.createServiceFactory();
}

} // namespace nodeagent
