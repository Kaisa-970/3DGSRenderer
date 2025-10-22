set(LOGGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Log.cpp
)

set(LOGGER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Log.h
)

set(MODULE_NAME Logger)

add_library(${MODULE_NAME} SHARED ${LOGGER_SOURCES} ${LOGGER_HEADERS})

# 链接spdlog
target_link_libraries(${MODULE_NAME} PUBLIC spdlog)

# 设置包含目录
target_include_directories(${MODULE_NAME} PUBLIC 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置导出宏
target_compile_definitions(${MODULE_NAME} PRIVATE LOGGER_EXPORTS)

# Unix平台下设置符号可见性
if(UNIX)
    set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

