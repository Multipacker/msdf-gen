#include "vulkan_initializers.c"
#include "vulkan_allocator.c"
#include "conversion_types.h"

#if defined(VK_USE_PLATFORM_XCB_KHR)
    global const char *vulkan_instance_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
    };
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    global const char *vulkan_instance_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
    };
#else
# error no integration support for this windowing system.
#endif

global const char *vulkan_device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef DEBUG_BUILD
    global const char *vulkan_instance_layers[] = {
        "VK_LAYER_KHRONOS_validation",
        0,
    };
    global const char *vulkan_device_layers[] = {
        "VK_LAYER_KHRONOS_validation",
        0,
    };
#else
    global const char *vulkan_instance_layers[] = {
        0,
    };
    global const char *vulkan_device_layers[] = {
        0,
    };
#endif

typedef struct {
    B32 has_graphics_family;
    U32 graphics_family;
    B32 has_present_family;
    U32 present_family;
} Vulkan_QueueFamilyIndicies;

typedef struct {
    U32                                    shader_stage_count;
    VkPipelineShaderStageCreateInfo       *shader_stages;
    VkVertexInputBindingDescription        vertex_binding_description;
    U32                                    vertex_attribute_description_count;
    VkVertexInputAttributeDescription     *vertex_attribute_descriptions;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
    VkViewport viewport;
    B32 dynamic_scissor;
    VkRect2D   scissor;
    VkPipelineRasterizationStateCreateInfo rasterization_state;
    VkPipelineMultisampleStateCreateInfo   multisample_state;
    VkPipelineDepthStencilStateCreateInfo  depth_stencil_state;
    VkPipelineColorBlendAttachmentState    color_blend_attachment;
    VkPipelineLayout                       pipeline_layout;
} Vulkan_GraphicsPipelineBuilder;

internal VkRect2D vulkan_rect2d_intersection(VkRect2D a, VkRect2D b) {
    VkRect2D result = { 0 };

    result.offset.x = s32_max(a.offset.x, b.offset.x);
    result.offset.y = s32_max(a.offset.y, b.offset.y);

    result.extent.width  = u32_min(a.offset.x + a.extent.width,  b.offset.x + b.extent.width)  - result.offset.x;
    result.extent.height = u32_min(a.offset.y + a.extent.height, b.offset.y + b.extent.height) - result.offset.y;

    return result;
}

internal Vulkan_QueueFamilyIndicies vulkan_find_queue_families(Arena *arena, VkPhysicalDevice device, VkSurfaceKHR surface) {
    Vulkan_QueueFamilyIndicies result = { 0 };

    // TODO: vkGetPhysicalDeviceQueueFamilyProperties2, vkGetPhysicalDeviceQueueFamilyProperties2KHR?
    // TODO: Error checks?
    U32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
    VkQueueFamilyProperties *queue_families = arena_push_array(arena, VkQueueFamilyProperties, queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    for (U32 i = 0; i < queue_family_count; ++i) {
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            result.has_graphics_family = true;
            result.graphics_family = i;
        }

        if (present_support) {
            result.has_present_family = true;
            result.present_family = i;
        }
    }

    return result;
}

// TODO: Better scoring of physical devices.
internal U32 vulkan_score_physical_device(Arena *scratch, VkPhysicalDevice device, VkSurfaceKHR surface, U32 required_extension_count, const char **required_extensions) {
    U32 score = 0;

    // TODO: vkGetPhysicalDeviceProperties2, vkGetPhysicalDeviceProperties2KHR?
    // TODO: Error checks?
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    U32 scores[] = {
        [VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU]   = 5,
        [VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU]    = 4,
        [VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU] = 3,
        [VK_PHYSICAL_DEVICE_TYPE_CPU]            = 2,
    };

    if (properties.deviceType < array_count(scores)) {
        score += scores[properties.deviceType];
    }

    Vulkan_QueueFamilyIndicies queue_indicies = vulkan_find_queue_families(scratch, device, surface);
    if (!queue_indicies.has_graphics_family || !queue_indicies.has_present_family) {
        score = 0;
    }

    if (score != 0) {
        U32 extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, 0, &extension_count, 0);
        VkExtensionProperties *available_extensions = arena_push_array(scratch, VkExtensionProperties, extension_count);
        vkEnumerateDeviceExtensionProperties(device, 0, &extension_count, available_extensions);

        for (U32 i = 0; i < required_extension_count; ++i) {
            Str8 wanted_name = str8_cstr((CStr) required_extensions[i]);
            B32 found = false;
            for (U32 j = 0; j < extension_count && !found; ++j) {
                Str8 name = str8_cstr(available_extensions[j].extensionName);
                if (str8_equal(wanted_name, name)) {
                    found = true;
                }
            }

            if (!found) {
                score = 0;
            }
        }
    }

    if (score != 0) {
        U32 format_count = 0;
        VULKAN_RESULT_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, 0));

        U32 present_mode_count = 0;
        VULKAN_RESULT_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, 0));

        if (format_count == 0 || present_mode_count == 0) {
            score = 0;
        }
    }

    return score;
}

