set(RENDERER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RendererContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/Primitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/CubePrimitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/SpherePrimitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Primitives/QuadPrimitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AssimpModelLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ForwardPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FinalPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GeometryPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Texture2D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stbimage/stb_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MaterialManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TextureManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Renderable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/GuiLayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Event/EventBus.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/RenderHelper/RenderHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LightingPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PostProcessPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Random.cpp
)

set(RENDERER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Window.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/AssimpModelLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameBuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ForwardPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FinalPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GeometryPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Texture2D.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stbimage/stb_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Material.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MaterialManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TextureManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Model.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Renderable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Scene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/GuiLayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Event/Event.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Event/EventBus.h

    ${CMAKE_CURRENT_SOURCE_DIR}/RenderHelper/RenderHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LightingPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PostProcessPass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MathUtils/Random.h
)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/vendor/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

set(IMGUI_HEADERS
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/imstb_rectpack.h
    ${IMGUI_DIR}/imstb_truetype.h
    ${IMGUI_DIR}/imstb_textedit.h
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.h
    ${IMGUI_DIR}/backends/imgui_impl_glfw.h
)

if(USE_GLES3)
    set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/src/vendor/glad-gles3/src/glad.c)
else()
    set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/src/vendor/glad-gl/src/glad.c)
endif()

set(MODULE_NAME Renderer)

add_library(${MODULE_NAME} SHARED ${RENDERER_SOURCES} ${RENDERER_HEADERS} ${GLAD_SOURCES} ${IMGUI_SOURCES} ${IMGUI_HEADERS})

if(USE_GLES3)
    target_compile_definitions(${MODULE_NAME} PUBLIC USE_GLES3)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${MODULE_NAME} PRIVATE OpenGL::GL)
endif()

target_link_libraries(${MODULE_NAME} PRIVATE glfw Logger assimp::assimp)

if(USE_GLES3)
    target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/vendor/glad-gles3/include)
else()
    target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/vendor/glad-gl/include)
endif()

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/vendor/assimp/include)
target_compile_definitions(${MODULE_NAME} PRIVATE RENDERER_EXPORTS)

target_include_directories(${MODULE_NAME} PUBLIC ${IMGUI_DIR})
target_include_directories(${MODULE_NAME} PUBLIC ${IMGUI_DIR}/backends)

if(UNIX)
    set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()
