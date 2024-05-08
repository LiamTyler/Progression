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

struct TGExecuteData
{
    Scene* scene;
    FrameData* frameData;
    CommandBuffer* cmdBuf;
    TaskGraph* taskGraph;

    std::vector<VkImageMemoryBarrier2> scratchImageBarriers;
    std::vector<VkBufferMemoryBarrier2> scratchBufferBarriers;
    std::vector<VkRenderingAttachmentInfo> scratchAttachmentInfos;
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
        bool showStats      = true;
    };

    bool Compile( TaskGraphBuilder& builder, CompileInfo& compileInfo );
    void Free();

    void Execute( TGExecuteData& data );

    // only to be used by tasks internally, during Execute()
    Buffer* GetBuffer( TGResourceHandle handle );
    Texture* GetTexture( TGResourceHandle handle );

private:
    void Compile_BuildResources( TaskGraphBuilder& builder, CompileInfo& compileInfo, std::vector<ResourceData>& resourceDatas );
    void Compile_MemoryAliasing( TaskGraphBuilder& builder, CompileInfo& compileInfo, std::vector<ResourceData>& resourceDatas );
    void Compile_SynchronizationAndTasks( TaskGraphBuilder& builder, CompileInfo& compileInfo );

    std::vector<Task*> tasks;
    std::vector<Buffer> buffers;
    std::vector<Texture> textures;
    std::vector<VmaAllocation> vmaAllocations;
    std::vector<std::pair<TGResourceHandle, ExtBufferFunc>> externalBuffers;
    std::vector<std::pair<TGResourceHandle, ExtTextureFunc>> externalTextures;
};

} // namespace PG::Gfx
