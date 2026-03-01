# SdkInterface Adapter - 集成到主 SDK CMake
# 此文件被 FalconMindSDK/CMakeLists.txt 包含

# 编译 SDK 接口适配器为共享库
# 这是实现 NodeAgent 与 SDK 解耦的关键

# 检查是否应该构建适配器
option(FALCONMINDSDK_BUILD_INTERFACE_ADAPTER "Build SDK interface adapter (for NodeAgent decoupling)" ON)

if(FALCONMINDSDK_BUILD_INTERFACE_ADAPTER)
    message(STATUS "FalconMindSDK: Building interface adapter for NodeAgent decoupling")
    
    # 收集适配器源文件
    set(ADAPTER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/adapter/SdkInterfaceImpl.cpp
    )
    
    # 创建共享库
    add_library(falconmind_sdk_interface SHARED ${ADAPTER_SOURCES})
    
    # 设置属性
    set_target_properties(falconmind_sdk_interface PROPERTIES
        OUTPUT_NAME "falconmind_sdk"
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    
    # 包含目录
    target_include_directories(falconmind_sdk_interface
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/NodeAgent/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # 链接 SDK 库
    # 使用 falconmind_sdk 目标（已在主 CMake 中定义）
    target_link_libraries(falconmind_sdk_interface
        PRIVATE
            falconmind_sdk
    )
    
    # 安装规则
    install(TARGETS falconmind_sdk_interface
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
    
    # 安装接口头文件
    install(FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/NodeAgent/include/nodeagent/sdk/SdkInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/NodeAgent/include/nodeagent/sdk/SdkLoader.h
        DESTINATION include/nodeagent/sdk
    )
    
    message(STATUS "FalconMindSDK: Interface adapter target: falconmind_sdk_interface")
    message(STATUS "  Output: libfalconmind_sdk.so")
    message(STATUS "  Purpose: Runtime loading by NodeAgent (decoupled architecture)")
endif()
