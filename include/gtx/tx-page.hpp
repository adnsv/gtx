#pragma once

#include <cstdint>
#include <functional>
#include <gtx/surface.hpp>
#include <memory>
#include <optional>
#include <vector>

namespace gtx::texture {

struct texel_box {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t w = 0;
    uint32_t h = 0;
};

struct texel_size {
    uint32_t w = 0;
    uint32_t h = 0;
    constexpr auto operator==(texel_size const&) const -> bool = default;
};

struct uv {
    float u = 0;
    float v = 0;
};

// uv_mapping maps texel coord to uv coordinate
struct uv_mapping {
    float scale_x_ = 1.0f;
    float scale_y_ = 1.0f;
    float offset_x_ = 0;
    float offset_y_ = 0;
    constexpr uv_mapping() noexcept = default;
    constexpr uv_mapping(uv_mapping const&) noexcept = default;
    constexpr uv_mapping(texel_size const& size, float texel_offset_x,
        float texel_offset_y) noexcept
        : scale_x_{size.w > 0 ? 1.0f / size.w : 0}
        , scale_y_{size.h > 0 ? 1.0f / size.h : 0}
        , offset_x_{texel_offset_x * scale_x_}
        , offset_y_{texel_offset_y * scale_y_}
    {
    }
    constexpr uv_mapping(texel_size const& size)
        : uv_mapping(size, 0, 0)
    {
    }
    constexpr auto operator()(float texel_x, float texel_y) const -> uv
    {
        // texel_x, texel_y are relative to box_
        return {
            texel_x * scale_x_ + offset_x_,
            texel_y * scale_y_ + offset_y_,
        };
    }
};

struct page_data;
struct sprite;

struct page {
    page() noexcept {}
    page(texel_size const& sz) { setup(sz); }
    page(uint32_t w, uint32_t h) { setup({w, h}); }
    page(page const&) = delete;
    page(page&&) = default;

    auto operator=(page&& other) -> page&
    {
        pd_ = std::move(other.pd_);
        return *this;
    }

    operator bool() const { return !pd_.expired(); }

    auto valid() const -> bool { return !pd_.expired(); }
    void release() { pd_.reset(); }
    static void release_all();

    void setup(texel_size const& sz, bool wrap = false);
    auto update(texel_box const& box, uint32_t const* data,
        size_t data_stride_bytes) -> bool;

    auto update(surface<uint32_t> const& surf) -> bool
    {
        return update(
            texel_box{0, 0, uint32_t(surf.width()), uint32_t(surf.height())},
            surf.data(), surf.stride());
    }

    auto native_handle() const -> void*;
    auto get_size() const -> texel_size;

    auto as_sprite() const -> sprite;

private:
    friend sprite;
    std::weak_ptr<page_data> pd_;

    page(std::weak_ptr<page_data> pp)
        : pd_{pp}
    {
    }
};

struct sprite {
    sprite()
        : pd_{}
        , box_{0, 0, 0, 0}
    {
    }
    sprite(page const& p, texture::texel_box const& b)
        : pd_{p.pd_}
        , box_{b}
    {
    }

    auto native_handle() const -> void*;
    auto get_page() const -> texture::page;
    auto get_box() const -> texture::texel_box const&;
    auto get_size() const -> texture::texel_size;
    auto uv_mapping() const -> texture::uv_mapping;

private:
    std::weak_ptr<page_data> pd_;
    texture::texel_box box_; // texel units
};

inline auto page::as_sprite() const -> sprite
{
    auto sz = get_size();
    return sprite{*this, {0, 0, sz.w, sz.h}};
}

inline auto sprite::get_page() const -> texture::page
{
    return texture::page{pd_};
}

inline auto sprite::get_box() const -> texture::texel_box const&
{
    return box_;
}

inline auto sprite::get_size() const -> texture::texel_size
{
    return {box_.w, box_.h};
}

inline auto sprite::uv_mapping() const -> texture::uv_mapping
{
    return texture::uv_mapping{
        get_page().get_size(), float(box_.x), float(box_.y)};
}

} // namespace gtx::texture