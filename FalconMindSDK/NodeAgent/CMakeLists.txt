cmake_minimum_required(VERSION 3.16)
project(NodeAgent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# 检测编译模式: 独立编译 vs 作为SDK子目录集成
# =============================================================================
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # 独立编译模式
    set(NODEAGENT_STANDALONE TRUE)
    message(STATUS "NodeAgent: 独立编译模式")
else()
    # 子目录集成模式 (作为FalconMindSDK的一部分)
    set(NODEAGENT_STANDALONE FALSE)
    message(STATUS "NodeAgent: SDK集成模式")
endif()

# =============================================================================
# 依赖配置 - 子目录模式使用SDK的依赖
# =============================================================================
if(NODEAGENT_STANDALONE)
    # 集成 Google Test（单元测试框架）
    option(NODEAGENT_BUILD_TESTS "Build NodeAgent unit tests" ON)

    if(NODEAGENT_BUILD_TESTS)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
            GIT_SHALLOW TRUE
        )
        set(FETCHCONTENT_QUIET OFF)
        FetchContent_MakeAvailable(googletest)
        message(STATUS "Google Test enabled for unit tests")
    endif()

    # 集成 nlohmann/json
    find_package(nlohmann_json QUIET)
    if(NOT nlohmann_json_FOUND)
        include(FetchContent)
        set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Skip updating content from remote")
        set(FETCHCONTENT_QUIET OFF)
        FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.2
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(json)
        message(STATUS "Using nlohmann/json from FetchContent")
    else()
        message(STATUS "Using system-installed nlohmann/json")
    endif()

    # 集成 paho-mqtt-cpp
    option(NODEAGENT_USE_MQTT "Enable MQTT support" ON)
    set(PAHOMQTTCPP_FOUND FALSE)

    if(NODEAGENT_USE_MQTT)
        find_package(PahoMqttCpp QUIET)
        if(PahoMqttCpp_FOUND AND TARGET paho-mqtt-cpp::paho-mqtt-cpp)
            message(STATUS "Using system-installed paho-mqtt-cpp")
            set(PAHOMQTTCPP_FOUND TRUE)
        else()
            include(FetchContent)
            set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Skip updating content from remote")
            set(FETCHCONTENT_QUIET OFF)
            FetchContent_Declare(
                paho-mqtt-c
                GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.c.git
                GIT_TAG v1.3.12
                GIT_SHALLOW TRUE
            )
            FetchContent_Declare(
                paho-mqtt-cpp
                GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
                GIT_TAG v1.2.0
                GIT_SHALLOW TRUE
            )
            FetchContent_MakeAvailable(paho-mqtt-c paho-mqtt-cpp)
            if(TARGET paho-mqtt-cpp::paho-mqtt-cpp)
                message(STATUS "Using paho-mqtt-cpp from FetchContent")
                set(PAHOMQTTCPP_FOUND TRUE)
            endif()
        endif()
        if(NOT PAHOMQTTCPP_FOUND)
            message(STATUS "MQTT support disabled")
        endif()
    endif()
else()
    # 子目录模式: 跳过FetchContent，使用SDK的依赖
    message(STATUS "NodeAgent: 使用SDK依赖 (跳过FetchContent)")
    set(NODEAGENT_BUILD_TESTS OFF)
    set(NODEAGENT_USE_MQTT OFF)
    set(PAHOMQTTCPP_FOUND FALSE)
endif()

# 查找 FalconMindSDK
if(NODEAGENT_STANDALONE)
    # 独立编译模式: SDK在兄弟目录
    find_path(FALCONMINDSDK_INCLUDE_DIR
        NAMES falconmind/sdk/core/Pipeline.h
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )

    find_library(FALCONMINDSDK_LIB
        NAMES falconmind_sdk
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../build
    )

    if(NOT FALCONMINDSDK_INCLUDE_DIR OR NOT FALCONMINDSDK_LIB)
        message(FATAL_ERROR "FalconMindSDK not found. Please build SDK first.")
    endif()
else()
    # 子目录集成模式: SDK已在父CMake中定义，直接使用
    if(NOT TARGET falconmind_sdk)
        message(FATAL_ERROR "FalconMindSDK target not found. Ensure SDK is built.")
    endif()
    message(STATUS "NodeAgent: 使用SDK目标 (falconmind_sdk)")
endif()

