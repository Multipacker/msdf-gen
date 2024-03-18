/*
 * This is a super simple first fit bump allocator that doesn't support
 * freeing. It is written to be simple and easy to understand, not to be a
 * general purpose allocator or to be the best allocator ever. It picks the
 * first memory type that fits the requirements and allocates fromt it.
 */

// TODO: Allow for higher level allocations that can signal preferred memory
// properties (e.g. CPU -> GPU would give VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
// VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).

internal S32 vulkan_allocator_find_memory_type_index(Vulkan_Allocator *allocator, U32 required_memory_type_bits, VkMemoryPropertyFlags required_properties) {
    for (U32 memory_index = 0; memory_index < allocator->memory_type_count; ++memory_index) {
        U32 memory_type_bits = 1 << memory_index;
        B32 is_required_memory_type = memory_type_bits & required_memory_type_bits;

        VkMemoryPropertyFlags properties = allocator->memory_types[memory_index].properties;
        B32 has_required_properties = (properties & required_properties) == required_properties;

        if (is_required_memory_type && has_required_properties) {
            return memory_index;
        }
    }

    return -1;
}

internal Vulkan_AllocationBlock *vulkan_allocator_allocate_block(Vulkan_Allocator *allocator, U32 memory_type_index, VkDeviceSize size) {
    assert(allocator->next_block < array_count(allocator->blocks));

    Vulkan_AllocationBlock *block = &allocator->blocks[allocator->next_block++];
    block->allocator         = allocator;
    block->memory_type_index = memory_type_index;

    block->size     = size;
    block->position = 0;

    VkMemoryAllocateInfo allocate_info = { 0 };
    allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize  = size;
    allocate_info.memoryTypeIndex = memory_type_index;
    VULKAN_RESULT_CHECK(vkAllocateMemory(allocator->device, &allocate_info, 0, &block->memory));

    block->map_count      = 0;

    return block;
}

internal Vulkan_Allocation vulkan_allocate(Vulkan_Allocator *allocator, VkMemoryRequirements memory_requirements, VkMemoryPropertyFlags required_properties) {
    S32 memory_type_index = vulkan_allocator_find_memory_type_index(allocator, memory_requirements.memoryTypeBits, required_properties);
    assert(memory_type_index != -1);

    Vulkan_MemoryType *memory_type = &allocator->memory_types[memory_type_index];

    if (memory_requirements.size < VULKAN_ALLOCATOR_MAX_SMALL_ALLOCATION) {
        // Small allocation, put it inside of a block.
        Vulkan_AllocationBlock *allocation_block = 0;

        // Look for a block that fits the allocation, taking alignment into account.
        for (Vulkan_AllocationBlock *block = memory_type->first_block; block; block = block->next) {
            VkDeviceSize aligned_position = u64_round_up_to_power_of_2(block->position, memory_requirements.alignment);

            if (aligned_position + memory_requirements.size <= block->size) {
                allocation_block = block;
                break;
            }
        }

        // If we didn't find a block with enough space, allocate a new one.
        if (!allocation_block) {
            allocation_block = vulkan_allocator_allocate_block(allocator, memory_type_index, VULKAN_ALLOCATOR_BLOCK_SIZE);
            dll_push_back(memory_type->first_block, memory_type->last_block, allocation_block);
        }

        // Perform the allocation.
        VkDeviceSize aligned_position = u64_round_up_to_power_of_2(allocation_block->position, memory_requirements.alignment);

        Vulkan_Allocation result = { 0 };
        result.block  = allocation_block;
        result.offset = aligned_position;
        result.size   = memory_requirements.size;

        allocation_block->position = aligned_position + result.size;

        return result;
    } else {
        // Large allocation, give it its own block.
        Vulkan_AllocationBlock *block = vulkan_allocator_allocate_block(allocator, memory_type_index, memory_requirements.size);
        dll_push_back(memory_type->first_block, memory_type->last_block, block);

        Vulkan_Allocation result = { 0 };
        result.block  = block;
        result.offset = 0;
        result.size   = memory_requirements.size;

        block->position = result.size;

        return result;
    }
}

