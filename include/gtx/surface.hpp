#pragma once

#include <cmath>

namespace gtx {

template <typename T> struct surface {
    using pixel_type = T;

    surface() noexcept {}

    surface(T* data, std::size_t w, std::size_t h, std::size_t stride) noexcept
        : data_{data}
        , width_{w}
        , height_{h}
        , stride_{stride}
    {
    }

    surface(T* data, std::size_t w, std::size_t h) noexcept
        : data_{data}
        , width_{w}
        , height_{h}
        , stride_{w}
    {
    }

    auto empty() const noexcept { return !data_ || !width_ || !height_; }
    auto width() const noexcept { return width_; }
    auto height() const noexcept { return height_; }
    auto stride() const noexcept { return stride_; }
    auto data() const noexcept { return data_; }
    auto data() noexcept { return data_; }
    operator bool() const noexcept { return !empty(); }

    // auto operator[](pixel::coord const& xy) -> pixel_type&
    //{
    //     return data_[xy.x + xy.y * stride_];
    // }
    //
    // auto operator[](pixel::coord const& xy) const -> pixel_type const&
    //{
    //    return data_[xy.x + xy.y * stride_];
    //}

    auto subsurface(std::size_t x, std::size_t y, std::size_t w,
        std::size_t h) const noexcept
    {
        return surface{data_ + x + y * stride_, x + w > width_ ? width_ - x : w,
            y + h > height_ ? height_ - y : h, stride_};
    }

private:
    T* data_ = nullptr;
    std::size_t width_ = 0;
    std::size_t height_ = 0;
    std::size_t stride_ = 0;
};

} // namespace gtx