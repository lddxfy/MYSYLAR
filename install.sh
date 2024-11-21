#!/bin/bash

# 定义目标目录
TARGET_DIR="/usr/include/sylar"

# 判断目标目录是否存在，如果不存在则创建
if [ ! -d "$TARGET_DIR" ]; then
    sudo mkdir -p "$TARGET_DIR"
    echo "Created directory: $TARGET_DIR"
else
    echo "Directory $TARGET_DIR already exists."
fi

# 安装头文件到目标目录
sudo cp -r sylar/include/* "$TARGET_DIR/"

# 安装动态库到 /usr/lib
sudo cp lib/libsylar.so /usr/lib/

# 更新动态链接器缓存
sudo ldconfig

echo "Library and headers have been successfully installed."