/**
 * @file BuiltinPlugins.h
 * @brief 内建插件 - SDK默认能力实现
 * 
 * 这些插件作为SDK的基础能力，在初始化时自动注册，
 * 但可以通过动态加载的插件替换。
 */

#pragma once

#include "falconmind/sdk/plugin/IPlugin.h"
#include <vector>
#include <string>

namespace falconmind {
namespace sdk {
namespace plugin {

/**
 * @brief 注册所有内建插件
 * 
 * 在SDK初始化时调用，注册默认实现
 */
void registerBuiltinPlugins();

/**
 * @brief 注销所有内建插件
 */
void unregisterBuiltinPlugins();

} // namespace plugin
} // namespace sdk
} // namespace falconmind
