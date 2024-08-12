#include "spirv-polyline.hpp"
#include <gtx/shader/polyline.hpp>
#include <memory>

namespace gtx {
extern device_info d;
extern frame_info f;
} // namespace gtx

namespace gtx::shdr {

polyline::polyline()
//    : uniform_buffer_{sizeof(mat4x4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT}
{
    if (!d.device)
        throw std::runtime_error("polyline pipeline: missing device.");

#if 0
    { // Uniforms with UBO buffers

        // DescriptorSetLayout
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(d.device, &layoutInfo, d.allocator,
                &descriptor_set_layout_) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");

        // DescriptorPool
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(d.device, &poolInfo, d.allocator,
                &descriptor_pool_) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");

        // Allocate Descriptor Set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptor_pool_;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptor_set_layout_;

        if (vkAllocateDescriptorSets(d.device, &allocInfo, &descriptor_set_) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniform_buffer_;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(mat4x4);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptor_set_;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(d.device, 1, &descriptorWrite, 0, nullptr);
    }
#endif

    auto vert = vk::shader{polyline_vert};
    auto geom = vk::shader{polyline_geom};
    auto frag = vk::shader{polyline_frag};

    VkPipelineShaderStageCreateInfo stages[3] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    stages[1].module = geom;
    stages[1].pName = "main";
    stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[2].module = frag;
    stages[2].pName = "main";

    // binding description
    VkVertexInputBindingDescription bd = {};
    bd.stride = sizeof(vertex);
    bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bd.binding = 0;

    // attributes to match fields in vertex
    VkVertexInputAttributeDescription attrs[3] = {};
    attrs[0].location = 0;
    attrs[0].binding = bd.binding;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = offsetof(polyline::vertex, pos);
    attrs[1].location = 1;
    attrs[1].binding = bd.binding;
    attrs[1].format = VK_FORMAT_R32_SFLOAT;
    attrs[1].offset = offsetof(polyline::vertex, thk);
    attrs[2].location = 2;
    attrs[2].binding = bd.binding;
    attrs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[2].offset = offsetof(polyline::vertex, clr);

    VkPipelineVertexInputStateCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bd;
    vi.vertexAttributeDescriptionCount = 3;
    vi.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia_info = {};
    ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster_info = {};
    raster_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_info.cullMode = VK_CULL_MODE_NONE;
    raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms_info = {};
    ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cba = {};
    cba.blendEnable = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_info = {};
    depth_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &cba;

    VkDynamicState dynamic_states[2] = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPushConstantRange push_constant = {};
    push_constant.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(mat4x4);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    //    pipelineLayoutInfo.setLayoutCount = 1;
    //    pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(d.device, &pipelineLayoutInfo, d.allocator,
            &pipeline_layout_) != VK_SUCCESS) 
        throw std::runtime_error("failed to create pipeline layout.");
    

    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = 3;
    info.pStages = stages;
    info.pVertexInputState = &vi;
    info.pInputAssemblyState = &ia_info;
    info.pViewportState = &viewport_info;
    info.pRasterizationState = &raster_info;
    info.pMultisampleState = &ms_info;
    info.pDepthStencilState = &depth_info;
    info.pColorBlendState = &blend_info;
    info.pDynamicState = &dynamic_state;
    info.layout = pipeline_layout_;
    info.renderPass = renderPass;
    info.subpass = subpass;
}

polyline::~polyline()
{
    if (pipeline_)
        vkDestroyPipeline(d.device, pipeline_, d.allocator);
    if (pipeline_layout_)
        vkDestroyPipelineLayout(d.device, pipeline_layout_, d.allocator);

#if 0
    if (descriptor_pool_)
        vkDestroyDescriptorPool(d.device, descriptor_pool_, d.allocator);
    if (descriptor_set_layout_)
        vkDestroyDescriptorSetLayout(
            d.device, descriptor_set_layout_, d.allocator);
#endif
}

void polyline::setup_mvp(mat4x4 const& m)
{
    vkCmdPushConstants(f.command_buffer, pipeline_layout_, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4x4), &m);

#if 0
    void* data;
    vkMapMemory(d.device, uniform_buffer_, 0, sizeof(mat4x4), 0, &data);
    memcpy(data, &m, sizeof(mat4x4));
    vkUnmapMemory(d.device, uniform_buffer_);
#endif
}

void polyline::render(std::size_t segment_id) {}

} // namespace gtx::shdr