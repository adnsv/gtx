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
    VkDescriptorSet descriptor_set;

    ~page_data();

private:
    factory* factory_;
    void release();
    friend struct factory;
};

struct factory {
    static auto get() -> factory*
    {
        static auto instance = factory{};
        return &instance;
    }

    ~factory();

private:
    std::vector<std::shared_ptr<texture::page_data>> pages;

    auto new_page(const texture::texel_size& sz,
        bool wrap) -> std::shared_ptr<texture::page_data>;

    device_info info_;
    VkSampler border_sampler_;
    VkSampler repeat_sampler_;

    void set_device(device_info const&);

    friend struct texture::page;
    friend struct texture::page_data;
    friend void set_device(device_info const&);
    friend auto get_device() -> device_info const&;
};

void set_device(device_info const& info) { factory::get()->set_device(info); }
auto get_device() -> device_info const& { return factory::get()->info_; }

auto create_sampler(device_info const& d, bool wrap) -> VkSampler
{
    auto sam = wrap ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                    : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    auto si = VkSamplerCreateInfo{};
    si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter = VK_FILTER_LINEAR;
    si.minFilter = VK_FILTER_LINEAR;
    si.addressModeU = sam;
    si.addressModeV = sam;
    si.addressModeW = sam;
    si.anisotropyEnable = VK_TRUE;
    si.maxAnisotropy = 16.0f;
    si.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    si.unnormalizedCoordinates = VK_FALSE;
    si.compareEnable = VK_FALSE;
    si.compareOp = VK_COMPARE_OP_ALWAYS;
    si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.mipLodBias = 0.0f;
    si.minLod = 0.0f;
    si.maxLod = 0.0f;

    VkSampler sampler;

    if (vkCreateSampler(d.device, &si, d.allocator, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");

    return sampler;
}

void release(device_info const& d, VkImage& v)
{
    if (v) {
        vkDestroyImage(d.device, v, d.allocator);
        v = nullptr;
    }
}

void release(device_info const& d, VkImageView& v)
{
    if (v) {
        vkDestroyImageView(d.device, v, d.allocator);
        v = nullptr;
    }
}

void release(device_info const& d, VkDeviceMemory& v)
{
    if (v) {
        vkFreeMemory(d.device, v, d.allocator);
        v = nullptr;
    }
}

void release(device_info const& d, VkBuffer& v)
{
    if (v) {
        vkDestroyBuffer(d.device, v, d.allocator);
        v = nullptr;
    }
}

void release(device_info const& d, VkSampler& v)
{
    if (v) {
        vkDestroySampler(d.device, v, d.allocator);
        v = nullptr;
    }
}

void release(device_info const& d, VkDescriptorSet& v)
{
    if (v) {
        vkFreeDescriptorSets(d.device, d.descriptor_pool, 1, &v);
        v = nullptr;
    }
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

    throw std::runtime_error("Failed to find suitable memory type.");
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

    if (vkCreateBuffer(d.device, &buffer_info, d.allocator, &buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer.");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(d.device, buffer, &mem_requirements);

    auto allocate_info = VkMemoryAllocateInfo{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = mem_requirements.size;
    allocate_info.memoryTypeIndex =
        find_memory_type(d, mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(d.device, &allocate_info, d.allocator,
            &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory.");
    }

    vkBindBufferMemory(d.device, buffer, buffer_memory, 0);
}

void create_image(device_info const& d, uint32_t width, uint32_t height,
    VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage& image, VkImageView& view, VkDeviceMemory& memory,
    VkDescriptorSet& ds)
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(d.device, &image_info, d.allocator, &image) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create image.");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(d.device, image, &mem_requirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = mem_requirements.size;
    allocInfo.memoryTypeIndex =
        find_memory_type(d, mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(d.device, &allocInfo, d.allocator, &memory) !=
        VK_SUCCESS) {
        release(d, image);
        throw std::runtime_error("Failed to allocate image memory.");
    }

    vkBindImageMemory(d.device, image, memory, 0);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(d.device, &view_info, d.allocator, &view) !=
        VK_SUCCESS) {
        release(d, image);
        release(d, memory);
        throw std::runtime_error("Failed to create texture image view.");
    }

    auto dsa_info = VkDescriptorSetAllocateInfo{};
    dsa_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsa_info.descriptorPool = d.descriptor_pool;
    dsa_info.descriptorSetCount = 1;
    dsa_info.pSetLayouts = &d.descriptor_set_layout;
    if (vkAllocateDescriptorSets(d.device, &dsa_info, &ds) != VK_SUCCESS) {
        release(d, view);
        release(d, image);
        release(d, memory);
        throw std::runtime_error("Failed to allocate image descriptor set.");
    }
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

    release(d, staging_buffer);
    release(d, staging_buffer_memory);
}

texture::page_data::~page_data() { release(); }

void texture::page_data::release()
{
    if (!factory_)
        return;

    gtx::release(factory_->info_, descriptor_set);
    gtx::release(factory_->info_, view);
    gtx::release(factory_->info_, image);
    gtx::release(factory_->info_, memory);
    factory_ = nullptr;
}

factory::~factory()
{
    if (info_.device) {
        for (auto& p : pages)
            p->release();

        release(info_, border_sampler_);
        release(info_, repeat_sampler_);
    }
}

void factory::set_device(device_info const& d)
{
    if (info_.device == d.device)
        return;

    if (info_.device) {
        for (auto& p : pages)
            p->release();

        release(info_, border_sampler_);
        release(info_, repeat_sampler_);
    }

    info_ = d;
}

auto factory::new_page(texture::texel_size const& sz,
    bool wrap) -> std::shared_ptr<texture::page_data>
{
    if (!sz.w || !sz.h)
        return {};

    VkImage image = nullptr;
    VkDeviceMemory memory = nullptr;
    VkImageView view = nullptr;
    VkDescriptorSet descriptor_set = nullptr;
    VkSampler sampler_ptr = nullptr;

    try {

        create_image(info_, sz.w, sz.h, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, view, memory,
            descriptor_set);

        if (wrap) {
            if (!repeat_sampler_)
                repeat_sampler_ = create_sampler(info_, true);
            sampler_ptr = repeat_sampler_;
        }
        else {
            if (!border_sampler_)
                border_sampler_ = create_sampler(info_, false);
            sampler_ptr = border_sampler_;
        }
        auto pd = std::make_shared<texture::page_data>();
        pd->width = sz.w;
        pd->height = sz.h;
        pd->image = image;
        pd->memory = memory;
        pd->view = view;
        pd->sampler = sampler_ptr;
        pd->descriptor_set = descriptor_set;
        pd->factory_ = this;
        pages.push_back(pd);
        return pd;
    }
    catch (std::exception const& err) {
        release(info_, descriptor_set);
        release(info_, view);
        release(info_, image);
        release(info_, memory);
        std::printf("[vulkan] ERROR: %s\n", err.what());
        return {};
    }
}

} // namespace gtx