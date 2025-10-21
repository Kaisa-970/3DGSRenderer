cmake_minimum_required(VERSION 3.16)
project(3DGSRenderer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 查找依赖库
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/vendor/glad/include)
# include_directories(${CMAKE_SOURCE_DIR}/src/vendor/glm)

# GLAD 源文件
set(GLAD_SOURCES
    ${CMAKE_SOURCE_DIR}/src/vendor/glad/src/glad.c
)

# 主项目源文件
set(SOURCES
    src/main.cpp

)

# 头文件
set(HEADERS

)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLAD_SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    glfw
    pthread
    dl
    X11
    Xrandr
    Xinerama
    Xi
    Xcursor
)

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# # 复制着色器文件到构建目录
# file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR}/bin)
# file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/bin)

# 调试信息
message(STATUS "OpenGL found: ${OPENGL_FOUND}")
message(STATUS "GLFW3 found: ${glfw3_FOUND}")
message(STATUS "GLAD sources: ${GLAD_SOURCES}")