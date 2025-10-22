# 3D Gaussian Splatting Renderer

一个基于 OpenGL 的 3D Gaussian Splatting 渲染器实现。

## 系统要求

- OpenGL 4.2+
- CMake 3.16+
- C++17
- GLFW3, GLM, GLAD

## 快速开始

### 1. 编译项目

```bash
# 编译 Debug 版本（默认）
./build.sh

# 编译 Release 版本
./build.sh Release

# 或者手动指定
./build.sh Debug
```

### 2. 运行程序

```bash
# 使用默认 GPU 运行
./run.sh

# 尝试使用 NVIDIA GPU 运行（如果有多显卡）
./run.sh nvidia
```

### 3. 直接运行可执行文件

```bash
./build/bin/3DGSRenderer
```

## 项目结构

```
3DGSRenderer/
├── CMakeLists.txt          # CMake 配置文件
├── build.sh                # 构建脚本
├── run.sh                  # 运行脚本
├── src/
│   ├── main.cpp           # 主程序入口
│   ├── vendor/
│   │   └── glad/          # GLAD OpenGL 加载器
│   ├── core/              # 核心系统（待实现）
│   ├── renderer/          # 渲染系统（待实现）
│   └── 3dgs/              # 3DGS 渲染逻辑（待实现）
├── shaders/               # GLSL 着色器（待创建）
└── assets/                # 资源文件（待创建）
```

## 开发路线图

- [x] 基础 OpenGL 窗口和渲染循环
- [ ] 着色器管理系统
- [ ] 相机系统
- [ ] 输入处理
- [ ] PLY 文件加载器
- [ ] Gaussian Splatting 渲染
- [ ] 深度排序与混合
- [ ] 性能优化

## 已知问题

- WSL 环境下可能无法正确切换到 NVIDIA GPU，需要在 Windows 端通过 NVIDIA 控制面板设置

## License

MIT

