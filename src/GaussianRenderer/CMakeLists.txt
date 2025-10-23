set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/GaussianRenderer.cpp
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/TypeDef.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GaussianRendererCore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GaussianRenderer.h
)

set(MODULE_NAME GaussianRenderer)

add_library(${MODULE_NAME} SHARED ${SOURCES} ${HEADERS})

target_link_libraries(${MODULE_NAME} PRIVATE Renderer)

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

if(USE_GLES3)
    target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/vendor/glad-gles3/include)
else()
    target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/vendor/glad-gl/include)
endif()

target_compile_definitions(${MODULE_NAME} PRIVATE GAUSSIAN_RENDERER_EXPORTS)

if(UNIX)
    set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()