#include "hlsl-polyline.hpp"
#include <gtx/shader/polyline.hpp>

#include <cstdio>

namespace gtx::shdr {

static void sh_error(dx::shader_error_info const& info)
{
    std::printf("shader compile (%s, %s): %s\n%s", info.entry_point,
        info.target, _com_error{info.hr}.ErrorMessage(), info.error_msg);
    exit(-1);
}

inline static const dx::shader_source source = {
    "polyline", POLYLINE_SHADER_HLSL};

inline static dx::shader_code vs_code = {
    source, dx::macro("VERTEX_SHADER"), "vertex_shader", "vs_5_0", sh_error};
inline static dx::shader_code gs_code = {source, dx::macro("GEOMETRY_SHADER"),
    "geometry_shader", "gs_5_0", sh_error};
inline static dx::shader_code ps_code = {
    source, dx::macro("PIXEL_SHADER"), "pixel_shader", "ps_5_0", sh_error};

polyline::polyline()
    : vertex_shader{vs_code}
    , geometry_shader{gs_code}
    , pixel_shader{ps_code}
    , layout{vs_code, // match vertex
          {
              {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(vertex, pos)},
              {"THK", 0, DXGI_FORMAT_R32_FLOAT, 0, offsetof(vertex, thk)},
              {"CLR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                  offsetof(vertex, clr)},
          }}
    , vbuffer{}
    , mvp{}
{
}

void polyline::setup_mvp(mat4x4 const& m) { mvp.update(m.elts); }

void polyline::render(std::size_t segment_id)
{
    auto indices = vertices.segments();
    if (segment_id >= indices.size())
        return;
    auto const& index_range = indices[segment_id];

    auto ctx = get_device().context;
    if (!ctx)
        throw dx::error("missing device");

    auto restore_when_done = gtx::dx::save<   //
        gtx::dx::state::vertex_shader,        //
        gtx::dx::state::geometry_shader,      //
        gtx::dx::state::pixel_shader,         //
        gtx::dx::state::vs_constant_buffer_0, //
        gtx::dx::state::gs_constant_buffer_0, //
        gtx::dx::state::vertex_buffer_0,      //
        gtx::dx::state::input_layout,         //
        gtx::dx::state::primitive_topology    //
        >{};

    if (vertices.dirty_flag()) {
        vertices.reset_dirty_flag();
        vbuffer.write(vertices.vertices().data(), vertices.vertices().size());
    }

    vertex_shader.bind();
    geometry_shader.bind();
    pixel_shader.bind();
    layout.bind();
    vbuffer.bind(0);
    mvp.bind_gs(0);

    ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
    ctx->Draw(index_range.last - index_range.first + 1, index_range.first);
}

} // namespace gtx::shdr