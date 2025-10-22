# build.ps1 - Windows 构建脚本

param(
    [string]$BuildType = "Debug",
    [string]$GraphicsAPI = "opengl"
)

# 将构建类型转换为首字母大写
$BuildType = (Get-Culture).TextInfo.ToTitleCase($BuildType.ToLower())

# 将图形 API 转换为小写
$GraphicsAPI = $GraphicsAPI.ToLower()

# 验证构建类型
if ($BuildType -ne "Debug" -and $BuildType -ne "Release") {
    Write-Host "Error: Invalid build type '$BuildType'" -ForegroundColor Red
    Write-Host "Usage: .\build.ps1 [Debug|Release] [opengl|gles3]"
    Write-Host "  The first parameter defaults to Debug"
    Write-Host "  The second parameter defaults to opengl"
    exit 1
}

# 验证图形 API
if ($GraphicsAPI -ne "opengl" -and $GraphicsAPI -ne "gles3") {
    Write-Host "Error: Invalid graphics API '$GraphicsAPI'" -ForegroundColor Red
    Write-Host "Usage: .\build.ps1 [Debug|Release] [opengl|gles3]"
    Write-Host "  The first parameter defaults to Debug"
    Write-Host "  The second parameter defaults to opengl"
    exit 1
}

# 获取 CPU 核心数用于并行编译
$NPROC = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
if (-not $NPROC) {
    $NPROC = 8
}

$OUTPUT_DIR = ".\output"
$OUTPUT_BIN_DIR = "$OUTPUT_DIR\$BuildType\bin"
New-Item -ItemType Directory -Force -Path $OUTPUT_BIN_DIR | Out-Null

# 设置 CMake 选项
$CMAKE_OPTIONS = @("-DCMAKE_BUILD_TYPE=$BuildType")

if ($GraphicsAPI -eq "gles3") {
    $CMAKE_OPTIONS += "-DUSE_GLES3=ON"
} else {
    $CMAKE_OPTIONS += "-DUSE_GLES3=OFF"
}

Write-Host "================================" -ForegroundColor Cyan
Write-Host "  Build type: $BuildType" -ForegroundColor Cyan
Write-Host "  Graphics API: $GraphicsAPI" -ForegroundColor Cyan
Write-Host "  Parallel threads: $NPROC" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan

# 配置 CMake
Write-Host "`nConfiguring CMake..." -ForegroundColor Yellow
cmake -B build @CMAKE_OPTIONS

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# 构建 Logger 模块
Write-Host "`nBuilding Logger module..." -ForegroundColor Yellow
cmake --build build --target Logger --config $BuildType -j $NPROC

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compile Logger module failed!" -ForegroundColor Red
    exit 1
}

# 复制 Logger 库文件
$LoggerLib = "build\src\Logger\$BuildType\Logger.lib"
$LoggerDll = "build\src\Logger\$BuildType\Logger.dll"
if (Test-Path $LoggerDll) {
    Copy-Item $LoggerDll -Destination "$OUTPUT_BIN_DIR\Logger.dll" -Force
    Write-Host "Copied Logger.dll to output directory" -ForegroundColor Green
}
if (Test-Path $LoggerLib) {
    Copy-Item $LoggerLib -Destination "$OUTPUT_BIN_DIR\Logger.lib" -Force
    Write-Host "Copied Logger.lib to output directory" -ForegroundColor Green
}

# 构建 Renderer 模块
Write-Host "`nBuilding Renderer module..." -ForegroundColor Yellow
cmake --build build --target Renderer --config $BuildType -j $NPROC

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compile Renderer module failed!" -ForegroundColor Red
    exit 1
}

# 复制 Renderer 库文件
$RendererLib = "build\src\Renderer\$BuildType\Renderer.lib"
$RendererDll = "build\src\Renderer\$BuildType\Renderer.dll"
if (Test-Path $RendererDll) {
    Copy-Item $RendererDll -Destination "$OUTPUT_BIN_DIR\Renderer.dll" -Force
    Write-Host "Copied Renderer.dll to output directory" -ForegroundColor Green
}
if (Test-Path $RendererLib) {
    Copy-Item $RendererLib -Destination "$OUTPUT_BIN_DIR\Renderer.lib" -Force
    Write-Host "Copied Renderer.lib to output directory" -ForegroundColor Green
}

# 构建主程序
Write-Host "`nBuilding main executable..." -ForegroundColor Yellow
cmake --build build --target 3DGSRenderer --config $BuildType -j $NPROC

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compile main executable failed!" -ForegroundColor Red
    exit 1
}

# 复制可执行文件
$ExePath = "build\bin\$BuildType\3DGSRenderer.exe"
if (-not (Test-Path $ExePath)) {
    # 尝试另一个可能的路径
    $ExePath = "build\$BuildType\3DGSRenderer.exe"
}

if (Test-Path $ExePath) {
    Copy-Item $ExePath -Destination "$OUTPUT_BIN_DIR\3DGSRenderer.exe" -Force
    Write-Host "`nCompile success!" -ForegroundColor Green
    Write-Host "Run program: $OUTPUT_BIN_DIR\3DGSRenderer.exe" -ForegroundColor Green
} else {
    Write-Host "`nWarning: Could not find 3DGSRenderer.exe" -ForegroundColor Yellow
    Write-Host "Build may have succeeded but executable was not found at expected location" -ForegroundColor Yellow
}


