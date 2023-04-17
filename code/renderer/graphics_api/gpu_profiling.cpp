#include "renderer/graphics_api/gpu_profiling.hpp"

#if !USING( PG_GPU_PROFILING )
namespace PG::Gfx::Profile
{
    bool Init() { return true; }
    void Shutdown() {}

    void Reset( const CommandBuffer& cmdbuf ) { PG_UNUSED( cmdbuf ); }
    void GetResults() {}
    
    void StartTimestamp( const CommandBuffer& cmdbuf, const std::string& name ) { PG_UNUSED( cmdbuf ); PG_UNUSED( name ); }
    void EndTimestamp( const CommandBuffer& cmdbuf, const std::string& name ) { PG_UNUSED( cmdbuf ); PG_UNUSED( name ); }
    double GetDuration( const std::string& start, const std::string& end ) { PG_UNUSED( start ); PG_UNUSED( end ); return 0; }
} // namespace PG::Gfx::Profile

#else // #if !USING( PG_GPU_PROFILING )

#include "core/time.hpp"
#include "data_structures/circular_array.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#define MAX_RECORDS 64
#define NUM_HISTORY_FRAMES 60

struct ProfileRecord
{
    std::string name;
    // all timings are in milliseconds
    double min;
    double max;
    double avg;
    CircularArray<double, NUM_HISTORY_FRAMES> frameHistory;
};
static ProfileRecord s_profileRecords[MAX_RECORDS];
static uint16_t s_numProfileRecords;

struct QueryRecord
{
    uint16_t profileEntryIndex;
    uint16_t startIndex; // indices into s_cpuQueries
    uint16_t endIndex;
};

static std::unordered_map<std::string, QueryRecord> s_nameToQueryRecordMap;


#define MAX_NUM_QUERIES_PER_FRAME MAX_RECORDS
struct QueryResult
{
    uint64_t timestamp; // 64 instead of 32 because of VK_QUERY_RESULT_64_BIT
    //uint64_t available; // because of VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
};
static QueryResult s_cpuQueries[MAX_NUM_QUERIES_PER_FRAME];
static VkQueryPool s_queryPool = VK_NULL_HANDLE;
static uint16_t s_timestampsWritten;
static double s_timestampDifferenceToMillis;
static bool s_waitingForResults;


namespace PG::Gfx::Profile
{

    bool Init()
    {
        VkQueryPoolCreateInfo createInfo = {};
        createInfo.sType        = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        createInfo.pNext        = nullptr;
        createInfo.queryType    = VK_QUERY_TYPE_TIMESTAMP;
        createInfo.queryCount   = MAX_NUM_QUERIES_PER_FRAME;
        VK_CHECK_RESULT( vkCreateQueryPool( r_globals.device.GetHandle(), &createInfo, nullptr, &s_queryPool ) );
        PG_DEBUG_MARKER_SET_QUERY_POOL_NAME( s_queryPool, "GPU Profiling Timestamps" );

        s_waitingForResults = false;
        s_timestampDifferenceToMillis = r_globals.physicalDevice.GetProperties().nanosecondsPerTick / 1e6;
        s_timestampsWritten = 0;
        s_numProfileRecords = 0;
        for ( int i = 0; i < MAX_RECORDS; ++i )
        {
            s_profileRecords[i].min = s_profileRecords[i].max = s_profileRecords[i].avg = 0;
            s_profileRecords[i].frameHistory.Clear();
        }

        return true;
    }

    void Shutdown()
    {
        if ( s_queryPool != VK_NULL_HANDLE )
            vkDestroyQueryPool( r_globals.device.GetHandle(), s_queryPool, nullptr );

        LOG( "Profiling results (ms)" );
        for ( uint16_t recordIdx = 0; recordIdx < s_numProfileRecords; ++recordIdx )
        {
            const ProfileRecord& record = s_profileRecords[recordIdx];
            LOG( "\t%s: avg = %.3f, min = %.3f, max = %.3f", record.name.c_str(), record.avg, record.min, record.max );
        }
    }

    void Reset( const CommandBuffer& cmdbuf )
    {
        if ( s_waitingForResults )
            return;

        vkCmdResetQueryPool( cmdbuf.GetHandle(), s_queryPool, 0, MAX_NUM_QUERIES_PER_FRAME );
        s_timestampsWritten = 0;
    }

