#include "glsl330-polyline.hpp"
#include <gtx/shader/polyline.hpp>

namespace gtx::shdr {

polyline::polyline()
    : vertex_shader(shaderc_glsl_vertex_shader, //
          "POLYLINE_VERTEX_SHADER_GLSL", POLYLINE_VERTEX_SHADER_GLSL)
    , geometry_shader{shaderc_glsl_geometry_shader, //
          "POLYLINE_GEOMETRY_SHADER_GLSL", POLYLINE_GEOMETRY_SHADER_GLSL}
    , fragment_shader{shaderc_glsl_fragment_shader, //
          "POLYLINE_FRAGMENT_SHADER_GLSL", POLYLINE_FRAGMENT_SHADER_GLSL}
{
}

void polyline::setup_mvp(mat4x4 const& m) {}

void polyline::render(std::size_t segment_id) {}

} // namespace gtx::shdr