internal Void vulkan_swapchain_create_or_recreate(Arena *arena, Vulkan_Swapchain *swapchain, U32 *window_width, U32 *window_height) {
    VkSwapchainKHR old_swapchain = swapchain->swapchain;
    if (swapchain->image_views) {
        for (U32 i = 0; i < swapchain->image_count; ++i) {
            vkDestroyImageView(swapchain->device, swapchain->image_views[i], 0);
        }
    }

    Arena_Temporary scratch = arena_get_scratch(0, 0);

    // TODO: vkGetPhysicalDeviceSurfaceCapabilities2KHR?
    VkSurfaceCapabilitiesKHR capabilities;
    VULKAN_RESULT_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(swapchain->physical_device, swapchain->surface, &capabilities));

    // TODO: Error, vkGetPhysicalDeviceSurfaceFormats2KHR?
    U32                 format_count = 0;
    VkSurfaceFormatKHR *formats      = 0;
    VULKAN_RESULT_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(swapchain->physical_device, swapchain->surface, &format_count, 0));
    formats = arena_push_array(scratch.arena, VkSurfaceFormatKHR, format_count);
    VULKAN_RESULT_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(swapchain->physical_device, swapchain->surface, &format_count, formats));

    // TODO: vkGetPhysicalDeviceSurfacePresentModes2EXT?
    U32               present_mode_count = 0;
    VkPresentModeKHR *present_modes      = 0;
    VULKAN_RESULT_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(swapchain->physical_device, swapchain->surface, &present_mode_count, 0));
    present_modes = arena_push_array(scratch.arena, VkPresentModeKHR, present_mode_count);
    VULKAN_RESULT_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(swapchain->physical_device, swapchain->surface, &present_mode_count, present_modes));

    VkExtent2D extent;
    if (capabilities.currentExtent.width == U32_MAX && capabilities.currentExtent.height == U32_MAX) {
        extent.width  = *window_width;
        extent.height = *window_height;
    } else {
        extent = capabilities.currentExtent;
        *window_width  = extent.width;
        *window_height = extent.height;
    }

    // NOTE: This is the only present mode that is guaranteed to be available.
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (U32 i = 0; i < present_mode_count; ++i) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    U32 desired_image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount != 0) {
        desired_image_count = u32_min(desired_image_count, capabilities.maxImageCount);
    }

    VkSurfaceTransformFlagsKHR pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    if (!(capabilities.supportedTransforms & pre_transform)) {
        pre_transform = capabilities.currentTransform;
    }

    // NOTE: The spec guarantees that at least one composite alpha blending mode is supported, but not which one.
    // TODO: Handle these cases properly throughout the rest of the renderer.
    VkCompositeAlphaFlagsKHR composite_alpha = 0;
    VkCompositeAlphaFlagsKHR composite_alpha_flags[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR, // TODO: If this mode is selected, do we want to change the platforms composite alpha blending mode?
    };
    for (U32 i = 0; i < array_count(composite_alpha_flags); ++i) {
        if (capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
            composite_alpha = composite_alpha_flags[i];
            break;
        }
    }

    VkSurfaceFormatKHR surface_format = formats[0];
    for (U32 i = 0; i < format_count; ++i) {
        if (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
                surface_format = formats[i];
            }
        }
    }

    swapchain->image_format = surface_format.format;
    swapchain->color_space  = surface_format.colorSpace;
    swapchain->extent       = extent;

    VkSwapchainCreateInfoKHR create_info = { 0 };
    create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface          = swapchain->surface;
    create_info.minImageCount    = desired_image_count;
    create_info.imageFormat      = surface_format.format;
    create_info.imageColorSpace  = surface_format.colorSpace;
    create_info.imageExtent      = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform     = pre_transform;
    create_info.compositeAlpha   = composite_alpha;
    create_info.presentMode      = present_mode;
    create_info.oldSwapchain     = old_swapchain;
    create_info.clipped          = VK_TRUE;

    VULKAN_RESULT_CHECK(vkCreateSwapchainKHR(swapchain->device, &create_info, 0, &swapchain->swapchain));

    if (old_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(swapchain->device, old_swapchain, 0);
    }

    VULKAN_RESULT_CHECK(vkGetSwapchainImagesKHR(swapchain->device, swapchain->swapchain, &swapchain->image_count, 0));
    VkImage *images = arena_push_array(scratch.arena, VkImage, swapchain->image_count);
    VULKAN_RESULT_CHECK(vkGetSwapchainImagesKHR(swapchain->device, swapchain->swapchain, &swapchain->image_count, images));

    swapchain->image_views = arena_push_array(arena, VkImageView, swapchain->image_count);
    for (U32 i = 0; i < swapchain->image_count; ++i) {
        VkImageViewCreateInfo create_info = { 0 };
        create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image    = images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format   = swapchain->image_format;

        create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel   = 0;
        create_info.subresourceRange.levelCount     = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount     = 1;

        VULKAN_RESULT_CHECK(vkCreateImageView(swapchain->device, &create_info, 0, &swapchain->image_views[i]));
    }

    arena_end_temporary(scratch);
}

internal VkShaderModule vulkan_create_shader_module(VkDevice device, Str8 code) {
    VkShaderModuleCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size;
    create_info.pCode = (U32 *) code.data;

    VkShaderModule module;
    VULKAN_RESULT_CHECK(vkCreateShaderModule(device, &create_info, 0, &module));

    return module;
}

internal VkResult vulkan_swapchain_acquire_next_image(Vulkan_Swapchain *swapchain, VkSemaphore image_available) {
    return vkAcquireNextImageKHR(swapchain->device, swapchain->swapchain, U64_MAX, image_available, VK_NULL_HANDLE, &swapchain->current_image_index);
}

internal VkResult vulkan_swapchain_queue_present(Vulkan_Swapchain *swapchain, VkQueue queue, VkSemaphore *wait_semaphore) {
    VkPresentInfoKHR present_info = { 0 };
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain->swapchain;
    present_info.pImageIndices      = &swapchain->current_image_index;

    if (wait_semaphore) {
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = wait_semaphore;
    }

    return vkQueuePresentKHR(queue, &present_info);
}

internal Void vulkan_swapchain_destroy(Vulkan_Swapchain *swapchain) {
    for (U32 i = 0; i < swapchain->image_count; ++i) {
        vkDestroyImageView(swapchain->device, swapchain->image_views[i], 0);
    }
    vkDestroySwapchainKHR(swapchain->device, swapchain->swapchain, 0);
}

internal U32 vulkan_determine_output_conversion(VkFormat image_format, VkColorSpaceKHR colorspace) {
    B32 format_is_linear;
    switch (image_format) {
        case VK_FORMAT_R8G8B8_UNORM:     case VK_FORMAT_R8G8B8_SNORM:     case VK_FORMAT_R8G8B8_USCALED:   case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:      case VK_FORMAT_R8G8B8_SINT:      case VK_FORMAT_B8G8R8_UNORM:     case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:   case VK_FORMAT_B8G8R8_SSCALED:   case VK_FORMAT_B8G8R8_UINT:      case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_R8G8B8A8_UNORM:   case VK_FORMAT_R8G8B8A8_SNORM:   case VK_FORMAT_R8G8B8A8_USCALED: case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:    case VK_FORMAT_R8G8B8A8_SINT:    case VK_FORMAT_B8G8R8A8_UNORM:   case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED: case VK_FORMAT_B8G8R8A8_SSCALED: case VK_FORMAT_B8G8R8A8_UINT:    case VK_FORMAT_B8G8R8A8_SINT: {
            format_is_linear = true;
        } break;
        case VK_FORMAT_R8G8B8_SRGB: case VK_FORMAT_B8G8R8_SRGB: case VK_FORMAT_R8G8B8A8_SRGB: case VK_FORMAT_B8G8R8A8_SRGB: {
            format_is_linear = false;
        } break;
        default: {
            os_console_print(str8_literal("WARNING(graphics/vulkan): Unable to determine surface format, colors may look wrong.\n"));
            format_is_linear = true;
        } break;
    }

    B32 colorspace_is_linear;
    switch (colorspace) {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: {
            colorspace_is_linear = false;
        } break;
        default: {
            os_console_print(str8_literal("WARNING(graphics/vulkan): Unable to determine surface colorspace, colors may look wrong.\n"));
            colorspace_is_linear = true;
        } break;
    }

    U32 conversion = VULKAN_CONVERSION_NONE;
    if (format_is_linear == colorspace_is_linear) {
        conversion = VULKAN_CONVERSION_NONE;
    } else if (format_is_linear && !colorspace_is_linear) {
        conversion = VULKAN_CONVERSION_SRGB;
    } else if (!format_is_linear && colorspace_is_linear) {
        conversion = VULKAN_CONVERSION_LINEAR;
    } else {
        assert(!"Well done, you broke the universe!");
    }

    return conversion;
}

