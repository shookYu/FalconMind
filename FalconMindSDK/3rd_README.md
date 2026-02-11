# 3rd 目录说明

## 目录用途

`3rd/` 目录用于存储通过 CMake FetchContent 自动下载的第三方依赖库。

## 包含的依赖库

- **json-src/** - nlohmann/json 库（如果系统未安装）
- **httplib-src/** - cpp-httplib 库（如果系统未安装）

## 目录结构

```
3rd/
├── json-src/          # nlohmann/json 源码
└── httplib-src/       # cpp-httplib 源码
```

## 注意事项

1. **此目录由 CMake 自动管理**：首次编译时会自动创建并下载依赖库
2. **不会重复下载**：如果目录已存在，CMake 不会重新下载
3. **可以删除**：如果需要更新依赖库版本，可以删除对应目录后重新编译
4. **已添加到 .gitignore**：此目录不会被提交到 Git 仓库

## 清理依赖库

如果需要强制重新下载依赖库：

```bash
cd FalconMindSDK
rm -rf 3rd
cd build
cmake ..  # 会重新下载依赖库
```

## 使用系统库（推荐）

如果系统已安装依赖库，CMake 会自动使用，不会下载到 `3rd/` 目录：

```bash
# 安装系统库
sudo apt-get install nlohmann-json3-dev

# 编译时自动使用系统库，不会下载
cd build
cmake ..
```
