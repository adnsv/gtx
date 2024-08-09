#pragma once

namespace gtx {

constexpr char const* POLYLINE_SHADER_HLSL = R"(
cbuffer constants : register(b0)
{
    matrix mvp;
};

#if defined(VERTEX_SHADER) || defined(GEOMETRY_SHADER)
struct vertex {
    float2 pos : POS;   // x, y
    float  thk : THK;
    float4 clr : CLR;   // r, g, b, a
};
#endif

struct pixel {
    float4 pos : SV_Position;
    float4 clr : RGBA_NORM;
};

#ifdef VERTEX_SHADER
vertex vertex_shader(vertex v)
{
    v.thk = max(0.25, (v.thk - 1.0) * 0.5); // half-thickness
    return v;
};
#endif

#ifdef GEOMETRY_SHADER

#define emit(P, C) \
    p.pos = mul(mvp, float4((P).xy, 0, 1)); \
    p.clr = (C); \
    stream.Append(p)

float cross2(float2 a, float2 b) 
{
    return a.x * b.y - b.x * a.y;
}

void fan(inout TriangleStream<pixel> stream, 
    float2 pos, float2 c, float2 a, float2 m, float2 b, float r, float4 clr, float4 edg) {
    pixel p;
    float2 am = normalize(a + m);
    float2 mb = normalize(m + b);
    float re = r + 1;
    emit(pos, clr);
    emit(c + r * a, clr);
    emit(c + r * am, clr);
    emit(c + re * a, edg);
    emit(c + re * am, edg);
    stream.RestartStrip();
    emit(pos, clr);
    emit(c + r * am, clr);
    emit(c + r * m, clr);
    emit(c + re * am, edg);
    emit(c + re * m, edg);
    stream.RestartStrip();
    emit(pos, clr);
    emit(c + r * m, clr);
    emit(c + r * mb, clr);
    emit(c + re * m, edg);
    emit(c + re * mb, edg);
    stream.RestartStrip();
    emit(pos, clr);
    emit(c + r * mb, clr);
    emit(c + r * b, clr);
    emit(c + re * mb, edg);
    emit(c + re * b, edg);
    stream.RestartStrip();
}

[maxvertexcount(64)]
void geometry_shader(lineadj vertex g[4], inout TriangleStream<pixel> stream) 
{
    float2 p0 = g[0].pos;
    float2 p1 = g[1].pos;
    float2 p2 = g[2].pos;
    float2 p3 = g[3].pos;

    float4 c1 = g[1].clr;
    float4 c2 = g[2].clr;
    float4 c1e = float4(c1.xyz, 0);
    float4 c2e = float4(c2.xyz, 0);

    float th1 = g[1].thk;
    float th2 = g[2].thk;

    float2 d0 = normalize(p1 - p0);
    float2 d1 = normalize(p2 - p1);
    float2 d2 = normalize(p3 - p2);

    float2 n0 = float2(-d0.y, d0.x);
    float2 n1 = float2(-d1.y, d1.x);
    float2 n2 = float2(-d2.y, d2.x);

    float dp1 = dot(d0, d1);
    float dp2 = dot(d1, d2);

    float cw1 = 2 * step(cross2(d0, d1), 0) - 1;
    float cw2 = 2 * step(cross2(d1, d2), 0) - 1;

    float2 t1a = n1;
    float2 t2a = n1;
    float2 t1b = n1;
    float2 t2b = n1;

    float2 m1 = (n0 + n1) / (1.0 + dp1);
    float2 m2 = (n1 + n2) / (1.0 + dp2);

    pixel p;

    if (p0.x == p1.x && p0.y == p1.y) {
        // start
        fan(stream, p1, p1, n1, -d1, -n1, th1, c1, c1e);
    }
    else if (dp1 < -0.25) {
        // overlap joint
        fan(stream, p1, p1, n0 * cw1, normalize(d0-d1), n1 * cw1, th1, c1, c1e); 
    }
    else if (dp1 > 0.85) {
        // miter
        t1a = m1;
        t1b = t1a;
    }
    else {
        // rounded joint
        fan(stream, p1 - m1 * th1 * cw1, p1, n0 * cw1, normalize(n0 + n1) * cw1,  n1 * cw1, th1, c1, c1e);
        t1a = lerp(t1a, m1, float(cw1 < 0));
        t1b = lerp(t1b, m1, float(cw1 >= 0));        
    }

    if (p2.x == p3.x && p2.y == p3.y) {
        // end
        fan(stream, p2, p2, -n1, d1, n1, th2, c2, c2e);
    }
    else if (dp2 >= -0.25) {
        // before rounded joint   
        t2a = lerp(t2a, m2, float(cw2 < 0 || dp2 > 0.85));
        t2b = lerp(t2b, m2, float(cw2 >= 0 || dp2 > 0.85));
    }

    emit(p1 + t1a * th1, c1);
    emit(p1 - t1b * th1, c1);
    emit(p2 + t2a * th2, c2);
    emit(p2 - t2b * th2, c2);
    stream.RestartStrip();

    // aa edge
    emit(p1 + t1a * (th1 + 1), c1e);
    emit(p1 + t1a * th1, c1);
    emit(p2 + t2a * (th2 + 1), c2e);
    emit(p2 + t2a * th2, c2);
    stream.RestartStrip();

    emit(p1 - t1b * th1, c1);
    emit(p1 - t1b * (th1 + 1), c1e);
    emit(p2 - t2b * th2, c2);
    emit(p2 - t2b * (th2 + 1), c2e);
    stream.RestartStrip();
}
#endif

#if defined(PIXEL_SHADER)
float4 pixel_shader(pixel p) : SV_Target
{
    return p.clr;
}
#endif

)";

}