internal VkPipeline vulkan_build_graphics_pipeline(Vulkan_GraphicsPipelineBuilder builder, VkDevice device, VkRenderPass render_pass, VkFormat image_format, VkColorSpaceKHR colorspace) {
    U32 output_conversion = vulkan_determine_output_conversion(image_format, colorspace);
    VkSpecializationMapEntry entry = { 0 };
    entry.constantID = 0;
    entry.offset     = 0;
    entry.size       = sizeof(output_conversion);

    VkSpecializationInfo specialization_info = { 0 };
    specialization_info.mapEntryCount        = 1;
    specialization_info.pMapEntries          = &entry;
    specialization_info.dataSize             = sizeof(output_conversion);
    specialization_info.pData                = &output_conversion;

    for (U32 i = 0; i < builder.shader_stage_count; ++i) {
        builder.shader_stages[i].pSpecializationInfo = &specialization_info;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state = { 0 };
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.vertexBindingDescriptionCount   = 1;
    vertex_input_state.pVertexBindingDescriptions      = &builder.vertex_binding_description;
    vertex_input_state.vertexAttributeDescriptionCount = builder.vertex_attribute_description_count;
    vertex_input_state.pVertexAttributeDescriptions    = builder.vertex_attribute_descriptions;

    VkPipelineViewportStateCreateInfo viewport_state = { 0 };
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = &builder.viewport;
    viewport_state.scissorCount  = 1;
    if (!builder.dynamic_scissor) {
        viewport_state.pScissors     = &builder.scissor;
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state = { 0 };
    color_blend_state.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable   = VK_FALSE;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments    = &builder.color_blend_attachment;

    VkDynamicState dynamic_states[1] = { 0 };
    VkPipelineDynamicStateCreateInfo dynamic_state = { 0 };
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pDynamicStates = dynamic_states;
    if (builder.dynamic_scissor) {
        dynamic_states[dynamic_state.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    }

    VkGraphicsPipelineCreateInfo pipeline_info = { 0 };
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount          = builder.shader_stage_count;
    pipeline_info.pStages             = builder.shader_stages;
    pipeline_info.pVertexInputState   = &vertex_input_state;
    pipeline_info.pInputAssemblyState = &builder.input_assembly_state;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &builder.rasterization_state;
    pipeline_info.pMultisampleState   = &builder.multisample_state;
    pipeline_info.pColorBlendState    = &color_blend_state;
    pipeline_info.pDynamicState       = &dynamic_state;
    pipeline_info.layout              = builder.pipeline_layout;
    pipeline_info.renderPass          = render_pass;
    pipeline_info.subpass             = 0;
    pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, 0, &pipeline);
    if (result != VK_SUCCESS) {
        error_emit(str8_literal("ERROR(graphics/vulkan): Failed to create graphics pipeline."));
    }

    return pipeline;
}

internal Void vulkan_begin_batch(VulkanState *state) {
    Vulkan_Batch *batch = arena_push_struct_zero(state->frame_arena, Vulkan_Batch);
    if (state->scissors) {
        batch->scissor = state->scissors->scissor;
    } else {
        batch->scissor = vulkan_rect2d(0, 0, state->swapchain.extent.width, state->swapchain.extent.height);
    }
    batch->first_shape = state->shape_count;

    sll_queue_push(state->first_batch, state->last_batch, batch);
}

internal Void vulkan_end_batch(VulkanState *state) {
    Vulkan_Batch *batch = state->last_batch;
    batch->shape_count = state->shape_count - batch->first_shape;
}

internal Void vulkan_create_render_pass(VulkanState *state) {
    VkAttachmentDescription color_attachment = { 0 };
    color_attachment.format         = state->swapchain.image_format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference = { 0 };
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &color_attachment_reference;

    // TODO: Do we really need this dependency? Look at Vulkan Tutorial for their explanation.
    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstSubpass    = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = { 0 };
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments    = &color_attachment;
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies   = &dependency;

    VULKAN_RESULT_CHECK(vkCreateRenderPass(state->device, &render_pass_info, 0, &state->render_pass));
}

internal Void vulkan_create_graphics_pipeline(VulkanState *state) {
    VkPipelineLayoutCreateInfo pipeline_layout_info = { 0 };
    pipeline_layout_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts    = &state->descriptor_set_layout;

    VULKAN_RESULT_CHECK(vkCreatePipelineLayout(state->device, &pipeline_layout_info, 0, &state->pipeline_layout));

    Arena_Temporary scratch = arena_get_scratch(0, 0);

    Str8 vertex_shader_source = { 0 };
    arena_align(scratch.arena, sizeof(U32));
    os_file_read(scratch.arena, str8_literal("build/sdf.vert.spv"), &vertex_shader_source);
    assert(vertex_shader_source.size);
    VkShaderModule vertex_shader_module = vulkan_create_shader_module(state->device, vertex_shader_source);

    Str8 fragment_shader_source = { 0 };
    arena_align(scratch.arena, sizeof(U32));
    os_file_read(scratch.arena, str8_literal("build/sdf.frag.spv"), &fragment_shader_source);
    assert(fragment_shader_source.size);
    VkShaderModule fragment_shader_module = vulkan_create_shader_module(state->device, fragment_shader_source);

    Vulkan_GraphicsPipelineBuilder builder = { 0 };

    builder.shader_stage_count = 2;
    builder.shader_stages      = arena_push_array(scratch.arena, VkPipelineShaderStageCreateInfo, builder.shader_stage_count);
    builder.shader_stages[0]   = vulkan_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT,   vertex_shader_module);
    builder.shader_stages[1]   = vulkan_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_module);

    builder.vertex_binding_description.binding   = 0;
    builder.vertex_binding_description.stride    = sizeof(Vulkan_Shape);
    builder.vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    builder.vertex_attribute_description_count = 7;
    builder.vertex_attribute_descriptions      = arena_push_array(scratch.arena, VkVertexInputAttributeDescription, builder.vertex_attribute_description_count);
    builder.vertex_attribute_descriptions[0]   = vulkan_vertex_attribute_description(0, 0, VK_FORMAT_R32_UINT,         member_offset(Vulkan_Shape, shape_type));
    builder.vertex_attribute_descriptions[1]   = vulkan_vertex_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT,    member_offset(Vulkan_Shape, position));
    builder.vertex_attribute_descriptions[2]   = vulkan_vertex_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT,    member_offset(Vulkan_Shape, size));
    builder.vertex_attribute_descriptions[3]   = vulkan_vertex_attribute_description(0, 3, VK_FORMAT_R32G32B32_SFLOAT, member_offset(Vulkan_Shape, color));
    builder.vertex_attribute_descriptions[4]   = vulkan_vertex_attribute_description(0, 4, VK_FORMAT_R32_UINT,         member_offset(Vulkan_Shape, texture_index));
    builder.vertex_attribute_descriptions[5]   = vulkan_vertex_attribute_description(0, 5, VK_FORMAT_R32G32_SFLOAT,    member_offset(Vulkan_Shape, uv0));
    builder.vertex_attribute_descriptions[6]   = vulkan_vertex_attribute_description(0, 6, VK_FORMAT_R32G32_SFLOAT,    member_offset(Vulkan_Shape, uv1));

    builder.input_assembly_state = vulkan_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    builder.viewport.x        = 0.0f;
    builder.viewport.y        = 0.0f;
    builder.viewport.width    = (F32) state->swapchain.extent.width;
    builder.viewport.height   = (F32) state->swapchain.extent.height;
    builder.viewport.minDepth = 0.0f;
    builder.viewport.maxDepth = 1.0f;
    builder.dynamic_scissor   = true;

    builder.rasterization_state        = vulkan_rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    builder.multisample_state          = vulkan_multisample_state_create_info();
    builder.color_blend_attachment     = vulkan_color_blend_attachment_state();
    builder.pipeline_layout            = state->pipeline_layout;

    state->graphics_pipeline = vulkan_build_graphics_pipeline(builder, state->device, state->render_pass, state->swapchain.image_format, state->swapchain.color_space);

    vkDestroyShaderModule(state->device, fragment_shader_module, 0);
    vkDestroyShaderModule(state->device, vertex_shader_module, 0);

    arena_end_temporary(scratch);
}

