set(RENDERER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RendererContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/Primitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/CubePrimitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/SpherePrimitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/QuadPrimitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ForwardPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FinalPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GeometryPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Texture2D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stbimage/stb_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Renderable.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/RenderHelper/RenderHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LightingPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PostProcessPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Random.cpp
)

set(RENDERER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RendererContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/RenderCore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/TypeDef.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Vector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Matrix.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/Primitive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/CubePrimitive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/SpherePrimitive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/QuadPrimitive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameBuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ForwardPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FinalPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GeometryPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Texture2D.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stbimage/stb_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Material.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Model.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Renderable.h

    ${CMAKE_CURRENT_SOURCE_DIR}/RenderHelper/RenderHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LightingPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PostProcessPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Random.h
)

if(USE_GLES3)
    set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/src/vendor/glad-gles3/src/glad.c)
else()
    set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/src/vendor/glad-gl/src/glad.c)
endif()

set(MODULE_NAME Renderer)

add_library(${MODULE_NAME} SHARED ${RENDERER_SOURCES} ${RENDERER_HEADERS} ${GLAD_SOURCES})

if(USE_GLES3)
    target_compile_definitions(${MODULE_NAME} PUBLIC USE_GLES3)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${MODULE_NAME} PRIVATE OpenGL::GL)
endif()

target_link_libraries(${MODULE_NAME} PRIVATE glfw Logger)

if(USE_GLES3)
    target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/vendor/glad-gles3/include)
else()
    target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/vendor/glad-gl/include)
endif()

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${MODULE_NAME} PRIVATE RENDERER_EXPORTS)

if(UNIX)
    set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()
