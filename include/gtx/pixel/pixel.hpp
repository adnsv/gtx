#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace gtx::pixel {

struct size {
    unsigned w;
    unsigned h;
};

struct coord {
    unsigned x;
    unsigned y;
};

struct nargb8888 {
    // assuming non-premultiplied values
    using value_type = uint32_t;

    union {
        value_type value;
        struct { // assuming little-endian arch
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
    };

    constexpr nargb8888() noexcept
        : value{}
    {
    }
    constexpr nargb8888(nargb8888 const&) noexcept = default;
    constexpr nargb8888(
        uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept
        : value{uint32_t(a << 24 | r << 16 | g << 8 | b)}
    {
    }
    explicit constexpr nargb8888(uint32_t v) noexcept
        : value{v}
    {
    }
    constexpr auto operator=(nargb8888 const& rhs) noexcept
        -> nargb8888& = default;

    constexpr operator value_type&() { return value; }
    constexpr operator value_type() const { return value; }
};

struct xrgb8888 {
    using value_type = uint32_t;

    union {
        value_type value;
        struct { // assuming little-endian arch
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t x;
        };
    };

    constexpr xrgb8888() noexcept
        : value{}
    {
    }
    constexpr xrgb8888(xrgb8888 const&) noexcept = default;
    constexpr xrgb8888(uint8_t r, uint8_t g, uint8_t b) noexcept
        : value{uint32_t(0xff000000 | r << 16 | g << 8 | b)}
    {
    }
    constexpr auto operator=(xrgb8888 const& rhs) noexcept
        -> xrgb8888& = default;

    constexpr operator value_type&() { return value; }
    constexpr operator value_type() const { return value; }
};

struct rgb565 {
    using value_type = uint16_t;
    value_type value;
    constexpr rgb565() noexcept
        : value{}
    {
    }
    constexpr rgb565(rgb565 const&) noexcept = default;
    constexpr rgb565(uint8_t r, uint8_t g, uint8_t b) noexcept
        : value{uint16_t(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))}
    {
    }
    constexpr rgb565(xrgb8888 const& v) noexcept
        : value{uint16_t(((v.r >> 3) << 11) | ((v.g >> 2) << 5) | (v.b >> 3))}
    {
    }
    constexpr auto operator=(rgb565 const& rhs) noexcept -> rgb565& = default;

    constexpr operator value_type&() { return value; }
    constexpr operator value_type() const { return value; }
};

struct l8 {
    using value_type = uint8_t;
    uint8_t value;
    constexpr l8() noexcept
        : value{}
    {
    }
    constexpr l8(uint8_t l) noexcept
        : value{l}
    {
    }
    constexpr l8(l8 const& l) noexcept = default;
    constexpr auto operator=(l8 const& rhs) noexcept -> l8& = default;
    constexpr operator value_type&() { return value; }
    constexpr operator value_type() const { return value; }
};

struct a8 {
    using value_type = uint8_t;
    uint8_t value;
    constexpr a8() noexcept
        : value{}
    {
    }
    constexpr a8(uint8_t a) noexcept
        : value{a}
    {
    }
    constexpr a8(a8 const& a) noexcept = default;
    constexpr auto operator=(a8 const& rhs) noexcept -> a8& = default;
    constexpr operator value_type&() { return value; }
    constexpr operator value_type() const { return value; }
};

} // namespace gtx::pixel