internal Void vulkan_create_framebuffers(VulkanState *state) {
    state->swapchain_framebuffers = arena_push_array(state->frame_arena, VkFramebuffer, state->swapchain.image_count);
    for (U32 i = 0; i < state->swapchain.image_count; ++i) {
        VkFramebufferCreateInfo framebuffer_info = { 0 };
        framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass      = state->render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments    = &state->swapchain.image_views[i];
        framebuffer_info.width           = state->swapchain.extent.width;
        framebuffer_info.height          = state->swapchain.extent.height;
        framebuffer_info.layers          = 1;

        VULKAN_RESULT_CHECK(vkCreateFramebuffer(state->device, &framebuffer_info, 0, &state->swapchain_framebuffers[i]));
    }
}

internal Void vulkan_create_swapchain_and_dependants(VulkanState *state, U32 width, U32 height) {
    state->swapchain.physical_device = state->physical_device;
    state->swapchain.device = state->device;
    state->swapchain.surface = state->surface;

    vulkan_swapchain_create_or_recreate(state->frame_arena, &state->swapchain, &width, &height);
    vulkan_create_render_pass(state);
    vulkan_create_graphics_pipeline(state);
    vulkan_create_framebuffers(state);
}

internal Void vulkan_cleanup_swapchain_dependants(VulkanState *state) {
    if (state->swapchain_framebuffers) {
        for (U32 i = 0; i < state->swapchain.image_count; ++i) {
            vkDestroyFramebuffer(state->device, state->swapchain_framebuffers[i], 0);
        }
    }
    vkDestroyPipeline(state->device, state->graphics_pipeline, 0);
    vkDestroyPipelineLayout(state->device, state->pipeline_layout, 0);
    vkDestroyRenderPass(state->device, state->render_pass, 0);
}

internal Void vulkan_recreate_swapchain(VulkanState *state, U32 width, U32 height) {
    vkDeviceWaitIdle(state->device);

    vulkan_cleanup_swapchain_dependants(state);

    vulkan_create_swapchain_and_dependants(state, width, height);
}

internal Void vulkan_create_command_buffers(VulkanState *state) {
    VkCommandPoolCreateInfo pool_info = { 0 };
    pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = state->graphics_queue_family;

    VkCommandBufferAllocateInfo allocate_info = { 0 };
    allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    for (U32 i = 0; i < array_count(state->frames); ++i) {
        Vulkan_Frame *frame = &state->frames[i];
        VULKAN_RESULT_CHECK(vkCreateCommandPool(state->device, &pool_info, 0, &frame->command_pool));

        allocate_info.commandPool = frame->command_pool;
        VULKAN_RESULT_CHECK(vkAllocateCommandBuffers(state->device, &allocate_info, &frame->command_buffer));
    }
}

internal Void vulkan_create_synchronization_objects(VulkanState *state) {
    VkSemaphoreCreateInfo semaphore_info = { 0 };
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = { 0 };
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (U32 i = 0; i < array_count(state->frames); ++i) {
        Vulkan_Frame *frame = &state->frames[i];
        VULKAN_RESULT_CHECK(vkCreateSemaphore(state->device, &semaphore_info, 0, &frame->image_available_semaphore));
        VULKAN_RESULT_CHECK(vkCreateSemaphore(state->device, &semaphore_info, 0, &frame->render_finished_semaphore));
        VULKAN_RESULT_CHECK(vkCreateFence(state->device, &fence_info, 0, &frame->in_flight_fence));
    }
}

internal Void vulkan_create_instance(VulkanState *state) {
    // TODO: Inspect the instance extensions in case we want to use any of them.
    // TODO: Potientially connect the validation layers to a thread safe logger.
    // TODO: Maybe supply a way of specifying application and engine names and versions.
    VkApplicationInfo application_info  = { 0 };
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info    = { 0 };
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &application_info;
    create_info.enabledLayerCount       = array_count(vulkan_instance_layers) - 1;
    create_info.ppEnabledLayerNames     = vulkan_instance_layers;
    create_info.enabledExtensionCount   = array_count(vulkan_instance_extensions);
    create_info.ppEnabledExtensionNames = vulkan_instance_extensions;

    VULKAN_RESULT_CHECK(vkCreateInstance(&create_info, 0, &state->instance));
}

