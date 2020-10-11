#include "renderer/graphics_api/gpu_profiling.hpp"
#include "core/assert.hpp"
#include "core/time.hpp"
#include "renderer/r_globals.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#define MAX_NUM_QUERIES 100
#define MILLISECONDS_BETWEEN_SAMPLES 100

static uint64_t s_cpuQueries[MAX_NUM_QUERIES];
static VkQueryPool s_queryPool = VK_NULL_HANDLE;
static std::unordered_map< std::string, int > s_nameToIndexMap;
static uint32_t s_nextFreeIndex;
static double s_timestampToMillisInv;
std::string tmpFileName = "pg_profiling_log.txt";
static std::ofstream s_outputFile;
static float s_lastSampledTime;
static uint32_t s_totalSamples;

namespace PG
{
namespace Gfx
{
namespace Profile
{

    bool Init()
    {
        #if !USING( PG_GPU_PROFILING )
            return true;
        #endif // #if !USING( SHIP_BUILD )

        s_lastSampledTime = 0;
        VkQueryPoolCreateInfo createInfo = {};
        createInfo.sType        = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        createInfo.pNext        = nullptr;
        createInfo.queryType    = VK_QUERY_TYPE_TIMESTAMP;
        createInfo.queryCount   = MAX_NUM_QUERIES;
        VK_CHECK_RESULT( vkCreateQueryPool( r_globals.device.GetHandle(), &createInfo, nullptr, &s_queryPool ) );
        //PG_DEBUG_MARKER_SET_QUERY_POOL_NAME( s_queryPool, "GPU Profiling Timestamps" );

        s_timestampToMillisInv = 1.0 / r_globals.physicalDevice.GetProperties().limits.timestampPeriod / 1e6;
        s_nextFreeIndex = 0;
        s_totalSamples = 0;

        s_outputFile.open( tmpFileName );
        if ( !s_outputFile.is_open() )
        {
            return false;
        }

        return true;
    }

    void Shutdown()
    {
        if ( s_queryPool != VK_NULL_HANDLE )
        {
            vkDestroyQueryPool( r_globals.device.GetHandle(), s_queryPool, nullptr );
        }

        s_outputFile.close();

        if ( s_totalSamples == 0 )
        {
            return;
        }

        // read the log file back in and calculate average durations between timestamps with same prefix
        LOG( "Processing gpu profiling timestamps...\n" );
        std::ifstream in( tmpFileName );
        if ( !in )
        {
            LOG_ERR( "Failed to open timestamp data file\n" );
            return;
        }

        struct Entry
        {
            std::string name;
            double totalTime = 0;
            int count        = 0;
        };
        struct LogTimestamp
        {
            std::string name;
            double time;
        };

        std::unordered_map< std::string, Entry > pairings;
        std::vector< LogTimestamp > timestamps;
        std::string line;
        while ( std::getline( in, line ) )
        {
            // newline signifies end of frame. Calculate durations
            if ( line == "" )
            {
                std::sort( timestamps.begin(), timestamps.end(), []( const LogTimestamp& lhs, const LogTimestamp& rhs ) { return lhs.name < rhs.name; } );
                for ( size_t i = 0; i < timestamps.size() && i != timestamps.size() - 1; i += 2 )
                {
                    auto pos = timestamps[i].name.find( '_' );
                    std::string first_prefix  = timestamps[i].name.substr( 0, pos );
                    pos = timestamps[i + 1].name.find( '_' );
                    std::string second_prefix  = timestamps[i + 1].name.substr( 0, pos );

                    // timestamp had a start or end, but not both
                    if ( first_prefix != second_prefix )
                    {
                        --i;
                        continue;
                    }

                    std::string entryName = first_prefix;
                    auto& entry      = pairings[entryName];
                    entry.count     += 1;
                    entry.totalTime += ( timestamps[i].time - timestamps[i + 1].time );                    
                }
                timestamps.clear();

                continue;
            }

            LogTimestamp t;
            std::stringstream ss( line );
            ss >> t.name >> t.time;
            timestamps.push_back( t );
        }

        if ( pairings.size() == 0 )
        {
            return;
        }

        // display from lowest time to highest
        std::vector< Entry > entries;
        for ( auto& [ name, entry ] : pairings )
        {
            entry.name = name;
            entries.push_back( entry );
        }
        std::sort( entries.begin(), entries.end(), []( const Entry& lhs, const Entry& rhs ){ return lhs.totalTime / lhs.count < rhs.totalTime  / rhs.count; } );

        LOG( "GPU Profiling Results (ms):\n" );
        for ( const auto& entry : entries )
        {
            double time = entry.totalTime * s_timestampToMillisInv / entry.count;
            LOG( "%s: %1.3f\n", entry.name.c_str(), time );
        }
    }

    void Reset( const CommandBuffer& cmdbuf )
    {
        vkCmdResetQueryPool( cmdbuf.GetHandle(), s_queryPool, 0, MAX_NUM_QUERIES );
        s_nextFreeIndex = 0;
    }

    void GetResults()
    {
        float currentTime = Time::Time();
        if ( currentTime >= s_lastSampledTime + MILLISECONDS_BETWEEN_SAMPLES )
        {
            s_lastSampledTime = currentTime;
            VK_CHECK_RESULT( vkGetQueryPoolResults( r_globals.device.GetHandle(), s_queryPool, 0, s_nextFreeIndex, s_nextFreeIndex * sizeof( uint64_t ), s_cpuQueries, sizeof( uint64_t ), VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT ) );
        
            for ( const auto& [ name, index ] : s_nameToIndexMap )
            {
                s_outputFile << name << " " << s_cpuQueries[s_nameToIndexMap[name]] << "\n";
            }
            s_outputFile << "\n";
            s_totalSamples += s_nextFreeIndex;
        }
    }
    
    void Timestamp( const CommandBuffer& cmdbuf, const std::string& name )
    {
        s_nameToIndexMap[name] = s_nextFreeIndex++;
        vkCmdWriteTimestamp( cmdbuf.GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_queryPool, s_nameToIndexMap[name] );
    }

    uint64_t GetTimestamp( const std::string& name )
    {
        #if !USING( PG_GPU_PROFILING )
            return 0;
        #endif // #if !USING( SHIP_BUILD )
        
        PG_ASSERT( s_nameToIndexMap.find( name ) != s_nameToIndexMap.end() );
        return s_cpuQueries[s_nameToIndexMap[name]];
    }

    float GetDuration( const std::string& start, const std::string& end )
    {
        #if !USING( PG_GPU_PROFILING )
            return 0;
        #endif // #if !USING( SHIP_BUILD )
        
        return static_cast< float >( ( GetTimestamp( end ) - GetTimestamp( start ) ) * s_timestampToMillisInv );
    }

} // namespace Profile
} // namespace Gfx
} // namespace PG