    void GetResults()
    {
        float currentTime = Time::Time();
        //VkResult res = vkGetQueryPoolResults( r_globals.device.GetHandle(), s_queryPool, 0, s_timestampsWritten, sizeof( s_cpuQueries ), s_cpuQueries, sizeof( QueryResult ), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT | VK_QUERY_RESULT_64_BIT );
        VkResult res = vkGetQueryPoolResults( r_globals.device.GetHandle(), s_queryPool, 0, s_timestampsWritten, sizeof( s_cpuQueries ), s_cpuQueries, sizeof( QueryResult ), VK_QUERY_RESULT_64_BIT );
        if ( res == VK_NOT_READY )
        {
            s_waitingForResults = true;
            LOG( "Waiting for profiling results" ); // TODO: remove log. Just want to see how often this happens. Probably 0 right now, because of idling at end of frame
            return;
        }
        VK_CHECK_RESULT( res );

        //uint32_t numTimestampsAvailable = 0;
        for ( const auto& [ name, qRecord ] : s_nameToQueryRecordMap )
        {
            PG_ASSERT( qRecord.endIndex != 0, "Forgot to call StartTimestamp for timestamp '" + name + "'" );
            const QueryResult& startRes = s_cpuQueries[qRecord.startIndex];
            const QueryResult& endRes = s_cpuQueries[qRecord.endIndex];
            //numTimestampsAvailable += startRes.available != 0;
            //numTimestampsAvailable += endRes.available != 0;

            double duration = (endRes.timestamp - startRes.timestamp) * s_timestampDifferenceToMillis;
            ProfileRecord& record = s_profileRecords[qRecord.profileEntryIndex];
            record.frameHistory.Pushback( duration );
            record.avg = 0;
            record.max = 0;
            record.min = FLT_MAX;
            for ( int i = 0; i < record.frameHistory.Size(); ++i )
            {
                double t = record.frameHistory[i];
                record.max = std::max( record.max, t );
                record.min = std::min( record.min, t );
                record.avg += t;
            }
            record.avg /= record.frameHistory.Size();
        }
        //PG_ASSERT( numTimestampsAvailable == 2 * s_nameToQueryRecordMap.size(), "Not all timestamps were availble. Was unsure if this could happen or not" );

        s_timestampsWritten = 0;
        s_waitingForResults = false;
    }
    
    void StartTimestamp( const CommandBuffer& cmdbuf, const std::string& name )
    {
        if ( s_waitingForResults )
            return;

        uint32_t vkQueryVal = s_timestampsWritten;
        auto it = s_nameToQueryRecordMap.find( name );
        if ( it == s_nameToQueryRecordMap.end() )
        {
            s_nameToQueryRecordMap[name] = { s_numProfileRecords, s_timestampsWritten, 0 };
            s_profileRecords[s_numProfileRecords].name = name;
            ++s_numProfileRecords;
        }
        else
        {
            it->second.startIndex = s_timestampsWritten;
            it->second.endIndex = 0;
        }
        ++s_timestampsWritten;
        vkCmdWriteTimestamp( cmdbuf.GetHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, s_queryPool, vkQueryVal );
    }

    void EndTimestamp( const CommandBuffer& cmdbuf, const std::string& name )
    {
        if ( s_waitingForResults )
            return;

        PG_ASSERT( s_nameToQueryRecordMap.find( name ) != s_nameToQueryRecordMap.end(), "Forgot to call StartTimestamp for timestamp '" + name + "'" );
        uint32_t vkQueryVal = s_timestampsWritten;
        s_nameToQueryRecordMap[name].endIndex = s_timestampsWritten;
        ++s_timestampsWritten;
        vkCmdWriteTimestamp( cmdbuf.GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_queryPool, vkQueryVal );
    }

    double GetDuration( const std::string& start, const std::string& end )
    {
        //PG_ASSERT( s_nameToIndexMap.find( name ) != s_nameToIndexMap.end() );
        //const QueryRecord& result = s_cpuQueries[s_nameToIndexMap[name]];
        //
        //return static_cast< float >( ( GetTimestamp( end ) - GetTimestamp( start ) ) * s_timestampToMillisInv );
        return 0;
    }

} // PG::Gfx::Profile

#endif // #else // #if !USING( PG_GPU_PROFILING )