internal Void vulkan_initialize_font(Arena *arena, VulkanState *state, FontDescription *font_description) {
    B32 success = true;

    Arena_Temporary scratch = arena_get_scratch(0, 0);

    TTF_Font font    = { 0 };
    U32 atlas_width  = 0;
    U32 atlas_height = 0;

    success = ttf_load(arena, scratch.arena, font_description, &font, &atlas_width, &atlas_height);

    // Create the GPU side atlas.
    if (success) {
        VkImageCreateInfo image_info = { 0 };
        image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType     = VK_IMAGE_TYPE_2D;
        image_info.format        = VK_FORMAT_R8G8B8A8_UNORM;
        image_info.extent.width  = atlas_width;
        image_info.extent.height = atlas_height;
        image_info.extent.depth  = 1;
        image_info.mipLevels     = 1;
        image_info.arrayLayers   = 1;
        image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VULKAN_RESULT_CHECK(vkCreateImage(state->device, &image_info, 0, &state->msdf_image));

        VkMemoryRequirements image_memory_requirements;
        vkGetImageMemoryRequirements(state->device, state->msdf_image, &image_memory_requirements);
        state->msdf_allocation = vulkan_allocate(&state->allocator, image_memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VULKAN_RESULT_CHECK(vkBindImageMemory(state->device, state->msdf_image, state->msdf_allocation.block->memory, state->msdf_allocation.offset));

        VkImageViewCreateInfo image_view_info = { 0 };
        image_view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.image                           = state->msdf_image;
        image_view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format                          = VK_FORMAT_R8G8B8A8_UNORM;
        image_view_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel   = 0;
        image_view_info.subresourceRange.levelCount     = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount     = 1;
        VULKAN_RESULT_CHECK(vkCreateImageView(state->device, &image_view_info, 0, &state->msdf_image_view));
    }

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    Vulkan_Allocation staging_buffer_allocation = { 0 };

    // Create CPU side staging buffer.
    if (success) {
        U64 buffer_size = atlas_width * atlas_height * sizeof(U32);

        VkBufferCreateInfo buffer_info = { 0 };
        buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size        = buffer_size;
        buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VULKAN_RESULT_CHECK(vkCreateBuffer(state->device, &buffer_info, 0, &staging_buffer));

        VkMemoryRequirements staging_buffer_memory_requirements;
        vkGetBufferMemoryRequirements(state->device, staging_buffer, &staging_buffer_memory_requirements);
        staging_buffer_allocation = vulkan_allocate(&state->allocator, staging_buffer_memory_requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        VULKAN_RESULT_CHECK(vkBindBufferMemory(state->device, staging_buffer, staging_buffer_allocation.block->memory, staging_buffer_allocation.offset));
    }

    // Generate the atlas.
    if (success) {
        U8 *staging_buffer_data = vulkan_allocator_map(staging_buffer_allocation, 0, VK_WHOLE_SIZE);

        ttf_generate_msdf_atlas(arena, staging_buffer_data, atlas_width, atlas_height, &font, &state->font);

        vulkan_allocator_flush(staging_buffer_allocation, 0, VK_WHOLE_SIZE);
        vulkan_allocator_unmap(staging_buffer_allocation);
    }

    // Transfer data from the staging buffer to the GPU side buffer.
    if (success) {
        // TODO: Command buffers that can be used for transfers
        VkCommandBuffer command_buffer = state->frames[0].command_buffer;

        VkCommandBufferBeginInfo begin_info = { 0 };
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VULKAN_RESULT_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

        VkImageMemoryBarrier first_barrier = { 0 };
        first_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        first_barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        first_barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        first_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        first_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        first_barrier.srcAccessMask                   = 0;
        first_barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        first_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        first_barrier.subresourceRange.baseMipLevel   = 0;
        first_barrier.subresourceRange.levelCount     = 1;
        first_barrier.subresourceRange.baseArrayLayer = 0;
        first_barrier.subresourceRange.layerCount     = 1;
        first_barrier.image                           = state->msdf_image;
        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, 0,
            0, 0,
            1, &first_barrier
        );

        VkBufferImageCopy copy_region = { 0 };
        copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.imageSubresource.mipLevel       = 0;
        copy_region.imageSubresource.baseArrayLayer = 0;
        copy_region.imageSubresource.layerCount     = 1;
        copy_region.imageExtent.width               = atlas_width;
        copy_region.imageExtent.height              = atlas_height;
        copy_region.imageExtent.depth               = 1;
        vkCmdCopyBufferToImage(command_buffer, staging_buffer, state->msdf_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

        VkImageMemoryBarrier second_barrier = { 0 };
        second_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        second_barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        second_barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        second_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        second_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        second_barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        second_barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        second_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        second_barrier.subresourceRange.baseMipLevel   = 0;
        second_barrier.subresourceRange.levelCount     = 1;
        second_barrier.subresourceRange.baseArrayLayer = 0;
        second_barrier.subresourceRange.layerCount     = 1;
        second_barrier.image                           = state->msdf_image;
        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, 0,
            0, 0,
            1, &second_barrier
        );

        VULKAN_RESULT_CHECK(vkEndCommandBuffer(command_buffer));

        VkSubmitInfo submit_info = { 0 };
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffer;
        VULKAN_RESULT_CHECK(vkQueueSubmit(state->graphics_queue, 1, &submit_info, 0));
        VULKAN_RESULT_CHECK(vkQueueWaitIdle(state->graphics_queue));

        VULKAN_RESULT_CHECK(vkResetCommandBuffer(command_buffer, 0));
    }

    if (staging_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(state->device, staging_buffer, 0);
        // TODO: Free staging_buffer_allocation through some kind of temporary allocation on the GPU.
    }

    arena_end_temporary(scratch);
}

internal B32 vulkan_initialize(Arena *arena, VulkanState *state, U32 window_width, U32 window_height, FontDescription *font_description) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    VkResult error = VK_SUCCESS;

    state->shape_capacity = 100000;

    if (error == VK_SUCCESS) {
        U32 device_count = 0;
        VULKAN_RESULT_CHECK(vkEnumeratePhysicalDevices(state->instance, &device_count, 0));
        VkPhysicalDevice *devices = arena_push_array(scratch.arena, VkPhysicalDevice, device_count);
        VULKAN_RESULT_CHECK(vkEnumeratePhysicalDevices(state->instance, &device_count, devices));

        U32 best_score = 0;
        for (U32 i = 0; i < device_count; ++i) {
            U32 score = vulkan_score_physical_device(scratch.arena, devices[i], state->surface, array_count(vulkan_device_extensions), vulkan_device_extensions);
            if (score > best_score) {
                state->physical_device = devices[i];
            }
        }

        if (state->physical_device == VK_NULL_HANDLE) {
            error_emit(str8_literal("ERROR(graphics/vulkan): Could not find a suitable physical device."));
            error = VK_ERROR_UNKNOWN;
        }
    }

    vkGetPhysicalDeviceProperties(state->physical_device, &state->physical_device_properties);

    if (error == VK_SUCCESS) {
        U32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(state->physical_device, &queue_family_count, 0);
        VkQueueFamilyProperties *queue_families = arena_push_array(scratch.arena, VkQueueFamilyProperties, queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(state->physical_device, &queue_family_count, queue_families);

        for (U32 i = 0; i < queue_family_count; ++i) {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(state->physical_device, i, state->surface, &present_support);
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                state->graphics_queue_family = i;
            }
            if (present_support) {
                state->present_queue_family = i;
            }
        }

        F32 queue_priorities[] = {
            1.0f,
        };
        VkDeviceQueueCreateInfo queue_create_infos[2] = { 0 };
        queue_create_infos[0].sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[0].queueFamilyIndex        = state->graphics_queue_family;
        queue_create_infos[0].queueCount              = array_count(queue_priorities);
        queue_create_infos[0].pQueuePriorities        = queue_priorities;

        queue_create_infos[1].sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[1].queueFamilyIndex        = state->present_queue_family;
        queue_create_infos[1].queueCount              = array_count(queue_priorities);
        queue_create_infos[1].pQueuePriorities        = queue_priorities;

        U32 queue_count = 2;
        if (queue_create_infos[0].queueFamilyIndex == queue_create_infos[1].queueFamilyIndex) {
            queue_count = 1;
        }

        VkPhysicalDeviceFeatures device_features = { 0 };

        VkDeviceCreateInfo create_info = { 0 };
        create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos       = queue_create_infos;
        create_info.queueCreateInfoCount    = queue_count;
        create_info.pEnabledFeatures        = &device_features;
        create_info.enabledLayerCount       = array_count(vulkan_device_layers) - 1;
        create_info.ppEnabledLayerNames     = vulkan_device_layers;
        create_info.enabledExtensionCount   = array_count(vulkan_device_extensions);
        create_info.ppEnabledExtensionNames = vulkan_device_extensions;

        VULKAN_RESULT_CHECK(vkCreateDevice(state->physical_device, &create_info, 0, &state->device));

        vkGetDeviceQueue(state->device, state->graphics_queue_family, 0, &state->graphics_queue);
        vkGetDeviceQueue(state->device, state->present_queue_family,  0, &state->present_queue);
    }

    vulkan_allocator_create(state->physical_device, state->device, &state->allocator);

    VkDescriptorSetLayoutBinding layout_bindings[4] = { 0 };
    layout_bindings[0].binding         = 0;
    layout_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[1].binding         = 1;
    layout_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
    layout_bindings[1].descriptorCount = 1;
    layout_bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[2].binding         = 2;
    layout_bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    layout_bindings[2].descriptorCount = 2;
    layout_bindings[2].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[3].binding         = 3;
    layout_bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    layout_bindings[3].descriptorCount = 1;
    layout_bindings[3].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = { 0 };
    descriptor_set_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.bindingCount = array_count(layout_bindings);
    descriptor_set_layout_info.pBindings    = layout_bindings;

    VULKAN_RESULT_CHECK(vkCreateDescriptorSetLayout(state->device, &descriptor_set_layout_info, 0, &state->descriptor_set_layout));

    vulkan_create_swapchain_and_dependants(state, window_width, window_height);

    vulkan_create_command_buffers(state);
    vulkan_create_synchronization_objects(state);

    {
        VkSamplerCreateInfo sampler_info = { 0 };
        sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter               = VK_FILTER_LINEAR;
        sampler_info.minFilter               = VK_FILTER_LINEAR;
        sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.mipLodBias              = 0.0f;
        sampler_info.anisotropyEnable        = VK_FALSE;
        sampler_info.compareEnable           = VK_FALSE;
        sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
        sampler_info.minLod                  = 0.0f;
        sampler_info.maxLod                  = 0.0f;
        sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        VULKAN_RESULT_CHECK(vkCreateSampler(state->device, &sampler_info, 0, &state->sampler));
    }

    vulkan_initialize_font(arena, state, font_description);

    for (U32 i = 0; i < array_count(state->frames); ++i) {
        Vulkan_Frame *frame = &state->frames[i];

        VkDescriptorPoolSize pool_sizes[3] = { 0 };
        pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount = 1;
        pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_SAMPLER;
        pool_sizes[1].descriptorCount = 1;
        pool_sizes[2].type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        pool_sizes[2].descriptorCount = 3;

        VkDescriptorPoolCreateInfo descriptor_pool_info = { 0 };
        descriptor_pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_info.maxSets       = 1;
        descriptor_pool_info.poolSizeCount = array_count(pool_sizes);
        descriptor_pool_info.pPoolSizes    = pool_sizes;

        VULKAN_RESULT_CHECK(vkCreateDescriptorPool(state->device, &descriptor_pool_info, 0, &frame->descriptor_pool));

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
        descriptor_set_allocate_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.descriptorPool     = frame->descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts        = &state->descriptor_set_layout;

        vkAllocateDescriptorSets(state->device, &descriptor_set_allocate_info, &frame->descriptor_set);

        VkBufferCreateInfo uniform_buffer_info = { 0 };
        uniform_buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        uniform_buffer_info.size        = sizeof(Vulkan_UniformBuffer2D);
        uniform_buffer_info.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uniform_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VULKAN_RESULT_CHECK(vkCreateBuffer(state->device, &uniform_buffer_info, 0, &frame->uniform_buffer));

        VkMemoryRequirements uniform_buffer_memory_requirements;
        vkGetBufferMemoryRequirements(state->device, frame->uniform_buffer, &uniform_buffer_memory_requirements);
        frame->uniform_allocation = vulkan_allocate(&state->allocator, uniform_buffer_memory_requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        vkBindBufferMemory(state->device, frame->uniform_buffer, frame->uniform_allocation.block->memory, frame->uniform_allocation.offset);

        VkDescriptorBufferInfo buffer_info = { 0 };
        buffer_info.buffer = frame->uniform_buffer;
        buffer_info.offset = 0;
        buffer_info.range  = sizeof(Vulkan_UniformBuffer2D);

        VkDescriptorImageInfo sampler_info = { 0 };
        sampler_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        sampler_info.sampler     = state->sampler;

        VkDescriptorImageInfo msdf_infos[2] = { 0 };
        msdf_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        msdf_infos[0].imageView   = state->msdf_image_view;

        VkWriteDescriptorSet descriptor_writes[3] = { 0 };
        descriptor_writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet          = frame->descriptor_set;
        descriptor_writes[0].dstBinding      = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].pBufferInfo     = &buffer_info;

        descriptor_writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet          = frame->descriptor_set;
        descriptor_writes[1].dstBinding      = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptor_writes[1].pImageInfo      = &sampler_info;

        descriptor_writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[2].dstSet          = frame->descriptor_set;
        descriptor_writes[2].dstBinding      = 3;
        descriptor_writes[2].dstArrayElement = 0;
        descriptor_writes[2].descriptorCount = 1;
        descriptor_writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptor_writes[2].pImageInfo      = msdf_infos;

        vkUpdateDescriptorSets(state->device, array_count(descriptor_writes), descriptor_writes, 0, 0);

        VkBufferCreateInfo shape_buffer_info = { 0 };
        shape_buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        shape_buffer_info.size        = state->shape_capacity * sizeof(*state->shapes);
        shape_buffer_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        shape_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VULKAN_RESULT_CHECK(vkCreateBuffer(state->device, &shape_buffer_info, 0, &frame->shape_buffer));

        VkMemoryRequirements shape_buffer_memory_requirements;
        vkGetBufferMemoryRequirements(state->device, frame->shape_buffer, &shape_buffer_memory_requirements);
        frame->shape_allocation = vulkan_allocate(&state->allocator, shape_buffer_memory_requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        vkBindBufferMemory(state->device, frame->shape_buffer, frame->shape_allocation.block->memory, frame->shape_allocation.offset);

        VkQueryPoolCreateInfo timer_info = { 0 };
        timer_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        timer_info.queryType  = VK_QUERY_TYPE_TIMESTAMP;
        timer_info.queryCount = GRAPHICS_TIMER_COUNT;
        VULKAN_RESULT_CHECK(vkCreateQueryPool(state->device, &timer_info, 0, &frame->timer_pool));
    }

    arena_end_temporary(scratch);

    return (error == VK_SUCCESS);
}

internal VkResult vulkan_begin_frame(VulkanState *state) {
    Vulkan_Frame *frame = &state->frames[state->current_frame_index];

    VkResult result = vulkan_swapchain_acquire_next_image(&state->swapchain, frame->image_available_semaphore);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return result;
    } else if (result == VK_SUBOPTIMAL_KHR) {
    } else {
        VULKAN_RESULT_CHECK(result);
    }

    VkImageView *new_image_views = arena_push_array(state->frame_arena, VkImageView, state->swapchain.image_count);
    memory_copy(new_image_views, state->swapchain.image_views, state->swapchain.image_count * sizeof(*new_image_views));
    state->swapchain.image_views = new_image_views;

    VkFramebuffer *new_framebuffers = arena_push_array(state->frame_arena, VkFramebuffer, state->swapchain.image_count);
    memory_copy(new_framebuffers, state->swapchain_framebuffers, state->swapchain.image_count * sizeof(*new_framebuffers));
    state->swapchain_framebuffers = new_framebuffers;

    VULKAN_RESULT_CHECK(vkResetFences(state->device, 1, &frame->in_flight_fence));

    U64 timer_data[GRAPHICS_TIMER_COUNT] = { 0 };
    VkResult query_result = vkGetQueryPoolResults(
        state->device,
        frame->timer_pool,
        0, GRAPHICS_TIMER_COUNT,
        GRAPHICS_TIMER_COUNT * sizeof(U64), timer_data, sizeof(U64),
        VK_QUERY_RESULT_64_BIT
    );
    if (query_result == VK_SUCCESS) {
        F32 ns = (timer_data[GRAPHICS_TIMER_AFTER_2D] - timer_data[GRAPHICS_TIMER_BEFORE_2D]) * state->physical_device_properties.limits.timestampPeriod;

        state->timer_data[state->timer_index] = ns;
        state->max_timer_data = f32_max(state->max_timer_data, ns);
        state->timer_index = (state->timer_index + 1) % array_count(state->timer_data);
    } else if (query_result == VK_NOT_READY) {
    } else {
        VULKAN_RESULT_CHECK(result);
    }

    state->shapes = vulkan_allocator_map(frame->shape_allocation, 0, VK_WHOLE_SIZE);

    state->shape_count = 0;
    state->first_batch = 0;
    state->last_batch  = 0;
    state->scissors    = 0;

    vulkan_begin_batch(state);

    return result;
}