# 查找 OpenSSL（FlowExecutor 使用 cpp-httplib 需要 OpenSSL）
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
else()
    message(WARNING "OpenSSL not found. FlowExecutor HTTP features may not work.")
endif()

# 查找 zlib（cpp-httplib 需要，用于 gzip 压缩）
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    message(STATUS "Found ZLIB: ${ZLIB_VERSION}")
else()
    message(WARNING "ZLIB not found. HTTP compression features may not work.")
endif()

# 查找 brotli（cpp-httplib 可选，用于 brotli 压缩）
# 注意：brotli 可能不在标准 CMake 包中，需要查找多个库
find_library(BROTLI_COMMON_LIB NAMES brotlicommon brotli QUIET)
find_library(BROTLI_ENC_LIB NAMES brotlienc QUIET)
find_library(BROTLI_DEC_LIB NAMES brotlidec QUIET)
if(BROTLI_COMMON_LIB AND BROTLI_ENC_LIB AND BROTLI_DEC_LIB)
    message(STATUS "Found Brotli: ${BROTLI_COMMON_LIB}, ${BROTLI_ENC_LIB}, ${BROTLI_DEC_LIB}")
    set(BROTLI_LIBS ${BROTLI_COMMON_LIB} ${BROTLI_ENC_LIB} ${BROTLI_DEC_LIB})
else()
    message(STATUS "Brotli not found (optional, for HTTP compression)")
    set(BROTLI_LIBS "")
endif()

# NodeAgent 库
add_library(nodeagent
    src/NodeAgent.cpp
    src/UplinkClient.cpp
    src/DownlinkClient.cpp
    src/CommandHandler.cpp
    src/MissionHandler.cpp
    src/FlowHandler.cpp
    src/MessageAck.cpp
    src/MultiUavManager.cpp
    src/MqttUplinkClient.cpp
    src/MqttDownlinkClient.cpp
    src/Logger.cpp
    src/ErrorStatistics.cpp
    src/ReconnectManager.cpp
)

