#include "r_taskgraph.hpp"
#include "renderer/r_globals.hpp"
#include <unordered_map>


namespace PG
{
namespace Gfx
{

    TG_ResourceDesc::TG_ResourceDesc( ResourceType inType, PixelFormat inFormat, uint32_t inWidth, uint32_t inHeight, uint32_t inDepth, uint32_t inArrayLayers, uint32_t inMipLevels, const glm::vec4& inClearColor, bool inCleared ) :
        type( inType ), format( inFormat ), width( inWidth ), height( inHeight ), depth( inDepth ), arrayLayers( inArrayLayers ), mipLevels( inMipLevels ), clearColor( inClearColor ), isCleared( inCleared )
    {
    }


    void TG_ResourceDesc::ResolveSizes( uint16_t sceneWidth, uint16_t sceneHeight )
    {
        if ( (width & SCENE_WIDTH()) == SCENE_WIDTH() )
        {
            uint32_t divisor = width >> 16;
            width = divisor ? sceneWidth / divisor : sceneWidth;
        }
        if ( (height & SCENE_HEIGHT()) == SCENE_HEIGHT() )
        {
            uint32_t divisor = height >> 16;
            height = divisor ? sceneHeight / divisor : sceneHeight;
        }
        if ( mipLevels == AUTO_FULL_MIP_CHAIN() )
        {
            mipLevels = 1 + static_cast< int >( log2f( static_cast< float >( std::max( width, height ) ) ) );
        }
    }


    bool TG_ResourceDesc::operator==( const TG_ResourceDesc& d ) const
    {
        return format == d.format && width == d.width && height == d.height && depth == d.depth &&
            arrayLayers == d.arrayLayers && mipLevels == d.mipLevels && clearColor == d.clearColor;
    }


    bool TG_ResourceDesc::Mergable( const TG_ResourceDesc& d ) const
    {
        return type == d.type && format == d.format && width == d.width && height == d.height && depth == d.depth && arrayLayers == d.arrayLayers && mipLevels == d.mipLevels;
    }


