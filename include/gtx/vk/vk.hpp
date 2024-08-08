#pragma once

#include <gtx/device.hpp>
#include <stdexcept>

namespace gtx::vk {

struct sampler {
    sampler(bool wrap);
    sampler(sampler&&) = default;
    sampler(sampler const&) = delete;
    ~sampler();
    operator VkSampler() { return vk_sampler_; }

private:
    VkSampler vk_sampler_ = nullptr;
};

struct buffer {
    buffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);
    buffer(buffer&&) = default;
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
    image_info(image_info&&) = default;
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
    descriptor_set_layout(descriptor_set_layout&&) = default;
    descriptor_set_layout(descriptor_set_layout const&) = delete;
    ~descriptor_set_layout();

    operator VkDescriptorSetLayout() { return vk_descriptor_set_layout_; }

private:
    VkDescriptorSetLayout vk_descriptor_set_layout_ = nullptr;
};

struct descriptor_set {
    descriptor_set();
    descriptor_set(descriptor_set&&) = default;
    descriptor_set(descriptor_set const&) = delete;
    ~descriptor_set();

    void update(image_info& img, sampler& smp);

    operator VkDescriptorSet() { return vk_descriptor_set_; }

private:
    VkDescriptorSet vk_descriptor_set_ = nullptr;
};

} // namespace gtx::vk