internal Void vulkan_end_frame(VulkanState *state) {
    Vulkan_Frame *frame = &state->frames[state->current_frame_index];

    vulkan_end_batch(state);

    Vulkan_UniformBuffer2D *mapped_uniform_buffer = vulkan_allocator_map(frame->uniform_allocation, 0, sizeof(Vulkan_UniformBuffer2D));
    mapped_uniform_buffer->viewport_size = v2f32(state->swapchain.extent.width, state->swapchain.extent.height);

    vulkan_allocator_flush(frame->shape_allocation,   0, state->shape_count * sizeof(*state->shapes));
    vulkan_allocator_flush(frame->uniform_allocation, 0, sizeof(Vulkan_UniformBuffer2D));

    vulkan_allocator_unmap(frame->shape_allocation);
    vulkan_allocator_unmap(frame->uniform_allocation);

    VULKAN_RESULT_CHECK(vkResetCommandBuffer(frame->command_buffer, 0));

    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VULKAN_RESULT_CHECK(vkBeginCommandBuffer(frame->command_buffer, &begin_info));
    vkCmdResetQueryPool(frame->command_buffer, frame->timer_pool, 0, GRAPHICS_TIMER_COUNT);

    VkClearValue clear_color = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};

    // TODO: The amount of time taken to begin the render pass increases in
    // steps as the program runs. When the window is resized, the amount of
    // time taken decreases to reasonable levels again. It varies between 64us
    // and 600us.
    VkRenderPassBeginInfo render_pass_info = { 0 };
    render_pass_info.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass          = state->render_pass;
    render_pass_info.framebuffer         = state->swapchain_framebuffers[state->swapchain.current_image_index];
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.offset.y = 0;
    render_pass_info.renderArea.extent   = state->swapchain.extent;
    render_pass_info.clearValueCount     = 1;
    render_pass_info.pClearValues        = &clear_color;
    vkCmdBeginRenderPass(frame->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->graphics_pipeline);
    vkCmdBindDescriptorSets(frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline_layout, 0, 1, &frame->descriptor_set, 0, 0);
    VkDeviceSize shape_buffer_offset = 0;
    vkCmdBindVertexBuffers(frame->command_buffer, 0, 1, &frame->shape_buffer, &shape_buffer_offset);

    vkCmdWriteTimestamp(frame->command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, frame->timer_pool, GRAPHICS_TIMER_BEFORE_2D);
    for (Vulkan_Batch *batch = state->first_batch; batch; batch = batch->next) {
        if (batch->shape_count != 0) {
            vkCmdSetScissor(frame->command_buffer, 0, 1, &batch->scissor);
            vkCmdDraw(frame->command_buffer, 6, batch->shape_count, 0, batch->first_shape);
        }
    }

    vkCmdWriteTimestamp(frame->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frame->timer_pool, GRAPHICS_TIMER_AFTER_2D);

    vkCmdEndRenderPass(frame->command_buffer);

    VULKAN_RESULT_CHECK(vkEndCommandBuffer(frame->command_buffer));

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &frame->image_available_semaphore;
    submit_info.pWaitDstStageMask    = &wait_stage;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &frame->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &frame->render_finished_semaphore;

    VULKAN_RESULT_CHECK(vkQueueSubmit(state->graphics_queue, 1, &submit_info, frame->in_flight_fence));

    VkResult present_result = vulkan_swapchain_queue_present(&state->swapchain, state->present_queue, &frame->render_finished_semaphore);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
        // TODO: We might want to forward VK_SUBOPTIMAL_KHR to the windowing system.
    } else {
        VULKAN_RESULT_CHECK(present_result);
    }

    state->current_frame_index = (state->current_frame_index + 1) % array_count(state->frames);
}

