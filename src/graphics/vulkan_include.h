#ifndef VULKAN_INCLUDE_H
#define VULKAN_INCLUDE_H

#include <vulkan/vulkan.h>
#include "vulkan_allocator.h"

#define GRAPHICS_VULKAN_FRAMES_IN_FLIGHT 2

typedef struct {
    // NOTE: Resources not owned by the swapchain.
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;

    VkSwapchainKHR  swapchain;
    VkFormat        image_format;
    VkColorSpaceKHR color_space;
    U32             image_count;
    U32             current_image_index;
    VkImageView    *image_views;

    VkExtent2D      extent;
} Vulkan_Swapchain;

typedef struct {
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence     in_flight_fence;

    VkCommandPool   command_pool;
    VkCommandBuffer command_buffer; // NOTE: Implicitly destroyed with command pool.

    VkDescriptorPool  descriptor_pool;
    VkDescriptorSet   descriptor_set;

    VkBuffer          uniform_buffer;
    Vulkan_Allocation uniform_allocation;
    VkBuffer          shape_buffer;
    Vulkan_Allocation shape_allocation;

    VkQueryPool timer_pool;
} Vulkan_Frame;

typedef struct {
    V2F32 viewport_size;
    U32   render_msdf;
} Vulkan_UniformBuffer2D;

typedef struct {
    U32   shape_type;
    V2F32 position;
    V2F32 size;
    V3F32 color;
    U32   texture_index;
    V2F32 uv0;
    V2F32 uv1;
} Vulkan_Shape;

typedef struct Vulkan_Batch Vulkan_Batch;
struct Vulkan_Batch {
    Vulkan_Batch *next;

    VkRect2D scissor;

    U32 first_shape;
    U32 shape_count;
};

typedef struct Vulkan_Scissor Vulkan_Scissor;
struct Vulkan_Scissor {
    Vulkan_Scissor *next;
    VkRect2D scissor;
};

typedef enum {
    GRAPHICS_TIMER_BEFORE_2D,
    GRAPHICS_TIMER_AFTER_2D,
    GRAPHICS_TIMER_COUNT,
} Graphics_Timer;

typedef struct {
    // Per program data
    VkInstance       instance;
    VkPhysicalDevice physical_device; // NOTE: Implicitly destroyed with instance.
    VkPhysicalDeviceProperties physical_device_properties;
    VkDevice         device;
    U32              graphics_queue_family;
    VkQueue          graphics_queue;  // NOTE: Implicitly destroyed with logical device.
    U32              present_queue_family;
    VkQueue          present_queue;   // NOTE: Implicitly destroyed with logical device.
    Vulkan_Allocator allocator;

    // Per surface data.
    U32 surface_scale;
    VkSurfaceKHR     surface;
    Vulkan_Swapchain swapchain;
    VkFramebuffer   *swapchain_framebuffers;

    // While rendering data
    Arena          *frame_arena; 
    U32             shape_capacity;
    U32             shape_count;
    Vulkan_Shape   *shapes;
    Vulkan_Batch   *first_batch;
    Vulkan_Batch   *last_batch;
    Vulkan_Scissor *scissors;

    VkRenderPass          render_pass;
    VkPipelineLayout      pipeline_layout;
    VkPipeline            graphics_pipeline;
    VkDescriptorSetLayout descriptor_set_layout;
    VkSampler             sampler;

    U32 timer_index;
    F32 max_timer_data;
    F32 timer_data[200];

    Vulkan_Allocation msdf_allocation;
    VkImage           msdf_image;
    VkImageView       msdf_image_view;
    U32               render_msdf;

    // Frames in flight
    U32          current_frame_index;
    Vulkan_Frame frames[GRAPHICS_VULKAN_FRAMES_IN_FLIGHT];
} VulkanState;

// TODO: Logging
#define VULKAN_RESULT_CHECK(expression) \
    do { \
        VkResult result##__LINE__ = (expression); \
        if (result##__LINE__ != VK_SUCCESS) { \
            os_console_print(str8_literal("ASSERT(graphics/vulkan): " #expression "\n")); \
            assert(result##__LINE__ == VK_SUCCESS); \
        } \
    } while(0)

#endif // VULKAN_INCLUDE_H
