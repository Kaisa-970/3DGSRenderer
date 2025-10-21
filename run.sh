#!/bin/bash
# 强制使用 NVIDIA GPU 运行程序

# export __NV_PRIME_RENDER_OFFLOAD=1
# export __GLX_VENDOR_LIBRARY_NAME=nvidia
# export __VK_LAYER_NV_optimus=NVIDIA_only

export MESA_D3D12_DEFAULT_ADAPTER_NAME=NVIDIA
echo "=== Running with NVIDIA GPU ==="
./build/bin/3DGSRenderer

