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

# 检查 Ninja 是否安装
if ! command -v ninja &> /dev/null; then
    echo "Error: Ninja is not installed"
    echo "Please install it with: sudo apt-get install ninja-build"
    exit 1
fi

# 获取 CPU 核心数用于并行编译
NPROC=$(nproc 2>/dev/null || echo 8)

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
echo "  Generator: Ninja"
echo "  Parallel threads: $NPROC"
echo "================================"

# 配置 CMake
echo ""
echo "Configuring CMake..."
cmake -B build -G Ninja $CMAKE_OPTIONS

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OUTPUT_BIN_DIR

# 构建 Logger 模块
echo ""
echo "Building Logger module..."
cmake --build build --target Logger --config $BUILD_TYPE -j$NPROC

if [ $? -ne 0 ]; then
    echo "Compile Logger module failed!"
    exit 1
fi

# 复制 Logger 库文件
LOGGER_SO="build/src/Logger/libLogger.so"
if [ -f "$LOGGER_SO" ]; then
    cp "$LOGGER_SO" "$OUTPUT_BIN_DIR/libLogger.so"
    echo "Copied libLogger.so to output directory"
fi

# 构建 Renderer 模块
echo ""
echo "Building Renderer module..."
cmake --build build --target Renderer --config $BUILD_TYPE -j$NPROC

if [ $? -ne 0 ]; then
    echo "Compile Renderer module failed!"
    exit 1
fi

# 复制 Renderer 库文件
RENDERER_SO="build/src/Renderer/libRenderer.so"
if [ -f "$RENDERER_SO" ]; then
    cp "$RENDERER_SO" "$OUTPUT_BIN_DIR/libRenderer.so"
    echo "Copied libRenderer.so to output directory"
fi

# 构建主程序
echo ""
echo "Building main executable..."
cmake --build build --target 3DGSRenderer --config $BUILD_TYPE -j$NPROC

if [ $? -ne 0 ]; then
    echo "Compile main executable failed!"
    exit 1
fi

# 复制可执行文件
EXE_PATH="build/bin/3DGSRenderer"
if [ ! -f "$EXE_PATH" ]; then
    # 尝试其他可能的路径
    EXE_PATH="build/bin/$BUILD_TYPE/3DGSRenderer"
fi
if [ ! -f "$EXE_PATH" ]; then
    EXE_PATH="build/$BUILD_TYPE/3DGSRenderer"
fi

if [ -f "$EXE_PATH" ]; then
    cp "$EXE_PATH" "$OUTPUT_BIN_DIR/3DGSRenderer"
    echo ""
    echo "Compile success!"
    echo "Run program: $OUTPUT_BIN_DIR/3DGSRenderer"
else
    echo ""
    echo "Warning: Could not find 3DGSRenderer executable"
    echo "Build may have succeeded but executable was not found at expected location"
fi
