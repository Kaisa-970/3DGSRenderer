if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DGSENGINE_DEBUG)
endif()

set(ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Window/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Event/EventBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/GuiLayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelLoader/AssimpModelLoader.cpp
)

set(ENGINE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Window/Window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Event/Event.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Event/EventBus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/GuiLayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelLoader/AssimpModelLoader.h
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

set(MODULE_NAME GSEngine)

add_library(${MODULE_NAME} SHARED ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${IMGUI_SOURCES} ${IMGUI_HEADERS})

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/Renderer)
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/vendor/assimp/include)
target_include_directories(${MODULE_NAME} PRIVATE ${IMGUI_DIR})
target_include_directories(${MODULE_NAME} PRIVATE ${IMGUI_DIR}/backends)

target_link_libraries(${MODULE_NAME} PUBLIC Renderer Logger glfw assimp::assimp)

target_compile_definitions(${MODULE_NAME} PRIVATE GSENGINE_EXPORTS)

if(UNIX)
    set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()
