cmake_minimum_required(VERSION 3.16)
project(3DGSRenderer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(MSVC)
    add_compile_options(/W3)
    add_compile_options(/wd4251)
    add_compile_options(/utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 选项: 使用 OpenGL ES 3 而不是 OpenGL
option(USE_GLES3 "Use OpenGL ES 3 instead of OpenGL" OFF)

# 操作系统判断，并传入宏定义
if(WIN32)
    add_definitions(-DGSRENDERER_OS_WINDOWS)
elseif(APPLE)
    add_definitions(-DGSRENDERER_OS_MACOS)
elseif(UNIX)
    add_definitions(-DGSRENDERER_OS_LINUX)
endif()

# 子目录：spdlog 模块
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(src/vendor/spdlog)

add_subdirectory(src/vendor/glfw)

# 子目录：Logger 模块
add_subdirectory(src/Logger)

# 子目录：Renderer 模块
add_subdirectory(src/Renderer)

# 子目录：GaussianRenderer 模块
add_subdirectory(src/GaussianRenderer)

# 主项目源文件
set(SOURCES
    src/main.cpp

)

# 头文件
set(HEADERS

)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(${PROJECT_NAME}
        Logger
        Renderer
        GaussianRenderer
    )


if(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread dl)

    if( NOT USE_GLES3)
        target_link_libraries(${PROJECT_NAME}
            X11
            Xrandr
            Xinerama
            Xi
            Xcursor
        )
    endif()
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# # 复制着色器文件到构建目录
# file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR}/bin)
# file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/bin)