#pragma once

#if defined(GTX_DX11)
#include <d3d11.h>
#elif defined(GTX_OPENGL)
// using stub, nothing to include
#elif defined(GTX_VULKAN)
#include <vulkan/vulkan.h>
#else
#error Undefined GTX implementation
#endif

namespace gtx {

#if defined(GTX_DX11)

struct device_info {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    device_info() noexcept {}
    ~device_info();

    device_info(ID3D11Device* device, ID3D11DeviceContext* context) noexcept
        : device{device}
        , context{context}
    {
    }

    auto operator=(device_info const&) -> device_info&;

    template <typename T>
    device_info(T const& other)
        : device{other.device}
        , context{other.context}
    {
    }
};

auto get_device() -> device_info const&;

#elif defined(GTX_OPENGL)

struct device_info {
    int dummy = 0;

    device_info() noexcept {}

    template <typename T>
    device_info(T const& other)
        : dummy{other.dummy}
    {
    }
};

#elif defined(GTX_VULKAN)

struct device_info {
    VkAllocationCallbacks const* allocator = nullptr;
    VkDevice device = nullptr;
    VkPhysicalDevice physical_device = nullptr;
    VkQueue graphics_queue = nullptr;
    VkDescriptorPool descriptor_pool = nullptr;

    device_info() noexcept {}

    template <typename T>
    device_info(T const& other)
        : allocator{other.allocator}
        , device{other.device}
        , physical_device{physical_device}
        , graphics_queue{graphics_queue}
        , descriptor_pool{descriptor_pool}
    {
    }
};

#else

#error Undefined GTX backend

#endif

void set_device(device_info const&);

} // namespace gtx