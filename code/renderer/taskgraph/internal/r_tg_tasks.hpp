#pragma once

#include "r_tg_common.hpp"
#include "renderer/graphics_api/buffer.hpp"
#include <functional>

namespace PG::Gfx
{

struct TGExecuteData;

class Task
{
public:
    TG_DEBUG_ONLY( std::string debugName );
    std::vector<VkImageMemoryBarrier2> imageBarriers;
    std::vector<VkBufferMemoryBarrier2> bufferBarriers;

    virtual ~Task()                                    = default;
    virtual void Execute( TGExecuteData* executeData ) = 0;
    virtual void SubmitBarriers( TGExecuteData* executeData );
};

struct BufferClearSubTask
{
    TGResourceHandle bufferHandle;
    uint32_t clearVal;
};

struct TextureClearSubTask
{
    TGResourceHandle textureHandle;
    vec4 clearVal;
};

class ComputeTask;
using ComputeFunction = std::function<void( ComputeTask*, TGExecuteData* )>;

class ComputeTask : public Task
{
public:
    void Execute( TGExecuteData* data ) override;

    std::vector<BufferClearSubTask> bufferClears;
    std::vector<TextureClearSubTask> textureClears;
    std::vector<VkImageMemoryBarrier2> imageBarriersPreClears;

    std::vector<TGResourceHandle> inputBuffers;
    std::vector<TGResourceHandle> outputBuffers;
    std::vector<TGResourceHandle> inputTextures;
    std::vector<TGResourceHandle> outputTextures;

    ComputeFunction function;
};

struct TextureTransfer
{
    TGResourceHandle dst;
    TGResourceHandle src;
};

class TransferTask : public Task
{
public:
    void Execute( TGExecuteData* data ) override;

    std::vector<TextureTransfer> textureBlits;
};

class PresentTask : public Task
{
public:
    void Execute( TGExecuteData* data ) override;
};

} // namespace PG::Gfx
