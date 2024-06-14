#pragma once

#include "graphics_api/buffer.hpp"
#include "graphics_api/command_buffer.hpp"
#include "graphics_api/synchronization.hpp"

namespace PG::Gfx
{

enum class UploadCommandType : u8
{
    BUFFER_UPLOAD,
    TEXTURE_UPLOAD,
    LAYOUT_TRANSITION_PRE,
    LAYOUT_TRANSITION_POST,
};

struct BufferUploadRequest
{
    u64 dstOffset;
    u32 size;
    u32 offsetInStagingBuffer;
};

struct TextureUploadRequest
{
    VkImageSubresourceLayers imageSubresource;
    VkExtent3D imageExtent;
};

struct UploadRequest
{
    UploadRequest() = default;
    UploadRequest( UploadCommandType inType ) : type( inType ) {}

    UploadCommandType type;
    u32 size = 0;
    union
    {
        VkImage image;
        VkBuffer buffer;
    };
    union
    {
        u64 offset;
        TextureUploadRequest texReq;
    };
};

class UploadManager
{
public:
    static constexpr u32 NUM_BUFFERS = 2;

    Buffer stagingBuffers[NUM_BUFFERS];
    u64 currentSizes[NUM_BUFFERS];
    std::vector<UploadRequest> requests[NUM_BUFFERS];
    Fence fences[NUM_BUFFERS];
    CommandPool cmdPool;
    CommandBuffer cmdBufs[NUM_BUFFERS];
    std::vector<VkBufferMemoryBarrier2> pendingBufferBarriers;
    std::vector<VkImageMemoryBarrier2> pendingImageBarriers;

    void Init();
    void Free();

    void AddRequest( const UploadRequest& req, const char* data );
    void FlushAll();

private:
    u32 currentBufferIdx;

    void StartUploads();
    void SwapBuffers();
};

} // namespace PG::Gfx
