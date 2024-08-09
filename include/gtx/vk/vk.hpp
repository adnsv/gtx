#pragma once

#include <gtx/device.hpp>
#include <stdexcept>

#ifdef GTX_VULKAN_SHADERC
#include <shaderc/shaderc.hpp>
#endif

namespace gtx::vk {

struct sampler {
    sampler(bool wrap);
    sampler(sampler&&);
    sampler(sampler const&) = delete;
    ~sampler();
    operator VkSampler() { return vk_sampler_; }

private:
    VkSampler vk_sampler_ = nullptr;
};

struct buffer {
    buffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);
    buffer(buffer&&);
    buffer(buffer const&) = delete;
    ~buffer();

    operator VkBuffer() { return vk_buffer_; }
    operator VkDeviceMemory() { return vk_memory_; }

private:
    VkBuffer vk_buffer_ = nullptr;
    VkDeviceMemory vk_memory_ = nullptr;
};

struct image_info {
    image_info(uint32_t width, uint32_t height, VkFormat format,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    image_info(image_info&&);
    image_info(image_info const&) = delete;
    ~image_info();

    operator VkImage() { return vk_image_; }
    operator VkImageView() { return vk_view_; }

private:
    VkImage vk_image_ = nullptr;
    VkImageView vk_view_ = nullptr;
    VkDeviceMemory vk_memory_ = nullptr;
};

struct descriptor_set_layout {
    descriptor_set_layout();
    descriptor_set_layout(descriptor_set_layout&&);
    descriptor_set_layout(descriptor_set_layout const&) = delete;
    ~descriptor_set_layout();

    operator VkDescriptorSetLayout() { return vk_descriptor_set_layout_; }

private:
    VkDescriptorSetLayout vk_descriptor_set_layout_ = nullptr;
};

struct descriptor_set {
    descriptor_set();
    descriptor_set(descriptor_set&&);
    descriptor_set(descriptor_set const&) = delete;
    ~descriptor_set();

    void update(image_info& img, sampler& smp);

    operator VkDescriptorSet() { return vk_descriptor_set_; }

private:
    VkDescriptorSet vk_descriptor_set_ = nullptr;
};

struct shader {
#ifdef GTX_VULKAN_SHADERC
    shader(shaderc_shader_kind kind, char const* name, std::string_view code);
#endif
    shader(shader&&);
    shader(shader const&) = delete;
    ~shader();

    operator VkShaderModule() { return vk_module_; }

private:
    VkShaderModule vk_module_ = nullptr;
};

} // namespace gtx::vk