#!/bin/bash

# 默认构建类型为 Debug
BUILD_TYPE=${1:-Debug}

# 第二个参数可选：gles3 或 opengl (默认 opengl)
GRAPHICS_API=${2:-opengl}

# 将参数转换为首字母大写
BUILD_TYPE=$(echo "$BUILD_TYPE" | sed 's/^\(.\)/\U\1/')

# 将图形 API 转换为小写
GRAPHICS_API=$(echo "$GRAPHICS_API" | tr '[:upper:]' '[:lower:]')

# 验证构建类型
if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
    echo "Error: Invalid build type '$BUILD_TYPE'"
    echo "Usage: $0 [Debug|Release] [opengl|gles3]"
    echo "  The first parameter defaults to Debug"
    echo "  The second parameter defaults to opengl"
    exit 1
fi

# 验证图形 API
if [[ "$GRAPHICS_API" != "opengl" && "$GRAPHICS_API" != "gles3" ]]; then
    echo "Error: Invalid graphics API '$GRAPHICS_API'"
    echo "Usage: $0 [Debug|Release] [opengl|gles3]"
    echo "  The first parameter defaults to Debug"
    echo "  The second parameter defaults to opengl"
    exit 1
fi

# 获取 CPU 核心数用于并行编译
NPROC=8

OUTPUT_DIR="./output"
OUTPUT_BIN_DIR="$OUTPUT_DIR/$BUILD_TYPE/bin"
mkdir -p $OUTPUT_BIN_DIR

# 设置 CMake 选项
CMAKE_OPTIONS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

if [[ "$GRAPHICS_API" == "gles3" ]]; then
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DUSE_GLES3=ON"
else
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DUSE_GLES3=OFF"
fi

echo "================================"
echo "  Build type: $BUILD_TYPE"
echo "  Graphics API: $GRAPHICS_API"
echo "  Parallel threads: $NPROC"
echo "================================"

# 配置 CMake
cmake -B build $CMAKE_OPTIONS

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OUTPUT_BIN_DIR

cmake --build build --target Renderer -j$NPROC

if [ $? -ne 0 ]; then
    echo "Compile Renderer module failed!"
    exit 1
fi

cp build/src/Renderer/libRenderer.so $OUTPUT_BIN_DIR/libRenderer.so

cmake --build build --target 3DGSRenderer -j$NPROC

if [ $? -ne 0 ]; then
    echo "Compile main executable failed!"
    exit 1
fi

# # 编译（使用多线程）
# cmake --build build -j$NPROC

# # 检查 build 是否成功
# if [ $? -ne 0 ]; then
#     echo "Compile failed!"
#     exit 1
# fi

cp build/bin/3DGSRenderer $OUTPUT_BIN_DIR/3DGSRenderer

echo "Compile success!"
echo "Run program: $OUTPUT_BIN_DIR/3DGSRenderer"