internal Void *vulkan_allocator_map(Vulkan_Allocation allocation, VkDeviceSize offset, VkDeviceSize size) {
    Vulkan_AllocationBlock *block       = allocation.block;
    Vulkan_Allocator       *allocator   = block->allocator;
    Vulkan_MemoryType      *memory_type = &allocator->memory_types[block->memory_type_index];

    assert(memory_type->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    assert(offset < allocation.size);
    if (size != VK_WHOLE_SIZE) {
        assert(offset + size <= allocation.size);
    }

    if (block->map_count == 0) {
        VULKAN_RESULT_CHECK(vkMapMemory(allocator->device, block->memory, 0, VK_WHOLE_SIZE, 0, &block->mapped_pointer));
    }

    ++block->map_count;
    Void *result = (U8 *) block->mapped_pointer + allocation.offset + offset;

    return result;
}

internal Void vulkan_allocator_unmap(Vulkan_Allocation allocation) {
    Vulkan_AllocationBlock *block     = allocation.block;
    Vulkan_Allocator       *allocator = block->allocator;

    assert(block->map_count != 0);

    --block->map_count;

    if (block->map_count == 0) {
        vkUnmapMemory(allocator->device, block->memory);
    }
}

internal Void vulkan_allocator_flush(Vulkan_Allocation allocation, VkDeviceSize offset, VkDeviceSize size) {
    Vulkan_AllocationBlock *block       = allocation.block;
    Vulkan_Allocator       *allocator   = block->allocator;
    Vulkan_MemoryType      *memory_type = &allocator->memory_types[block->memory_type_index];

    assert(block->map_count != 0);
    assert(offset < allocation.size);

    if (!(memory_type->properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        VkMappedMemoryRange memory_range = { 0 };
        memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memory_range.memory = block->memory;
        memory_range.offset = u64_round_down_to_power_of_2(allocation.offset + offset, allocator->non_coherent_atom_size);

        if (size == VK_WHOLE_SIZE) {
            memory_range.size = allocation.size - offset;
        } else {
            assert(offset + size < allocation.size);
            memory_range.size = size;
        }

        memory_range.size = u64_round_up_to_power_of_2(memory_range.size + (offset - memory_range.offset), allocator->non_coherent_atom_size);

        assert(allocation.offset % allocator->non_coherent_atom_size == 0);

        memory_range.offset += allocation.offset;
        memory_range.size = u64_min(memory_range.size, block->size - memory_range.size);

        VULKAN_RESULT_CHECK(vkFlushMappedMemoryRanges(allocator->device, 1, &memory_range));
    }
}

internal Void vulkan_allocator_create(VkPhysicalDevice physical_device, VkDevice device, Vulkan_Allocator *allocator) {
    allocator->device     = device;
    allocator->next_block = 0;

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    allocator->memory_type_count = memory_properties.memoryTypeCount;
    for (U32 memory_index = 0; memory_index < allocator->memory_type_count; ++memory_index) {
        allocator->memory_types[memory_index].properties  = memory_properties.memoryTypes[memory_index].propertyFlags;
        allocator->memory_types[memory_index].first_block = 0;
        allocator->memory_types[memory_index].last_block = 0;
    }

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);

    allocator->non_coherent_atom_size = device_properties.limits.nonCoherentAtomSize;
}

internal Void vulkan_alloctor_destroy(Vulkan_Allocator *allocator) {
    for (U32 memory_index = 0; memory_index < allocator->memory_type_count; ++memory_index) {
        for (Vulkan_AllocationBlock *block = allocator->memory_types[memory_index].first_block; block; block = block->next) {
            vkFreeMemory(allocator->device, block->memory, 0);
        }
    }
}