internal Void vulkan_cleanup(VulkanState *state) {
    vkDeviceWaitIdle(state->device);

    for (U32 i = 0; i < array_count(state->frames); ++i) {
        Vulkan_Frame *frame = &state->frames[i];

        vkDestroySemaphore(state->device, frame->image_available_semaphore, 0);
        vkDestroySemaphore(state->device, frame->render_finished_semaphore, 0);
        vkDestroyFence(state->device, frame->in_flight_fence, 0);

        vkDestroyCommandPool(state->device, frame->command_pool, 0);

        vkDestroyDescriptorPool(state->device, frame->descriptor_pool, 0);

        vkDestroyBuffer(state->device, frame->uniform_buffer, 0);

        vkDestroyBuffer(state->device, frame->shape_buffer, 0);

        vkDestroyQueryPool(state->device, frame->timer_pool, 0);
    }

    vkDestroySampler(state->device, state->sampler, 0);
    vkDestroyImageView(state->device, state->msdf_image_view, 0);
    vkDestroyImage(state->device, state->msdf_image, 0);

    vkDestroyDescriptorSetLayout(state->device, state->descriptor_set_layout, 0);

    vulkan_alloctor_destroy(&state->allocator);

    vulkan_cleanup_swapchain_dependants(state);
    vulkan_swapchain_destroy(&state->swapchain);

    vkDestroyDevice(state->device, 0);
    vkDestroySurfaceKHR(state->instance, state->surface, 0);
    vkDestroyInstance(state->instance, 0);
}

internal Void graphics_push_scissor(GraphicsContext *state, V2F32 position, V2F32 size, B32 should_inherit) {
    VulkanState *vulkan = (VulkanState *) state->pointer;

    vulkan_end_batch(vulkan);

    Vulkan_Scissor *scissor = arena_push_struct_zero(vulkan->frame_arena, Vulkan_Scissor);
    scissor->scissor.offset.x      = f32_floor(position.x * vulkan->surface_scale);
    scissor->scissor.offset.y      = f32_floor(position.y * vulkan->surface_scale);
    scissor->scissor.extent.width  = f32_floor(size.x * vulkan->surface_scale);
    scissor->scissor.extent.height = f32_floor(size.y * vulkan->surface_scale);

    VkRect2D parent = vulkan_rect2d(0, 0, vulkan->swapchain.extent.width, vulkan->swapchain.extent.height);
    if (should_inherit && vulkan->scissors) {
        parent = vulkan->scissors->scissor;
    }
    scissor->scissor = vulkan_rect2d_intersection(scissor->scissor, parent);

    sll_stack_push(vulkan->scissors, scissor);
    vulkan_begin_batch(vulkan);
}

