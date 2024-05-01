cmake_minimum_required(VERSION 3.20)

project(gtx LANGUAGES CXX)

#set(GTX_BACKEND "GLAD" CACHE STRING "GTX Backend")
#set(GTX_BACKEND "DX11" CACHE STRING "GTX Backend")

# note GTX_BACKEND must be configured to GLAD or DX11

add_library(gtx STATIC "")
target_compile_features(gtx PUBLIC cxx_std_20)

if(GTX_BACKEND STREQUAL "GLAD")
    message(STATUS "GTX -- Using OPENGL/GLAD")
    target_compile_definitions(gtx PUBLIC "GTX_OPENGL")
    target_compile_definitions(gtx PUBLIC "GTX_GLAD")
    target_link_libraries (gtx PUBLIC "glad" "${OPENGL_gl_LIBRARY}")
    target_sources(gtx PUBLIC "src/page-gl.cpp")

    if(GTX_GLSL_320ES)
        target_sources(gtx PUBLIC "src/polyline-glsl-320es.cpp")
    else()
        target_sources(gtx PUBLIC "src/polyline-glsl-330.cpp")
    endif()

elseif(GTX_BACKEND STREQUAL "DX11")
    message(STATUS "GTX -- Using DirectX 11")
    target_compile_definitions(gtx PUBLIC "GTX_DIRECTX")
    target_compile_definitions(gtx PUBLIC "GTX_DX11")
    target_link_libraries(gtx PUBLIC "d3d11" "dxgi" "D3DCompiler")
    target_sources(gtx PUBLIC 
        "src/page-dx.cpp" 
        "src/shader-dx.cpp"
        "src/polyline-hlsl.cpp")

endif()


target_include_directories(gtx PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
