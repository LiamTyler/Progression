#pragma once

#include "r_tg_common.hpp"
#include <functional>

namespace PG::Gfx
{

struct TGExecuteData;
class TaskGraph;

class Task
{
public:
#if USING( PG_GPU_PROFILING ) || USING( TG_DEBUG )
    std::string name;
#endif // #if USING( PG_GPU_PROFILING ) || USING( TG_DEBUG )
    std::vector<VkImageMemoryBarrier2> imageBarriers;
    std::vector<VkBufferMemoryBarrier2> bufferBarriers;

    virtual ~Task()                                    = default;
    virtual void Execute( TGExecuteData* executeData ) = 0;
    virtual void SubmitBarriers( TGExecuteData* executeData );

    TG_DEBUG_ONLY( virtual void Print( TaskGraph* taskGraph ) const );
};

struct BufferClearSubTask
{
    TGResourceHandle bufferHandle;
    u32 clearVal;
};

struct TextureClearSubTask
{
    TGResourceHandle textureHandle;
    vec4 clearVal;
};

class ComputeTask;
using ComputeFunction = std::function<void( ComputeTask*, TGExecuteData* )>;

class PipelineTask : public Task
{
public:
    TG_DEBUG_ONLY( virtual void Print( TaskGraph* taskGraph ) const override );

    std::vector<BufferClearSubTask> bufferClears;
    std::vector<TextureClearSubTask> textureClears;
    std::vector<VkImageMemoryBarrier2> imageBarriersPreClears;

    std::vector<TGResourceHandle> inputBuffers;
    std::vector<TGResourceHandle> outputBuffers;
    std::vector<TGResourceHandle> inputTextures;
    std::vector<TGResourceHandle> outputTextures;
};

class ComputeTask : public PipelineTask
{
public:
    void Execute( TGExecuteData* data ) override;
    TG_DEBUG_ONLY( virtual void Print( TaskGraph* taskGraph ) const override );

    ComputeFunction function;
};

class GraphicsTask;
using GraphicsFunction = std::function<void( GraphicsTask*, TGExecuteData* )>;

class GraphicsTask : public PipelineTask
{
public:
    ~GraphicsTask();
    void Execute( TGExecuteData* data ) override;
    TG_DEBUG_ONLY( virtual void Print( TaskGraph* taskGraph ) const override );

    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    VkRenderingAttachmentInfo* depthAttach = nullptr;
    VkRenderingInfo renderingInfo;
    GraphicsFunction function;
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
    TG_DEBUG_ONLY( virtual void Print( TaskGraph* taskGraph ) const override );

    std::vector<TextureTransfer> textureBlits;
};

class PresentTask : public Task
{
public:
    void Execute( TGExecuteData* data ) override;
    TG_DEBUG_ONLY( virtual void Print( TaskGraph* taskGraph ) const override );
};

} // namespace PG::Gfx
