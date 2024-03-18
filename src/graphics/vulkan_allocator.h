#ifndef VULKAN_ALLOCATOR_H
#define VULKAN_ALLOCATOR_H

#define VULKAN_ALLOCATOR_BLOCK_SIZE           megabytes(256)
#define VULKAN_ALLOCATOR_MAX_SMALL_ALLOCATION megabytes(64)
#define VULKAN_ALLOCATOR_MAX_BLOCKS           128

typedef struct Vulkan_AllocationBlock Vulkan_AllocationBlock;
typedef struct Vulkan_Allocator Vulkan_Allocator;

struct Vulkan_AllocationBlock {
    Vulkan_AllocationBlock *next;
    Vulkan_AllocationBlock *previous;

    Vulkan_Allocator *allocator;
    U32               memory_type_index;

    VkDeviceMemory memory;
    VkDeviceSize   size;
    VkDeviceSize   position;

    U32 map_count;
    Void *mapped_pointer;
};

typedef struct {
    VkMemoryPropertyFlags properties;

    Vulkan_AllocationBlock *first_block;
    Vulkan_AllocationBlock *last_block;
} Vulkan_MemoryType;

struct Vulkan_Allocator {
    VkDevice device;

    VkDeviceSize non_coherent_atom_size;

    Vulkan_MemoryType memory_types[VK_MAX_MEMORY_TYPES];
    U32               memory_type_count;

    Vulkan_AllocationBlock blocks[VULKAN_ALLOCATOR_MAX_BLOCKS];
    U32                    next_block;
};

typedef struct {
    Vulkan_AllocationBlock *block;
    VkDeviceSize offset;
    VkDeviceSize size;
} Vulkan_Allocation;

internal Vulkan_Allocation vulkan_allocate(Vulkan_Allocator *allocator, VkMemoryRequirements memory_requirements, VkMemoryPropertyFlags required_properties);

internal Void *vulkan_allocator_map(Vulkan_Allocation allocation, VkDeviceSize offset, VkDeviceSize size);
internal Void vulkan_allocator_unmap(Vulkan_Allocation allocation);
internal Void vulkan_allocator_flush(Vulkan_Allocation allocation, VkDeviceSize offset, VkDeviceSize size);

internal Void vulkan_allocator_create(VkPhysicalDevice physical_device, VkDevice device, Vulkan_Allocator *allocator);
internal Void vulkan_alloctor_destroy(Vulkan_Allocator *allocator);

#endif // VULKAN_ALLOCATOR_H
