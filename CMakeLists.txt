cmake_minimum_required(VERSION 3.20)

project(gtx LANGUAGES CXX)

set(GTX_BACKEND "GLAD" CACHE STRING "GTX: backend implementation.")
set_property(CACHE GTX_BACKEND PROPERTY STRINGS "GLAD" "DX11" "VULKAN")

set(GTX_GLAD_LIBRARY "glad" CACHE STRING "GTX: provide the name of GLAD library to use with the GLAD backend.")

add_library(gtx STATIC "")
target_compile_features(gtx PUBLIC cxx_std_20)

if(GTX_BACKEND STREQUAL "GLAD")
    message(STATUS "GTX: Using OPENGL/GLAD")
    if(NOT ANDROID)
        find_package(OpenGL)
    endif()
    target_compile_definitions(gtx PUBLIC "GTX_OPENGL")
    target_compile_definitions(gtx PUBLIC "GTX_GLAD")
    if(GTX_GLSL_320ES)
        target_compile_definitions(gtx PUBLIC "GTX_GLSL_320ES")
    endif()
    target_link_libraries (gtx PUBLIC "${GTX_GLAD_LIBRARY}" "${OPENGL_gl_LIBRARY}")
    target_sources(gtx PUBLIC 
        "src/gl.cpp" 
        "src/gl-polyline.cpp"
    )

elseif(GTX_BACKEND STREQUAL "DX11")
    message(STATUS "GTX: Using DirectX 11")
    target_compile_definitions(gtx PUBLIC "GTX_DIRECTX")
    target_compile_definitions(gtx PUBLIC "GTX_DX11")
    target_link_libraries(gtx PUBLIC "d3d11" "dxgi" "D3DCompiler")
    target_sources(gtx PUBLIC 
        "src/dx.cpp" 
        "src/dx-polyline.cpp"
    )

elseif(GTX_BACKEND STREQUAL "VULKAN")
    message(STATUS "GTX: Using Vulkan")
    target_compile_definitions(gtx PUBLIC "GTX_VULKAN")
    find_package(Vulkan REQUIRED)
    target_link_libraries(gtx PUBLIC "Vulkan::Vulkan")
    target_sources(gtx PUBLIC
        "src/vk.cpp"
        "src/vk-polyline.cpp"
    )
endif()

target_include_directories(gtx PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
