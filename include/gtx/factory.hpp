#pragma once

#if defined(GTX_DX11)
#include <d3d11.h>
#elif defined(GTX_VULKAN)
#include <vulkan/vulkan.h>
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
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkQueue graphics_queue;
    VkCommandPool command_pool;

    device_info() noexcept {}

    device_info(VkDevice device, VkPhysicalDevice physical_device,
        VkQueue graphics_queue, VkCommandPool command_pool)
        : device{device}
        , physical_device{physical_device}
        , graphics_queue{graphics_queue}
        , command_pool{command_pool}
    {
    }

    template <typename T>
    device_info(T const& other)
        : device{other.device}
        , physical_device{other.physical_device}
        , graphics_queue{other.graphics_queue}
        , command_pool{other.command_pool}
    {
    }
};

#else

#error Undefined GTX backend

#endif

void set_device(device_info const& info);
auto get_device() -> device_info const&;

} // namespace gtx