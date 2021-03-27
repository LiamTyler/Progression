#include "r_taskgraph.hpp"
#include <unordered_map>

namespace PG
{
namespace Gfx
{

    TG_ResourceDesc::TG_ResourceDesc( PixelFormat inFormat, uint32_t inWidth, uint32_t inHeight, uint32_t inDepth, uint32_t inArrayLayers, uint32_t inMipLevels, const glm::vec4& inClearColor ) :
        format( inFormat ), width( inWidth ), height( inHeight ), depth( inDepth ), arrayLayers( inArrayLayers ), mipLevels( inMipLevels ), clearColor( inClearColor )
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
        return format == d.format && width == d.width && height == d.height && depth == d.depth && arrayLayers == d.arrayLayers && mipLevels == d.mipLevels;
    }


    TG_ResourceInput::TG_ResourceInput( ResourceType _type, const std::string& _name ) : type( _type ), name( _name ) {}


    RenderTaskBuilder::RenderTaskBuilder( const std::string& inName ) : name( inName )
    {
    }

    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        TG_ResourceOutput output = { ResourceType::COLOR_ATTACH, name, { format, width, height, depth, arrayLayers, mipLevels, clearColor } };
        outputs.emplace_back( output );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        TG_ResourceOutput output = { ResourceType::TEXTURE, name, { format, width, height, depth, arrayLayers, mipLevels, clearColor } };
        outputs.emplace_back( output );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name )        { AddColorOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ) ); }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name )      { AddTextureOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ) ); }
    void RenderTaskBuilder::AddTextureInput( const std::string& name )       { inputs.emplace_back( ResourceType::TEXTURE, name ); }
    void RenderTaskBuilder::AddTextureInputOutput( const std::string& name ) { inputs.emplace_back( ResourceType::TEXTURE, name ); }


    bool PhysicalGraphResource::Mergable( const PhysicalGraphResource* res ) const
    {
        bool overlapForward  = res->lastTask >= firstTask && res->firstTask <= lastTask;
        bool overlapBackward = lastTask >= res->firstTask && firstTask <= res->lastTask;
        if ( overlapForward || overlapBackward )
        {
            return false;
        }

        return desc.Mergable( res->desc );
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


    struct LogicalGraphResource : public PhysicalGraphResource
    {
        uint16_t physicalResourceIndex = USHRT_MAX;
    };


    bool RenderGraph::Compile( uint16_t sceneWidth, uint16_t sceneHeight )
    {
        // TODO: re-order tasks for more optimal rendering. For now, keep same order

        // TODO: cull tasks that dont affect the final image
        assert( buildRenderTasks.size() <= MAX_FINAL_TASKS );

        numRenderTasks = 0;
        numPhysicalResources = 0;
        numLogicalOutputs = 0;

        // find resource lifetimes
        std::vector< LogicalGraphResource > graphOutputs;
        std::unordered_map< std::string, uint16_t > outputNameToLogicalMap;
        uint16_t numBuildTasks = static_cast< uint16_t >( buildRenderTasks.size() );
        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTaskBuilder& task = buildRenderTasks[taskIndex];
            for ( auto& output : task.outputs )
            {
                const std::string& name = output.name;
                if ( output.desc.format != PixelFormat::INVALID )
                {
                    if ( outputNameToLogicalMap.find( name ) != outputNameToLogicalMap.end() )
                    {
                        printf( "Duplicate create params for output '%s', only should specify them once\n", name.c_str() );
                        return false;
                    }

                    output.physicalResourceIndex = static_cast< uint16_t >( graphOutputs.size() );
                    outputNameToLogicalMap[name] = static_cast< uint16_t >( graphOutputs.size() );
                    LogicalGraphResource res;
                    res.name = name;
                    res.desc = output.desc;
                    res.desc.ResolveSizes( sceneWidth, sceneHeight );
                    res.firstTask = taskIndex;
                    res.lastTask = taskIndex;
                    res.physicalResourceIndex = USHRT_MAX;
                    graphOutputs.push_back( res );
                    ++numLogicalOutputs;
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
                    output.physicalResourceIndex = it->second;
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
                input.physicalResourceIndex = it->second;
                LogicalGraphResource& res = graphOutputs[it->second];
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
            LogicalGraphResource& logicalRes = graphOutputs[i];
            for ( uint16_t physicalIdx = 0; physicalIdx < numPhysicalResources; ++physicalIdx )
            {
                PhysicalGraphResource& physicalRes = physicalResources[physicalIdx];
                if ( physicalRes.Mergable( &logicalRes ) )
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
                PhysicalGraphResource newRes = *static_cast< PhysicalGraphResource* >( &logicalRes );
                physicalResources[numPhysicalResources] = newRes;
                assert( numPhysicalResources <= MAX_PHYSICAL_RESOURCES );
                ++numPhysicalResources;
            }
        }

        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTask& finalTask = renderTasks[numRenderTasks];
            RenderTaskBuilder& buildTask = buildRenderTasks[taskIndex];

            finalTask.name       = std::move( buildTask.name );
            finalTask.numInputs  = 0;
            for ( auto& input : buildTask.inputs )
            {
                uint16_t physicalIdx = graphOutputs[input.physicalResourceIndex].physicalResourceIndex;
                finalTask.inputIndices[finalTask.numInputs] = physicalIdx;
                ++finalTask.numInputs;
            }

            finalTask.numOutputs = 0;
            for ( auto& output : buildTask.outputs )
            {
                uint16_t physicalIdx = graphOutputs[output.physicalResourceIndex].physicalResourceIndex;
                finalTask.outputIndices[finalTask.numOutputs] = physicalIdx;
                ++finalTask.numOutputs;
            }

            ++numRenderTasks;
        }

        return true;
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
            printf( "\t\tformat: %u, width: %u, height: %u, depth: %u, arrayLayers: %u, mipLevels: %u\n", res.desc.format, res.desc.width, res.desc.height, res.desc.depth, res.desc.arrayLayers, res.desc.mipLevels );
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

} // namespace Gfx
} // namespace PG
