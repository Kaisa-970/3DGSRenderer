#!/bin/bash

BUILD_TYPE=${1:-Debug}

# 解析参数
USE_NVIDIA=true
if [ "$1" = "nvidia" ] || [ "$1" = "NVIDIA" ]; then
    USE_NVIDIA=true
fi

# 设置 NVIDIA GPU 环境变量（如果指定）
if [ "$USE_NVIDIA" = true ]; then
    # export __NV_PRIME_RENDER_OFFLOAD=1
    # export __GLX_VENDOR_LIBRARY_NAME=nvidia
    # export __VK_LAYER_NV_optimus=NVIDIA_only
    export MESA_D3D12_DEFAULT_ADAPTER_NAME=NVIDIA
    echo "=== Try to use NVIDIA GPU ==="
else
    echo "=== Use default GPU ==="
    echo "Tip: Use './run.sh nvidia' to try to enable NVIDIA GPU"
fi

OUTPUT_DIR="./output"
OUTPUT_BIN_DIR="$OUTPUT_DIR/$BUILD_TYPE/bin"
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OUTPUT_BIN_DIR

# 运行程序
$OUTPUT_BIN_DIR/3DGSRenderer
