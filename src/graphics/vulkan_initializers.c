internal VkRect2D vulkan_rect2d(S32 x, S32 y, U32 width, U32 height) {
    VkRect2D result = { 0 };

    result.offset.x      = x;
    result.offset.y      = y;
    result.extent.width  = width;
    result.extent.height = height;

    return result;
}
internal VkPipelineShaderStageCreateInfo vulkan_shader_stage_create_info(VkShaderStageFlagBits shader_stage, VkShaderModule shader_module) {
    VkPipelineShaderStageCreateInfo info = { 0 };
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    info.stage  = shader_stage;
    info.module = shader_module;
    info.pName  = "main";

    return info;
}

internal VkPipelineInputAssemblyStateCreateInfo vulkan_input_assembly_state_create_info(VkPrimitiveTopology topology) {
    VkPipelineInputAssemblyStateCreateInfo info = { 0 };
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    info.topology               = topology;
    info.primitiveRestartEnable = VK_FALSE;

    return info;
}

internal VkPipelineRasterizationStateCreateInfo vulkan_rasterization_state_create_info(VkPolygonMode polygon_mode) {
    VkPipelineRasterizationStateCreateInfo info = { 0 };
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    info.depthClampEnable        = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;

    info.polygonMode = polygon_mode;
    info.lineWidth   = 1.0f;

    info.cullMode  = VK_CULL_MODE_BACK_BIT;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;

    info.depthBiasEnable = VK_FALSE;

    return info;
}

internal VkPipelineMultisampleStateCreateInfo vulkan_multisample_state_create_info(Void) {
    VkPipelineMultisampleStateCreateInfo info = { 0 };
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    info.sampleShadingEnable   = VK_FALSE;
    info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    info.minSampleShading      = 1.0f;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable      = VK_FALSE;

    return info;
}

internal VkPipelineColorBlendAttachmentState vulkan_color_blend_attachment_state(Void) {
    VkPipelineColorBlendAttachmentState color_blend_attachment = { 0 };

    color_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable         = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    return color_blend_attachment;
}

internal VkVertexInputAttributeDescription vulkan_vertex_attribute_description(U32 binding, U32 location, VkFormat format, U32 offset) {
    VkVertexInputAttributeDescription attribute = { 0 };

    attribute.binding  = binding;
    attribute.location = location;
    attribute.format   = format;
    attribute.offset   = offset;

    return attribute;
}
