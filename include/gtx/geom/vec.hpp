#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace gtx {

template <typename T> struct vec2 {
    using component_type = T;

    T x;
    T y;

    constexpr vec2() noexcept
        : x{}
        , y{}
    {
    }
    constexpr vec2(component_type const& x, component_type const& y) noexcept
        : x{x}
        , y{y}
    {
    }
    constexpr vec2(vec2 const&) noexcept = default;
    template <typename U>
    constexpr explicit vec2(vec2<U> const& src) noexcept
        : x{component_type(src.x)}
        , y{component_type(src.y)}
    {
    }
    constexpr auto operator=(vec2 const& rhs) noexcept -> vec2& = default;
    template <typename U>
    constexpr auto operator=(vec2<U> const& rhs) noexcept -> vec2&
    {
        x = rhs.x;
        y = rhs.y;
        return *this;
    }
    constexpr auto operator-() const { return vec2{-x, -y}; }
    template <typename U> constexpr auto operator+=(vec2<U> const& pt) -> vec2&
    {
        x += pt.x;
        y += pt.y;
        return *this;
    }
    template <typename U> constexpr auto operator-=(vec2<U> const& pt) -> vec2&
    {
        x -= pt.x;
        y -= pt.y;
        return *this;
    }
    template <typename U> constexpr auto operator*=(U const& factor) -> vec2&
    {
        x *= factor;
        y *= factor;
        return *this;
    }
    template <typename U> constexpr auto operator/=(U const& factor) -> vec2&
    {
        x /= factor;
        y /= factor;
        return *this;
    }
    template <typename U>
    constexpr auto shifted(vec2<U> const& pt) const -> vec2
    {
        return vec2(component_type(x + pt.x), component_type(y + pt.y));
    }
    friend auto operator+(vec2 const& a, vec2 const& b)
    {
        return vec2{component_type(a.x + b.x), component_type(a.y + b.y)};
    }
    friend auto operator-(vec2 const& a, vec2 const& b)
    {
        return vec2{component_type(a.x - b.x), component_type(a.y - b.y)};
    }
    friend auto operator*(vec2 const& a, vec2 const& b)
    {
        return vec2{component_type(a.x * b.x),
            component_type(a.y * b.y)}; // per-component multiplication
    }
    template <typename U,
        typename std::enable_if_t<std::is_arithmetic<U>::value>* = nullptr>
    friend auto operator*(U const& a, vec2 const& b)
    {
        return vec2{component_type(a * b.x), component_type(a * b.y)};
    }
    template <typename U,
        typename std::enable_if_t<std::is_arithmetic<U>::value>* = nullptr>
    friend auto operator*(vec2 const& a, U const& b)
    {
        return vec2{component_type(a.x * b), component_type(a.y * b)};
    }
    template <typename U,
        typename std::enable_if_t<std::is_arithmetic<U>::value>* = nullptr>
    friend auto operator/(vec2 const& a, U const& b)
    {
        return vec2{component_type(a.x / b), component_type(a.y / b)};
    }

    [[nodiscard]] constexpr auto norm() const noexcept { return x * x + y * y; }
    [[nodiscard]] auto length() const { return std::sqrt(norm()); }

    [[nodiscard]] auto normalized() const { return *this * (1.0f / length()); }

    [[nodiscard]] constexpr auto clamp(
        vec2 const& lo, vec2 const& hi) const noexcept
    {
        return vec2{std::clamp(x, lo.x, hi.x), std::clamp(y, lo.y, hi.y)};
    }
    constexpr auto inside(vec2 const& lo, vec2 const& hi) const
    {
        return lo.x <= x && x < hi.x && lo.y <= y && y < hi.y;
    }
};

template <typename T, typename U>
constexpr auto operator==(vec2<T> const& lhs, vec2<U> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}
template <typename T, typename U>
constexpr auto operator!=(vec2<T> const& lhs, vec2<U> const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

template <typename T> struct rect {
    using vec2_type = vec2<T>;
    using component_type = typename vec2_type::component_type;

    vec2_type min;
    vec2_type max;

    constexpr rect() noexcept {}
    constexpr rect(component_type const& left, component_type const& top,
        component_type const& right, component_type const& bottom) noexcept
        : min{left, top}
        , max{right, bottom}
    {
    }
    constexpr rect(vec2_type const& tl, vec2_type const& br) noexcept
        : min{tl}
        , max{br}
    {
    }
    constexpr rect(rect const&) noexcept = default;
    constexpr auto operator=(rect const&) noexcept -> rect& = default;
    constexpr auto size() const -> vec2_type { return max - min; }
    constexpr auto width() const -> component_type { return max.x - min.x; }
    constexpr auto height() const -> component_type { return max.y - min.y; }
};

template <typename T, typename U>
constexpr auto operator==(rect<T> const& lhs, rect<U> const& rhs)
{
    return lhs.min == rhs.min && lhs.max == rhs.max;
}
template <typename T, typename U>
constexpr auto operator!=(rect<T> const& lhs, rect<U> const& rhs)
{
    return lhs.min != rhs.min || lhs.max != rhs.max;
}

template <typename T> struct vec4 {
    T x;
    T y;
    T z;
    T w;
};

} // namespace gtx