#pragma once

#include <gtx/geom/vec.hpp>

namespace gtx {

struct xform {
    float m11 = 1.0f; //  transformation matrix is  | m11 m21 dx |
    float m12 = 0.0f; //                            | m12 m22 dy |
    float m21 = 0.0f; //                            | 0   0   1  |
    float m22 = 1.0f;
    float dx = 0.0f;
    float dy = 0.0f;

    constexpr xform() {}
    constexpr xform(xform const&) = default;
    constexpr xform(
        float m11, float m12, float m21, float m22, float dx, float dy)
        : m11(m11)
        , m12(m12)
        , m21(m21)
        , m22(m22)
        , dx(dx)
        , dy(dy)
    {
    }

    static auto rotate(float angle_rad) -> xform
    {
        auto s = std::sin(angle_rad);
        auto c = std::cos(angle_rad);
        return xform{c, -s, s, c, 0, 0};
    }

    static constexpr auto translate(float tx, float ty) -> xform
    {
        return xform{1.0f, 0, 0, 1.0f, tx, ty};
    }

    static constexpr auto scale(float sx, float sy) -> xform
    {
        return xform{sx, 0, 0, sy, 0, 0};
    }

    friend constexpr auto operator*(xform const& a, xform const& b) -> xform
    {
        return {
            a.m11 * b.m11 + a.m21 * b.m12,
            a.m12 * b.m11 + a.m22 * b.m12,
            a.m11 * b.m21 + a.m21 * b.m22,
            a.m12 * b.m21 + a.m22 * b.m22,
            a.m11 * b.dx + a.m21 * b.dy + a.dx,
            a.m12 * b.dx + a.m22 * b.dy + a.dy,
        };
    }

    template <typename T> auto operator()(vec2<T> const& v) -> vec2<float>
    {
        return {m11 * v.x + m21 * v.y + dx, m12 * v.x + m22 * v.y + dy};
    }

    template <typename T>
    friend constexpr auto operator*(xform const& a, vec2<T> const& v)
        -> vec2<float>
    {
        return {
            a.m11 * v.x + a.m21 * v.y + a.dx, a.m12 * v.x + a.m22 * v.y + a.dy};
    }
};

} // namespace gtx