#pragma once

#ifdef GTX_DX11
#include <d3d11.h>
#endif

namespace gtx {

#if defined(GTX_DX11)
struct device_info {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    device_info() noexcept {}

    device_info(ID3D11Device* device, ID3D11DeviceContext* context) noexcept
        : device{device}
        , context{context}
    {
    }

    template <typename T>
    device_info(T const& other)
        : device{other.device}
        , context{other.context}
    {
    }
};
#else
struct device_info {
    int dummy = 0;

    device_info() noexcept {}

    template <typename T>
    device_info(T const& other)
        : dummy{other.dummy}
    {
    }
};
#endif

void set_device(device_info const& info);
auto get_device() -> device_info const&;

} // namespace texture