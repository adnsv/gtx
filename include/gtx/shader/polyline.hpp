#pragma once

#include <cassert>
#include <gtx/geom/mat.hpp>
#include <gtx/geom/vec.hpp>
#include <gtx/vlist.hpp>

#if defined(GTX_DIRECTX)
#include <gtx/dx/dx.hpp>
#elif defined(GTX_OPENGL)
#include <gtx/gl/gl.hpp>
#elif defined(GTX_VULKAN)
#include <gtx/vk/vk.hpp>
#else
#error Undefined GTX implementation
#endif

namespace gtx::shdr {

struct polyline {
    struct vertex {
        vec2<float> pos;
        float thk;
        float dummy = 0.0f;
        vec4<float> clr;
        vertex(vec2<float> const& pos, float thk, vec4<float> const& clr)
            : pos{pos}
            , thk{thk}
            , clr{clr}
        {
        }
    };
    vlist<vertex> vertices;

    polyline();
    void setup_mvp(mat4x4 const& m);
    void render(std::size_t segment_id);

#if defined(GTX_DIRECTX)
    dx::vertex_shader vertex_shader;
    dx::geometry_shader geometry_shader;
    dx::pixel_shader pixel_shader;
    dx::input_layout layout;
    dx::vertex_buffer<vertex> vbuffer;
    dx::constant_buffer<float[4][4]> mvp;

#elif defined(GTX_OPENGL)
    gl::shader vertex_shader;
    gl::shader geometry_shader;
    gl::shader fragment_shader;
    gl::program program;
    gl::uniform mvp;
    gl::buffer<GL_ARRAY_BUFFER> vertex_buffer;
    gl::vertex_array vertex_array;
#elif defined(GTX_VULKAN)


#endif
};

} // namespace gtx::shdr