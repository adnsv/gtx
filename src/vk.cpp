#include <gtx/device.hpp>
#include <gtx/tx-page.hpp>
#include <gtx/vk/vk.hpp>
#include <stdexcept>

namespace gtx {

struct texture::page_data {
    texel_size sz = {0, 0};
    bool wrap = false;
    vk::image_info image;
    vk::descriptor_set ds;
};

device_info d;
frame_info f;
std::unique_ptr<vk::descriptor_set_layout> ds_layout;

std::vector<std::shared_ptr<texture::page_data>> pages;

std::shared_ptr<vk::sampler> border_sampler;
std::shared_ptr<vk::sampler> repeat_sampler;

auto get_device() -> device_info const& { return d; }

void set_device(device_info const& v)
{
    pages.clear();
    border_sampler.reset();
    repeat_sampler.reset();
    ds_layout.reset();

    d = v;
}

void set_frame(frame_info const& v) { f = v; }

auto find_memory_type(
    uint32_t type_filter, VkMemoryPropertyFlags properties) -> uint32_t
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(d.physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

vk::sampler::sampler(bool wrap)
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

    if (vkCreateSampler(d.device, &si, d.allocator, &vk_sampler_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");
}

vk::sampler::~sampler()
{
    vkDestroySampler(d.device, vk_sampler_, d.allocator);
}

vk::buffer::buffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    auto buffer_info = VkBufferCreateInfo{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(d.device, &buffer_info, d.allocator, &vk_buffer_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer.");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(d.device, vk_buffer_, &mem_requirements);

    auto allocate_info = VkMemoryAllocateInfo{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = mem_requirements.size;
    allocate_info.memoryTypeIndex =
        find_memory_type(mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(d.device, &allocate_info, d.allocator, &vk_memory_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory.");
    }

    vkBindBufferMemory(d.device, vk_buffer_, vk_memory_, 0);
}

vk::buffer::~buffer()
{
    if (vk_buffer_)
        vkDestroyBuffer(d.device, vk_buffer_, d.allocator);
    if (vk_memory_)
        vkFreeMemory(d.device, vk_memory_, d.allocator);
}

vk::image_info::image_info(uint32_t width, uint32_t height, VkFormat format,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    {
        auto create_info = VkImageCreateInfo{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.extent.width = width;
        create_info.extent.height = height;
        create_info.extent.depth = 1;
        create_info.mipLevels = 1;
        create_info.arrayLayers = 1;
        create_info.format = format;
        create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        create_info.usage = usage;
        create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(d.device, &create_info, d.allocator, &vk_image_) !=
            VK_SUCCESS)
            throw std::runtime_error("Failed to create image.");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(d.device, vk_image_, &mem_requirements);

    auto alloc_info = VkMemoryAllocateInfo{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
        find_memory_type(mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(d.device, &alloc_info, d.allocator, &vk_memory_) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to allocate image memory.");

    vkBindImageMemory(d.device, vk_image_, vk_memory_, 0);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = vk_image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(d.device, &view_info, d.allocator, &vk_view_) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image view.");
}

vk::image_info::~image_info()
{
    if (vk_view_)
        vkDestroyImageView(d.device, vk_view_, d.allocator);
    if (vk_memory_)
        vkFreeMemory(d.device, vk_memory_, d.allocator);
    if (vk_image_)
        vkDestroyImage(d.device, vk_image_, d.allocator);
}

vk::descriptor_set_layout::descriptor_set_layout()
{
    auto const layout_binding = VkDescriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
    auto const create_info = VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .bindingCount = 1,
        .pBindings = &layout_binding,
    };
    if (vkCreateDescriptorSetLayout(d.device, &create_info, d.allocator,
            &vk_descriptor_set_layout_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout.");
}

vk::descriptor_set_layout::~descriptor_set_layout()
{
    if (vk_descriptor_set_layout_)
        vkDestroyDescriptorSetLayout(
            d.device, vk_descriptor_set_layout_, d.allocator);
}

vk::descriptor_set::descriptor_set()
{
    if (!ds_layout)
        ds_layout = std::make_unique<vk::descriptor_set_layout>();

    const auto dsl = VkDescriptorSetLayout(*ds_layout);

    auto dsa_info = VkDescriptorSetAllocateInfo{};
    dsa_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsa_info.descriptorPool = d.descriptor_pool;
    dsa_info.descriptorSetCount = 1;
    dsa_info.pSetLayouts = &dsl;
    if (auto err =
            vkAllocateDescriptorSets(d.device, &dsa_info, &vk_descriptor_set_);
        err != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate image descriptor set.");
}

vk::descriptor_set::~descriptor_set()
{
    vkFreeDescriptorSets(d.device, d.descriptor_pool, 1, &vk_descriptor_set_);
}

void vk::descriptor_set::update(image_info& img, sampler& smp)
{
    auto info = VkDescriptorImageInfo{};
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.imageView = VkImageView(img);
    info.sampler = smp;

    auto writes = VkWriteDescriptorSet{};
    writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes.dstSet = vk_descriptor_set_;
    writes.dstBinding = 0;
    writes.dstArrayElement = 0;
    writes.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes.descriptorCount = 1;
    writes.pImageInfo = &info;
    vkUpdateDescriptorSets(d.device, 1, &writes, 0, nullptr);
}

static auto begin_single_time_commands(
    VkCommandPool command_pool) -> VkCommandBuffer
{
    auto alloc_info = VkCommandBufferAllocateInfo{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    auto cb = VkCommandBuffer{};
    vkAllocateCommandBuffers(d.device, &alloc_info, &cb);

    auto begin_info = VkCommandBufferBeginInfo{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cb, &begin_info);

    return cb;
}

static void end_single_time_commands(
    VkCommandPool command_pool, VkCommandBuffer cb)
{
    vkEndCommandBuffer(cb);

    auto submit_info = VkSubmitInfo{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cb;

    vkQueueSubmit(d.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(d.graphics_queue);

    vkFreeCommandBuffers(d.device, command_pool, 1, &cb);
}

static void update_image_region(VkCommandPool command_pool, VkImage image,
    uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t const* data,
    std::size_t data_stride)
{
    auto const buffer_size = VkDeviceSize(w * h * sizeof(uint32_t));

    auto staging_buffer =
        vk::buffer{buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    char* dst;
    vkMapMemory(d.device, staging_buffer, 0, buffer_size, 0,
        reinterpret_cast<void**>(&dst));
    for (uint32_t y = 0; y < h; ++y) {
        memcpy(dst, data, w * sizeof(uint32_t));
        dst += w * sizeof(uint32_t);
        data += data_stride * sizeof(uint32_t);
    }
    vkUnmapMemory(d.device, staging_buffer);

    auto command_buffer = begin_single_time_commands(command_pool);

    auto copy_barrier = VkImageMemoryBarrier{};
    copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; 
    copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier.image = image;
    copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_barrier.subresourceRange.levelCount = 1;
    copy_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copy_barrier);

    auto region = VkBufferImageCopy{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
    region.imageExtent = {w, h, 1};

    vkCmdCopyBufferToImage(command_buffer, staging_buffer, image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout back to shader read only optimal
    auto use_barrier = VkImageMemoryBarrier{};
    use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier.image = image;
    use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    use_barrier.subresourceRange.levelCount = 1;
    use_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
        &use_barrier);

    end_single_time_commands(command_pool, command_buffer);
}

static auto new_page(texture::texel_size const& sz,
    bool wrap) -> std::shared_ptr<texture::page_data>
{
    if (!sz.w || !sz.h)
        return {};

    auto info = vk::image_info{sz.w, sz.h, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    auto ds = vk::descriptor_set{};

    if (wrap) {
        if (!repeat_sampler)
            repeat_sampler = std::make_shared<vk::sampler>(true);
        ds.update(info, *repeat_sampler);
    }
    else {
        if (!border_sampler)
            border_sampler = std::make_shared<vk::sampler>(false);
        ds.update(info, *border_sampler);
    }

    auto p = std::make_shared<texture::page_data>(
        sz, wrap, std::move(info), std::move(ds));
    pages.push_back(p);
    return p;
}

auto texture::page::update(texture::texel_box const& box, uint32_t const* data,
    size_t data_stride) -> bool
{
    if (!data || data_stride < size_t(box.w) || !f.command_pool)
        return false;

    if (auto pp = pd_.lock()) {
        auto& pd = *pp;
        if (box.x + box.w > pd.sz.w || box.y + box.h > pd.sz.h)
            return false;

        update_image_region(f.command_pool, VkImage(pd.image), box.x, box.y,
            box.w, box.h, data, data_stride);
        return true;
    }
    return false;
}

auto texture::page::native_handle() const -> void*
{
    if (auto pp = pd_.lock()) {
        auto& ds = pp->ds;
        return VkDescriptorSet(pp->ds);
    }
    return nullptr;
}

auto texture::page::get_size() const -> texture::texel_size
{
    if (auto pp = pd_.lock())
        return pp->sz;
    return {0, 0};
}

void texture::page::setup(texture::texel_size const& sz, bool wrap)
{
    if (auto pp = pd_.lock()) {
        if (pp->sz == sz && pp->wrap == wrap)
            return;
    }
    pd_ = new_page(sz, wrap);
}

void texture::page::release_all() { pages.clear(); }

auto texture::sprite::native_handle() const -> void*
{
    if (auto pp = pd_.lock())
        return VkDescriptorSet(pp->ds);
    return nullptr;
}

} // namespace gtx