internal Void graphics_pop_scissor(GraphicsContext *state) {
    VulkanState *vulkan = (VulkanState *) state->pointer;

    vulkan_end_batch(vulkan);
    sll_stack_pop(vulkan->scissors);
    vulkan_begin_batch(vulkan);
}

internal Void graphics_rectangle(GraphicsContext *state, V2F32 position, V2F32 size, V3F32 color) {
    VulkanState *vulkan = (VulkanState *) state->pointer;

    if (vulkan->shape_count < vulkan->shape_capacity) {
        Vulkan_Shape rectangle = { 0 };

        rectangle.shape_type    = SDF_TYPE_RECTANGLE;
        rectangle.position      = v2f32_scale(position, vulkan->surface_scale);
        rectangle.size          = v2f32_scale(size, vulkan->surface_scale);
        rectangle.color         = color;

        vulkan->shapes[vulkan->shape_count++] = rectangle;
    }
}

internal Void graphics_rectangle_texture(GraphicsContext *state, V2F32 position, V2F32 size, V3F32 color, U32 texture_index, V2F32 uv0, V2F32 uv1) {
    VulkanState *vulkan = (VulkanState *) state->pointer;

    if (vulkan->shape_count < vulkan->shape_capacity) {
        Vulkan_Shape rectangle = { 0 };

        rectangle.shape_type    = SDF_TYPE_RECTANGLE;
        rectangle.position      = v2f32_scale(position, vulkan->surface_scale);
        rectangle.size          = v2f32_scale(size, vulkan->surface_scale);
        rectangle.color         = color;
        rectangle.texture_index = texture_index;
        rectangle.uv0           = uv0;
        rectangle.uv1           = uv1;

        vulkan->shapes[vulkan->shape_count++] = rectangle;
    }
}

internal Void graphics_circle(GraphicsContext *state, V2F32 position, F32 radius, V3F32 color) {
    VulkanState *vulkan = (VulkanState *) state->pointer;

    if (vulkan->shape_count < vulkan->shape_capacity) {
        Vulkan_Shape circle = { 0 };

        circle.shape_type    = SDF_TYPE_CIRCLE;
        circle.position      = v2f32_scale(position, vulkan->surface_scale);
        circle.size.x        = radius * vulkan->surface_scale;
        circle.color         = color;

        vulkan->shapes[vulkan->shape_count++] = circle;
    }
}

internal Void graphics_circle_texture(GraphicsContext *state, V2F32 position, F32 radius, V3F32 color, U32 texture_index, V2F32 uv0, V2F32 uv1) {
    VulkanState *vulkan = (VulkanState *) state->pointer;

    if (vulkan->shape_count < vulkan->shape_capacity) {
        Vulkan_Shape circle = { 0 };

        circle.shape_type    = SDF_TYPE_CIRCLE;
        circle.position      = v2f32_scale(position, vulkan->surface_scale);
        circle.size.x        = radius * vulkan->surface_scale;
        circle.color         = color;
        circle.texture_index = texture_index;
        circle.uv0           = uv0;
        circle.uv1           = uv1;

        vulkan->shapes[vulkan->shape_count++] = circle;
    }
}

internal Void graphics_line(GraphicsContext *state, V2F32 p0, V2F32 p1, F32 width, V3F32 color) {
    VulkanState *vulkan = (VulkanState *) state->pointer;

    if (vulkan->shape_count < vulkan->shape_capacity) {
        Vulkan_Shape line = { 0 };

        line.shape_type = SDF_TYPE_LINE;
        line.position   = v2f32_scale(p0, vulkan->surface_scale);
        line.size       = v2f32_scale(p1, vulkan->surface_scale);
        line.color      = color;

        vulkan->shapes[vulkan->shape_count++] = line;
    }
}

internal Void graphics_msdf(GraphicsContext *state, V2F32 position, V2F32 size, V3F32 color, V2F32 uv0, V2F32 uv1) {
    VulkanState *vulkan = (VulkanState *) state->pointer;

    if (vulkan->shape_count < vulkan->shape_capacity) {
        Vulkan_Shape msdf = { 0 };

        msdf.shape_type    = SDF_TYPE_MSDF;
        msdf.position      = v2f32_scale(position, vulkan->surface_scale);
        msdf.size          = v2f32_scale(size, vulkan->surface_scale);
        msdf.color         = color;
        msdf.uv0           = uv0;
        msdf.uv1           = uv1;

        vulkan->shapes[vulkan->shape_count++] = msdf;
    }
}

internal Void graphics_text(GraphicsContext *state, V2F32 position, F32 point_size, Str8 text) {
    VulkanState *vulkan = (VulkanState *) state->pointer;
    S32 origin_funit_x = 0;
    S32 origin_funit_y = 0;

    state->dpi = v2f32(92, 92);

    V2F32 funits_to_pixels = v2f32_scale(state->dpi, point_size / (72.0f * vulkan->font.funits_per_em));

    U8 *codepoint_ptr = text.data;
    U8 *codepoint_opl = text.data + text.size;
    while (codepoint_ptr < codepoint_opl) {
        StringDecode decode = string_decode_utf8(codepoint_ptr, (U64) (codepoint_opl - codepoint_ptr));
        codepoint_ptr += decode.size;

        U32 glyph_index = font_codepoint_to_glyph_index(&vulkan->font, decode.codepoint);
        Font_Glyph *glyph = &vulkan->font.glyphs[glyph_index];

        if (decode.codepoint == '\n') {
            origin_funit_x  = 0;
            origin_funit_y += vulkan->font.funits_per_em;
        } else if (decode.codepoint == '\r') {
            origin_funit_x = 0;
        } else if (decode.codepoint == '\f') {
            origin_funit_y += vulkan->font.funits_per_em;
        } else if (decode.codepoint == '\t') {
            U32 glyph_index = font_codepoint_to_glyph_index(&vulkan->font, ' ');
            origin_funit_x += 4 * vulkan->font.glyphs[glyph_index].advance_width;
        } else {
            // TODO: Only render if the character has a glyph.
            if (vulkan->shape_count < vulkan->shape_capacity) {
                F32 width    = glyph->x_max - glyph->x_min;
                F32 height   = glyph->y_max - glyph->y_min;
                F32 x_offset = (F32) origin_funit_x + glyph->x_min + (F32) glyph->left_side_bearing;
                F32 y_offset = (F32) origin_funit_y - glyph->y_max;

                Vulkan_Shape msdf = { 0 };

                msdf.shape_type = SDF_TYPE_MSDF;
                msdf.position = v2f32(
                    position.x * vulkan->surface_scale + x_offset * funits_to_pixels.x,
                    position.y * vulkan->surface_scale + y_offset * funits_to_pixels.y
                );
                msdf.size  = v2f32(width * funits_to_pixels.x, height * funits_to_pixels.y);
                msdf.color = v3f32(1, 1, 1);
                msdf.uv0   = glyph->uv0;
                msdf.uv1   = glyph->uv1;

                vulkan->shapes[vulkan->shape_count++] = msdf;
            }
            origin_funit_x += glyph->advance_width;
        }
    }
}
