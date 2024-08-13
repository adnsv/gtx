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

struct frame_info {
    frame_info() noexcept {}

    template <typename T> frame_info(T const& other) noexcept {}
};

auto get_device() -> device_info const&;

#elif defined(GTX_OPENGL)

struct device_info {
    device_info() noexcept {}

    template <typename T> device_info(T const& other) noexcept {}
};

struct frame_info {
    frame_info() noexcept {}
    template <typename T> frame_info(T const& other) noexcept {}
};

#elif defined(GTX_VULKAN)

struct device_info {
    VkAllocationCallbacks const* allocator = nullptr;
    VkDevice device = nullptr;
    VkPhysicalDevice physical_device = nullptr;
    VkQueue graphics_queue = nullptr;
    VkPipelineCache pipeline_cache = nullptr;
    VkDescriptorPool descriptor_pool = nullptr;
    VkRenderPass render_pass = nullptr;

    device_info() noexcept {}

    device_info(device_info const&) noexcept = default;

    template <typename T>
    device_info(T const& other) noexcept
        : allocator{other.allocator}
        , device{other.device}
        , physical_device{other.physical_device}
        , graphics_queue{other.graphics_queue}
        , pipeline_cache{other.pipeline_cache}
        , descriptor_pool{other.descriptor_pool}
        , render_pass{other.render_pass}
    {
    }
};

struct frame_info {
    VkCommandPool command_pool = nullptr;
    VkCommandBuffer command_buffer = nullptr;

    frame_info() noexcept {}

    template <typename T>
    frame_info(T const& other) noexcept
        : command_pool{other.command_pool}
        , command_buffer{other.command_buffer}
    {
    }
};

#else

#error Undefined GTX backend

#endif

void set_device(device_info const&);
void set_frame(frame_info const&);

} // namespace gtx