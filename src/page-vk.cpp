#include <gtx/factory.hpp>
#include <gtx/tx-page.hpp>
#include <stdexcept>

namespace gtx {

struct factory;

struct texture::page_data {
    uint32_t width;
    uint32_t height;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
};

struct factory {
    static auto get() -> factory*
    {
        static auto instance = factory{};
        return &instance;
    }

    ~factory();

private:
    device_info info_;
    VkSampler sampler_;

    void set_device(device_info const& info);

    friend void set_device(device_info const&);
    friend auto get_device() -> device_info const&;
};

void set_device(device_info const& info) { factory::get()->set_device(info); }
auto get_device() -> device_info const& { return factory::get()->info_; }

factory::~factory()
{
    if (info_.device && sampler_) {
        vkDestroySampler(info_.device, sampler_, nullptr);
        sampler_ = nullptr;
    }
}

auto create_sampler(device_info const& d) -> VkSampler
{
    auto si = VkSamplerCreateInfo{};
    si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter = VK_FILTER_LINEAR;
    si.minFilter = VK_FILTER_LINEAR;
    si.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.anisotropyEnable = VK_TRUE;
    si.maxAnisotropy = 16.0f;
    si.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    si.unnormalizedCoordinates = VK_FALSE;
    si.compareEnable = VK_FALSE;
    si.compareOp = VK_COMPARE_OP_ALWAYS;
    si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.mipLodBias = 0.0f;
    si.minLod = 0.0f;
    si.maxLod = 0.0f;

    VkSampler sampler;

    if (vkCreateSampler(d.device, &si, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");

    return sampler;
}

void factory::set_device(device_info const& info)
{
    if (info_.device == info.device)
        return;

    if (info_.device && sampler_) {
        vkDestroySampler(info_.device, sampler_, nullptr);
        sampler_ = nullptr;
    }

    info_ = info;

    if (info_.device)
        sampler_ = create_sampler(info_);
}

auto begin_single_time_commands(device_info const& d) -> VkCommandBuffer
{
    auto alloc_info = VkCommandBufferAllocateInfo{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = d.command_pool;
    alloc_info.commandBufferCount = 1;

    auto cb = VkCommandBuffer{};
    vkAllocateCommandBuffers(d.device, &alloc_info, &cb);

    auto begin_info = VkCommandBufferBeginInfo{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cb, &begin_info);

    return cb;
}

void end_single_time_commands(device_info const& d, VkCommandBuffer cb)
{
    vkEndCommandBuffer(cb);

    auto submit_info = VkSubmitInfo{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cb;

    vkQueueSubmit(d.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(d.graphics_queue);

    vkFreeCommandBuffers(d.device, d.command_pool, 1, &cb);
}

auto find_memory_type(device_info const& d, uint32_t typeFilter,
    VkMemoryPropertyFlags properties) -> uint32_t
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(d.physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void create_buffer(device_info const& d, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
    auto buffer_info = VkBufferCreateInfo{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(d.device, &buffer_info, nullptr, &buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(d.device, buffer, &mem_requirements);

    auto allocate_info = VkMemoryAllocateInfo{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = mem_requirements.size;
    allocate_info.memoryTypeIndex =
        find_memory_type(d, mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(d.device, &allocate_info, nullptr, &buffer_memory) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(d.device, buffer, buffer_memory, 0);
}

auto create_image(device_info const& d, uint32_t width, uint32_t height,
    VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkDeviceMemory& image_memory) -> VkImage
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage image;
    if (vkCreateImage(d.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(d.device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(
        d.physical_device, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(d.device, &allocInfo, nullptr, &image_memory) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(d.device, image, image_memory, 0);

    return image;
}

auto create_image_view(
    device_info const& d, VkImage image, VkFormat format) -> VkImageView
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(d.device, &viewInfo, nullptr, &imageView) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image view!");
    }

    return imageView;
}

void update_image_region(device_info const& d, VkImage image,
    texture::texel_box const& box, uint32_t const* data,
    std::size_t data_stride)
{
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    VkDeviceSize buffer_size = box.w * box.h * sizeof(uint32_t);
    create_buffer(d, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    char* dst;
    vkMapMemory(d.device, staging_buffer_memory, 0, buffer_size, 0,
        reinterpret_cast<void**>(&dst));
    for (uint32_t y = 0; y < box.h; ++y) {
        memcpy(dst, data, box.w * sizeof(uint32_t));
        dst += box.w * sizeof(uint32_t);
        data += data_stride * sizeof(uint32_t);
    }
    vkUnmapMemory(d.device, staging_buffer_memory);

    auto command_buffer = begin_single_time_commands(d);

    auto barrier = VkImageMemoryBarrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    auto region = VkBufferImageCopy{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {
        static_cast<int32_t>(box.x), static_cast<int32_t>(box.y), 0};
    region.imageExtent = {box.w, box.h, 1};

    vkCmdCopyBufferToImage(command_buffer, staging_buffer, image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout back to shader read only optimal
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
        &barrier);

    end_single_time_commands(d, command_buffer);

    vkDestroyBuffer(d.device, staging_buffer, nullptr);
    vkFreeMemory(d.device, staging_buffer_memory, nullptr);
}

} // namespace gtx