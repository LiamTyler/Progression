#pragma once

#include "internal/r_tg_builder.hpp"
#include "internal/r_tg_resource_packing.hpp"
#include "internal/r_tg_tasks.hpp"
#include "renderer/graphics_api/buffer.hpp"
#include "renderer/graphics_api/command_buffer.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "renderer/r_globals.hpp"
#include <utility>

namespace PG
{
class Scene;
}

namespace PG::Gfx
{

class TaskGraph;

struct TGExecuteData
{
    Scene* scene;
    FrameData* frameData;
    CommandBuffer* cmdBuf;
    TaskGraph* taskGraph;

    std::vector<VkImageMemoryBarrier2> scratchImageBarriers;
    std::vector<VkBufferMemoryBarrier2> scratchBufferBarriers;
    std::vector<VkRenderingAttachmentInfo> scratchColorAttachmentInfos;
    VkRenderingAttachmentInfo scratchDepthAttachmentInfo;
};

struct TGStats
{
    // all the sizes are in bytes
    size_t unAliasedTextureMem;
    uint32_t numTextures;
    size_t unAliasedBufferMem;
    uint32_t numBuffers;
    size_t totalMemoryPostAliasing;

    uint32_t numComputeTasks;
    uint32_t numGraphicsTasks;
    uint32_t numTransferTasks;

    uint32_t numBarriers_Buffer;
    uint32_t numBarriers_Image;
    uint32_t numBarriers_Global;

    float compileTimeMSec;
    float resAllocTimeMSec;
};

class TaskGraph
{
public:
    struct CompileInfo
    {
        uint32_t sceneWidth;
        uint32_t sceneHeight;
        uint32_t displayWidth;
        uint32_t displayHeight;
        bool mergeResources = true;
        bool showStats      = true; // only works if TG_STATS is enabled
    };

    bool Compile( TaskGraphBuilder& builder, CompileInfo& compileInfo );
    void Free();
    void DisplayStats();
    TG_DEBUG_ONLY( void Print() );

    void Execute( TGExecuteData& data );

    // only to be used by tasks internally, during Execute()
    Buffer* GetBuffer( TGResourceHandle handle );
    Texture* GetTexture( TGResourceHandle handle );

private:
    void Compile_BuildResources( TaskGraphBuilder& builder, CompileInfo& compileInfo, std::vector<ResourceData>& resourceDatas );
    void Compile_MemoryAliasing( TaskGraphBuilder& builder, CompileInfo& compileInfo, std::vector<ResourceData>& resourceDatas );
    void Compile_SynchronizationAndTasks( TaskGraphBuilder& builder, CompileInfo& compileInfo );

    std::vector<Task*> m_tasks;
    std::vector<Buffer> m_buffers;
    std::vector<Texture> m_textures;
    std::vector<VmaAllocation> m_vmaAllocations;
    std::vector<std::pair<TGResourceHandle, ExtBufferFunc>> m_externalBuffers;
    std::vector<std::pair<TGResourceHandle, ExtTextureFunc>> m_externalTextures;
#if USING( TG_STATS )
    TGStats m_stats;
#endif // #if USING( TG_STATS )
};

} // namespace PG::Gfx