    RenderTaskBuilder::RenderTaskBuilder( const std::string& inName ) : name( inName )
    {
    }


    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::COLOR_ATTACH, format, width, height, depth, arrayLayers, mipLevels, clearColor, true ) } );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::COLOR_ATTACH, format, width, height, depth, arrayLayers, mipLevels, glm::vec4( 0 ), false ) } );
    }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, float clearValue )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::DEPTH_ATTACH, format, width, height, 1, 1, 1, glm::vec4( clearValue ), true ) } );
    }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::DEPTH_ATTACH, format, width, height, 1, 1, 1, glm::vec4( 0 ), false ) } );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::TEXTURE, format, width, height, depth, arrayLayers, mipLevels, clearColor, true ) } );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::TEXTURE, format, width, height, depth, arrayLayers, mipLevels, glm::vec4( 0 ), false ) } );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name )        { AddColorOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ) ); }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name )        { AddDepthOutput( name, PixelFormat::INVALID, 0, 0, 0 ); }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name )      { AddTextureOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ) ); }
    void RenderTaskBuilder::AddTextureInput( const std::string& name )       { inputs.push_back( { name } ); }
    void RenderTaskBuilder::AddTextureInputOutput( const std::string& name ) { inputs.push_back( { name } ); }


    GraphResource::GraphResource( const std::string& inName, const TG_ResourceDesc& inDesc, uint16_t currentTask ) :
        name( inName ), desc( inDesc ), firstTask( currentTask ), lastTask( currentTask ), physicalResourceIndex( USHRT_MAX )
    {
    }

    bool GraphResource::Mergable( const GraphResource& res ) const
    {
        bool overlapForward  = res.lastTask >= firstTask && res.firstTask <= lastTask;
        bool overlapBackward = lastTask >= res.firstTask && firstTask <= res.lastTask;
        if ( overlapForward || overlapBackward )
        {
            return false;
        }

        return desc.Mergable( res.desc );
    }


    RenderGraph::RenderGraph()
    {
        buildRenderTasks.reserve( 64 );
    }


    RenderTaskBuilder* RenderGraph::AddTask( const std::string& name )
    {
        size_t index = buildRenderTasks.size();
        buildRenderTasks.push_back( name );
        return &buildRenderTasks[index];
    }

    bool RenderGraph::Compile( uint16_t sceneWidth, uint16_t sceneHeight )
    {
        // TODO: re-order tasks for more optimal rendering. For now, keep same order

        // TODO: cull tasks that dont affect the final image
        assert( buildRenderTasks.size() <= MAX_FINAL_TASKS );

        numRenderTasks = 0;
        numPhysicalResources = 0;
        numLogicalOutputs = 0;

        // find resource lifetimes
        std::vector< GraphResource > graphOutputs;
        std::unordered_map< std::string, uint16_t > outputNameToLogicalMap;

        uint16_t numBuildTasks = static_cast< uint16_t >( buildRenderTasks.size() );
        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTaskBuilder& task = buildRenderTasks[taskIndex];
            for ( TG_ResourceOutput& output : task.outputs )
            {
                const std::string& name = output.name;
                if ( output.desc.format != PixelFormat::INVALID )
                {
                    if ( outputNameToLogicalMap.find( name ) != outputNameToLogicalMap.end() )
                    {
                        printf( "Duplicate create params for output '%s', only should specify them once\n", name.c_str() );
                        return false;
                    }

                    output.createInfoIdx = static_cast< uint16_t >( graphOutputs.size() );
                    outputNameToLogicalMap[name] = static_cast< uint16_t >( graphOutputs.size() );
                    GraphResource res( name, output.desc, taskIndex );
                    res.desc.ResolveSizes( sceneWidth, sceneHeight );
                    graphOutputs.push_back( res );
                }
                else if ( name == "BACK_BUFFER" )
                {
                    output.createInfoIdx = static_cast< uint16_t >( graphOutputs.size() );
                    outputNameToLogicalMap[name] = static_cast< uint16_t >( graphOutputs.size() );
                    GraphResource res( name, output.desc, taskIndex );
                    graphOutputs.push_back( res );
                }
                else
                {
                    auto it = outputNameToLogicalMap.find( name );
                    if ( it == outputNameToLogicalMap.end() )
                    {
                        printf( "Output '%s' used without any create params specified in this or previous tasks\n", name.c_str() );
                        return false;
                    }
                    graphOutputs[it->second].lastTask = taskIndex;
                    output.createInfoIdx = it->second;
                }
            }
        }

        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTaskBuilder& task = buildRenderTasks[taskIndex];
            for ( auto& input : task.inputs )
            {
                const std::string& name = input.name;
                auto it = outputNameToLogicalMap.find( name );
                // todo: external textures
                if ( it == outputNameToLogicalMap.end() )
                {
                    printf( "Resource '%s' used as input, but no tasks generate it as an output\n", name.c_str() );
                    return false;
                }
                input.createInfoIdx = it->second;
                GraphResource& res = graphOutputs[it->second];
                if ( res.firstTask >= taskIndex )
                {
                    printf( "Resource '%s' used as input before it created as an output. This means the task ordering is bad\n", name.c_str() );
                    return false;
                }
                res.lastTask = std::max( res.lastTask, taskIndex );
            }
        }

        // select merge-able resources. Currently only allowing if the desc is the same size, format, etc.
        // Eventually would be nice to allow smaller images to use the same memory of a larger one that isn't in use, or similar formats, etc
        for ( size_t i = 0; i < graphOutputs.size(); ++i )
        {
            GraphResource& logicalRes = graphOutputs[i];
            for ( uint16_t physicalIdx = 0; physicalIdx < numPhysicalResources; ++physicalIdx )
            {
                GraphResource& physicalRes = physicalResources[physicalIdx];
                if ( physicalRes.Mergable( logicalRes ) )
                {
                    physicalRes.firstTask = std::min( physicalRes.firstTask, logicalRes.firstTask );
                    physicalRes.lastTask = std::max( physicalRes.lastTask, logicalRes.lastTask );
                    logicalRes.physicalResourceIndex = physicalIdx;
                    physicalRes.name += "_" + logicalRes.name;
                    break;
                }
            }

            if ( logicalRes.physicalResourceIndex == USHRT_MAX )
            {
                logicalRes.physicalResourceIndex = numPhysicalResources;
                physicalResources[numPhysicalResources] = logicalRes;
                ++numPhysicalResources;
                assert( numPhysicalResources <= MAX_PHYSICAL_RESOURCES );
            }
        }

        ImageLayout currentImageLayouts[MAX_PHYSICAL_RESOURCES] = { ImageLayout::UNDEFINED };
        int lastOutputTask[MAX_PHYSICAL_RESOURCES] = { -1 };
        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTask& finalTask = renderTasks[numRenderTasks];
            RenderTaskBuilder& buildTask = buildRenderTasks[taskIndex];
            RenderPassDescriptor renderPassDesc;

            finalTask.name = std::move( buildTask.name );
            finalTask.numInputs = 0;
            for ( const TG_ResourceInput& input : buildTask.inputs )
            {
                uint16_t physicalIdx = graphOutputs[input.createInfoIdx].physicalResourceIndex;
                finalTask.inputIndices[finalTask.numInputs] = physicalIdx;
                ++finalTask.numInputs;

                // check to see if the final layout of previous renderpass needs to be adjusted
                if ( currentImageLayouts[physicalIdx] == ImageLayout::COLOR_ATTACHMENT_OPTIMAL )
                {

                }
            }

            finalTask.numOutputs = 0;
            for ( const TG_ResourceOutput& output : buildTask.outputs )
            {
                uint16_t physicalIdx = graphOutputs[output.createInfoIdx].physicalResourceIndex;
                finalTask.outputIndices[finalTask.numOutputs] = physicalIdx;
                ++finalTask.numOutputs;

                LoadAction loadOp = LoadAction::LOAD;
                if ( output.desc.isCleared )
                {
                    loadOp = LoadAction::CLEAR;
                }
                else if ( currentImageLayouts[physicalIdx] == ImageLayout::UNDEFINED )
                {
                    loadOp = LoadAction::DONT_CARE;
                }

                // guess at the final layout, w
                if ( output.desc.type == ResourceType::COLOR_ATTACH )
                {
                    renderPassDesc.AddColorAttachment( output.desc.format, loadOp, StoreAction::STORE, output.desc.clearColor, currentImageLayouts[physicalIdx], ImageLayout::COLOR_ATTACHMENT_OPTIMAL );
                    currentImageLayouts[physicalIdx] = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
                }
                else if ( output.desc.type == ResourceType::DEPTH_ATTACH )
                {
                    renderPassDesc.AddDepthAttachment( output.desc.format, loadOp, StoreAction::STORE, output.desc.clearColor[0], currentImageLayouts[physicalIdx], ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
                    currentImageLayouts[physicalIdx] = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
            }

            ++numRenderTasks;
        }
        numLogicalOutputs = static_cast< uint16_t >( graphOutputs.size() );

        if ( !AllocatePhysicalResources() )
        {
            return false;
        }

        return true;
    }


    void RenderGraph::Free()
    {
        for ( uint16_t i = 0; i < numPhysicalResources; ++i )
        {
            if ( textures[i] )
            {
                textures[i].Free();
            }
        }
    }


    void RenderGraph::PrintTaskGraph() const
    {
        printf( "Unique Outputs: %u\n", numLogicalOutputs );
        printf( "Physical Resources: %u\n", numPhysicalResources );
        for ( uint16_t i = 0; i < numPhysicalResources; ++i )
        {
            const auto& res = physicalResources[i];
            printf( "\tPhysical resource[%u]: '%s'\n", i, res.name.c_str() );
            printf( "\t\tUsed in tasks: %u - %u (%s - %s)\n", res.firstTask, res.lastTask, renderTasks[res.firstTask].name.c_str(), renderTasks[res.lastTask].name.c_str() );
            printf( "\t\tformat: %s, width: %u, height: %u, depth: %u, arrayLayers: %u, mipLevels: %u\n", PixelFormatName( res.desc.format ).c_str(), res.desc.width, res.desc.height, res.desc.depth, res.desc.arrayLayers, res.desc.mipLevels );
        }

        printf( "Tasks: %u\n", numRenderTasks );
        for ( uint16_t taskIdx = 0; taskIdx < numRenderTasks; ++taskIdx )
        {
            const auto& task = renderTasks[taskIdx];
            printf( "Task[%u]: %s\n", taskIdx, task.name.c_str() );
            for ( uint8_t i = 0; i < task.numInputs; ++i )
            {
                printf( "\tInput[%u]: %s\n", i, physicalResources[task.inputIndices[i]].name.c_str() );
            }
            for ( uint8_t i = 0; i < task.numOutputs; ++i )
            {
                printf( "\tOutput[%u]: %s\n", i, physicalResources[task.outputIndices[i]].name.c_str() );
            }
        }
    }


    bool RenderGraph::AllocatePhysicalResources()
    {
        bool ret = true;
        for ( uint16_t i = 0; i < numPhysicalResources; ++i )
        {
            GraphResource& resource = physicalResources[i];
            if ( resource.name == "BACK_BUFFER" )
            {
                continue;
            }

            TextureDescriptor texDesc;
            texDesc.format = resource.desc.format;
            texDesc.width = resource.desc.width;
            texDesc.height = resource.desc.height;
            texDesc.depth = resource.desc.depth;
            texDesc.arrayLayers = resource.desc.arrayLayers;
            texDesc.mipLevels = resource.desc.mipLevels;
            texDesc.addToBindlessArray = true;
            texDesc.type = ImageType::TYPE_2D;
            texDesc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            if ( resource.desc.type == ResourceType::COLOR_ATTACH )
            {
                texDesc.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            else if ( resource.desc.type == ResourceType::DEPTH_ATTACH )
            {
                texDesc.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            textures[i] = r_globals.device.NewTexture( texDesc, resource.name );
            ret = ret && textures[i];
        }

        return ret;
    }


    bool RenderGraph::CreateRenderPasses()
    {
        //RenderPassDescriptor renderPassDesc;
        //renderPassDesc.AddColorAttachment( PixelFormat::R16_G16_B16_A16_FLOAT, LoadAction::LOAD, StoreAction::STORE, glm::vec4( 0 ), ImageLayout::COLOR_ATTACHMENT_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL );
        //renderPassDesc.AddDepthAttachment( PixelFormat::DEPTH_32_FLOAT, LoadAction::LOAD, StoreAction::STORE, 1.0f, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
        //s_renderPasses[GFX_RENDER_PASS_SKYBOX] = r_globals.device.NewRenderPass( renderPassDesc, "Skybox" );
        //
        //VkImageView attachments[] = { r_globals.colorTex.GetView(), r_globals.depthTex.GetView() };
        //VkFramebufferCreateInfo framebufferInfo = {};
        //framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        //framebufferInfo.renderPass      = s_renderPasses[GFX_RENDER_PASS_SKYBOX].GetHandle();
        //framebufferInfo.attachmentCount = 2;
        //framebufferInfo.pAttachments    = attachments;
        //framebufferInfo.width           = r_globals.swapchain.GetWidth();
        //framebufferInfo.height          = r_globals.swapchain.GetHeight();
        //framebufferInfo.layers          = 1;
        //s_framebuffers[GFX_RENDER_PASS_SKYBOX] = r_globals.device.NewFramebuffer( framebufferInfo, "Skybox" );
        //
        //return s_renderPasses[GFX_RENDER_PASS_SKYBOX] && s_framebuffers[GFX_RENDER_PASS_SKYBOX];

        for ( uint16_t taskIdx = 0; taskIdx < numRenderTasks; ++taskIdx )
        {
            RenderTask& task = renderTasks[taskIdx];
            RenderPassDescriptor renderPassDesc;

            VkImageView attachments[RenderTask::MAX_OUTPUTS];
            for ( uint8_t outputIdx = 0; outputIdx < task.numOutputs; ++outputIdx )
            {
                const GraphResource& res = physicalResources[task.outputIndices[outputIdx]];
                if ( res.desc.type == ResourceType::COLOR_ATTACH )
                {
                    renderPassDesc.AddColorAttachment( res.desc.format, LoadAction::LOAD, StoreAction::STORE, glm::vec4( 0 ), ImageLayout::COLOR_ATTACHMENT_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL );
                }
            }
        }
    }

} // namespace Gfx
} // namespace PG