target_include_directories(nodeagent
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(NODEAGENT_STANDALONE)
    target_include_directories(nodeagent PUBLIC ${FALCONMINDSDK_INCLUDE_DIR})
    target_link_libraries(nodeagent PRIVATE ${FALCONMINDSDK_LIB})
else()
    target_include_directories(nodeagent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../FalconMindSDK/include)
    target_link_libraries(nodeagent PRIVATE falconmind_sdk)
endif()

target_link_libraries(nodeagent PRIVATE nlohmann_json::nlohmann_json)

# 如果找到 OpenSSL，链接 OpenSSL 库（FlowExecutor 需要）
if(OpenSSL_FOUND)
    target_link_libraries(nodeagent
        PRIVATE
            OpenSSL::SSL
            OpenSSL::Crypto
    )
endif()

# 如果找到 zlib，链接 zlib 库（cpp-httplib 需要）
if(ZLIB_FOUND)
    target_link_libraries(nodeagent
        PRIVATE
            ZLIB::ZLIB
    )
endif()

# 如果找到 brotli，链接 brotli 库（cpp-httplib 可选）
if(BROTLI_LIBS)
    target_link_libraries(nodeagent
        PRIVATE
            ${BROTLI_LIBS}
    )
endif()

# 如果启用了 MQTT，链接 paho-mqtt-cpp
if(PAHOMQTTCPP_FOUND AND TARGET paho-mqtt-cpp::paho-mqtt-cpp)
    target_link_libraries(nodeagent
        PRIVATE
            paho-mqtt-cpp::paho-mqtt-cpp
    )
    target_compile_definitions(nodeagent PRIVATE NODEAGENT_MQTT_ENABLED)
    message(STATUS "MQTT support enabled for nodeagent")
else()
    message(STATUS "MQTT support disabled for nodeagent")
endif()

# Demo 可执行程序
add_executable(nodeagent_demo
    demo/nodeagent_demo_main.cpp
)

target_link_libraries(nodeagent_demo PRIVATE nodeagent)

if(NOT NODEAGENT_STANDALONE)
    target_link_libraries(nodeagent_demo PRIVATE falconmind_sdk)
endif()

# Cluster Center Mock 服务器（用于测试）
add_executable(cluster_center_mock
    demo/cluster_center_mock.cpp
)

target_link_libraries(cluster_center_mock
    PRIVATE
        nlohmann_json::nlohmann_json
)

# 查找 libcurl（用于 HTTP POST 转发到 Viewer）
find_package(CURL QUIET)
if(CURL_FOUND)
    target_link_libraries(cluster_center_mock PRIVATE ${CURL_LIBRARIES})
    target_include_directories(cluster_center_mock PRIVATE ${CURL_INCLUDE_DIRS})
    message(STATUS "Found libcurl: ${CURL_LIBRARIES}")
else()
    # 如果系统没有 libcurl，尝试使用 pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(CURL libcurl)
        if(CURL_FOUND)
            target_link_libraries(cluster_center_mock PRIVATE ${CURL_LIBRARIES})
            target_include_directories(cluster_center_mock PRIVATE ${CURL_INCLUDE_DIRS})
            message(STATUS "Found libcurl via pkg-config: ${CURL_LIBRARIES}")
        else()
            message(WARNING "libcurl not found. Telemetry forwarding to Viewer will be disabled.")
            target_compile_definitions(cluster_center_mock PRIVATE NO_CURL)
        endif()
    else()
        message(WARNING "libcurl not found. Telemetry forwarding to Viewer will be disabled.")
        target_compile_definitions(cluster_center_mock PRIVATE NO_CURL)
    endif()
endif()

# Telemetry 数据流测试程序
add_executable(test_telemetry_flow
    demo/test_telemetry_flow.cpp
)

target_link_libraries(test_telemetry_flow PRIVATE nodeagent)

if(NODEAGENT_STANDALONE)
    target_link_libraries(test_telemetry_flow PRIVATE ${FALCONMINDSDK_LIB})
else()
    target_link_libraries(test_telemetry_flow PRIVATE falconmind_sdk)
endif()

# 下行消息测试程序
add_executable(test_downlink_demo
    demo/test_downlink_demo.cpp
)

target_link_libraries(test_downlink_demo PRIVATE nodeagent)

if(NODEAGENT_STANDALONE)
    target_link_libraries(test_downlink_demo PRIVATE ${FALCONMINDSDK_LIB})
else()
    target_link_libraries(test_downlink_demo PRIVATE falconmind_sdk)
endif()

# JSON 解析功能测试程序
add_executable(test_json_parsing
    demo/test_json_parsing.cpp
)

target_link_libraries(test_json_parsing
    PRIVATE
        nlohmann_json::nlohmann_json
)

# 单元测试可执行程序
if(NODEAGENT_BUILD_TESTS)
    add_executable(nodeagent_unit_tests
        tests/unit_tests.cpp
        tests/command_handler_tests.cpp
        tests/mission_handler_tests.cpp
        tests/message_ack_tests.cpp
        tests/multi_uav_tests.cpp
        tests/logger_tests.cpp
        tests/error_statistics_tests.cpp
        tests/reconnect_manager_tests.cpp
        tests/flow_handler_integration_tests.cpp
    )

    target_include_directories(nodeagent_unit_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    if(NODEAGENT_STANDALONE)
        target_include_directories(nodeagent_unit_tests PRIVATE ${FALCONMINDSDK_INCLUDE_DIR})
        target_link_libraries(nodeagent_unit_tests PRIVATE ${FALCONMINDSDK_LIB})
    else()
        target_include_directories(nodeagent_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../FalconMindSDK/include)
        target_link_libraries(nodeagent_unit_tests PRIVATE falconmind_sdk)
    endif()

    target_link_libraries(nodeagent_unit_tests
        PRIVATE
            nodeagent
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
    )

    # 如果找到 OpenSSL，链接 OpenSSL 库（FlowExecutor 需要）
    if(OpenSSL_FOUND)
        target_link_libraries(nodeagent_unit_tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()

    # 如果找到 zlib，链接 zlib 库（cpp-httplib 需要）
    if(ZLIB_FOUND)
        target_link_libraries(nodeagent_unit_tests PRIVATE ZLIB::ZLIB)
    endif()

    # 如果找到 brotli，链接 brotli 库（cpp-httplib 可选）
    if(BROTLI_LIBS)
        target_link_libraries(nodeagent_unit_tests PRIVATE ${BROTLI_LIBS})
    endif()

    # 启用测试发现
    include(GoogleTest)
    gtest_discover_tests(nodeagent_unit_tests)
endif()
