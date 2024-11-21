#!/bin/bash

# 定义目标目录
TARGET_DIR="/usr/include/sylar"

# 删除头文件
if [ -d "$TARGET_DIR" ]; then
    sudo rm -rf "$TARGET_DIR"
    echo "Removed directory: $TARGET_DIR"
else
    echo "Directory $TARGET_DIR does not exist."
fi

# 删除动态库
sudo rm -f /usr/lib/libsylar.so

# 更新动态链接器缓存
sudo ldconfig

echo "Library and headers have been successfully removed."