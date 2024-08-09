
#include <gtx/shader/polyline.hpp>

#ifdef GTX_GLSL_320ES
#include "glsl320es-polyline.hpp"
#else
#include "glsl330-polyline.hpp"
#endif

namespace gtx::shdr {

polyline::polyline()
    : vertex_shader{GL_VERTEX_SHADER, POLYLINE_VERTEX_SHADER_GLSL}
    , geometry_shader{GL_GEOMETRY_SHADER, POLYLINE_GEOMETRY_SHADER_GLSL}
    , fragment_shader{GL_FRAGMENT_SHADER, POLYLINE_FRAGMENT_SHADER_GLSL}
    , program{{vertex_shader, geometry_shader, fragment_shader}}
    , mvp{program, "mvp"}
    , vertex_buffer{}
    , vertex_array{vertex_buffer, program,
          {
              gl::vertex_attrib{"in_pos", 2, gl::comp::f32_unorm,
                  sizeof(vertex), offsetof(vertex, pos)},
              gl::vertex_attrib{"in_thk", 1, gl::comp::f32_unorm,
                  sizeof(vertex), offsetof(vertex, thk)},
              gl::vertex_attrib{"in_clr", 4, gl::comp::f32_norm, sizeof(vertex),
                  offsetof(vertex, clr)},
          }}
{
}

void polyline::setup_mvp(mat4x4 const& m)
{
    auto restore_when_done = gl::save<gl::state::program>{};
    program.use();
    mvp = m.elts;
}

void polyline::render(std::size_t segment_id)
{
    auto indices = vertices.segments();
    if (segment_id >= indices.size())
        return;
    auto const& index_range = indices[segment_id];

    auto restore_when_done = gtx::gl::save<gtx::gl::state::array_buffer,
        gtx::gl::state::vertex_array, gtx::gl::state::program>{};

    program.use();
    vertex_array.bind();
    if (vertices.dirty_flag()) {
        vertices.reset_dirty_flag();
        vertex_buffer.data(vertices.vertices().size_bytes(),
            vertices.vertices().data(), GL_DYNAMIC_DRAW);
    }
    glDrawArrays(GL_LINE_STRIP_ADJACENCY, index_range.first,
        index_range.last - index_range.first);
}

} // namespace gtx::shdr