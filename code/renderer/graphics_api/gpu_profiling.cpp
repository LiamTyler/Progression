#include "renderer/graphics_api/gpu_profiling.hpp"

#if !USING( PG_GPU_PROFILING )
namespace PG::Gfx::Profile
{

bool Init() { return true; }
void Shutdown() { }
void StartFrame( const CommandBuffer& cmdbuf ) { PG_UNUSED( cmdbuf ); }
void Reset( const CommandBuffer& cmdbuf ) { PG_UNUSED( cmdbuf ); }
void EndFrame() { }

void StartTimestamp( const CommandBuffer& cmdbuf, const std::string& name )
{
    PG_UNUSED( cmdbuf );
    PG_UNUSED( name );
}

void EndTimestamp( const CommandBuffer& cmdbuf, const std::string& name )
{
    PG_UNUSED( cmdbuf );
    PG_UNUSED( name );
}

double GetDuration( const std::string& start, const std::string& end )
{
    PG_UNUSED( start );
    PG_UNUSED( end );
    return 0;
}

} // namespace PG::Gfx::Profile

#else // #if !USING( PG_GPU_PROFILING )

#include "core/time.hpp"
#include "data_structures/circular_array.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/debug_ui.hpp"
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

static std::unordered_map<std::string, uint16_t> s_timestampNameToProfileRecordIndex;

struct QueryRecord
{
    uint16_t profileEntryIndex;
    uint16_t startIndex; // index into s_cpuQueries
    uint16_t endIndex;   // index into s_cpuQueries
};

#define MAX_NUM_QUERIES_PER_FRAME MAX_RECORDS
struct QueryResult
{
    uint64_t timestamp; // 64 instead of 32 because of VK_QUERY_RESULT_64_BIT
    // uint64_t available; // because of VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
};
static QueryResult s_cpuQueries[MAX_NUM_QUERIES_PER_FRAME];
static double s_timestampDifferenceToMillis;

struct PerFrameData
{
    uint16_t timestampsWritten;
    bool waitingForResults;
    VkQueryPool queryPool;
    std::unordered_map<std::string, QueryRecord> nameToQueryRecordMap;
};

static PerFrameData s_frameData[NUM_FRAME_OVERLAP];
static uint8_t s_frameIndex;

namespace PG::Gfx::Profile
{

bool Init()
{
    s_timestampDifferenceToMillis = rg.physicalDevice.GetProperties().nanosecondsPerTick / 1e6;
    s_numProfileRecords           = 0;
    for ( int i = 0; i < MAX_RECORDS; ++i )
    {
        s_profileRecords[i].min = s_profileRecords[i].max = s_profileRecords[i].avg = 0;
        s_profileRecords[i].frameHistory.Clear();
    }
    s_timestampNameToProfileRecordIndex.clear();
    s_timestampNameToProfileRecordIndex.reserve( 64 );

    VkQueryPoolCreateInfo createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.pNext                 = nullptr;
    createInfo.queryType             = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount            = MAX_NUM_QUERIES_PER_FRAME;
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        VK_CHECK( vkCreateQueryPool( rg.device.GetHandle(), &createInfo, nullptr, &s_frameData[i].queryPool ) );
        PG_DEBUG_MARKER_SET_QUERY_POOL_NAME( s_frameData[i].queryPool, "GPU Profiling Timestamp Pool " + std::to_string( i ) );
        s_frameData[i].timestampsWritten = 0;
        s_frameData[i].waitingForResults = false;
        s_frameData[i].nameToQueryRecordMap.clear();
        s_frameData[i].nameToQueryRecordMap.reserve( 64 );
    }
    s_frameIndex = 0;

    return true;
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        if ( s_frameData[i].queryPool != VK_NULL_HANDLE )
            vkDestroyQueryPool( rg.device.GetHandle(), s_frameData[i].queryPool, nullptr );
    }

    LOG( "Profiling results (ms)" );
    for ( uint16_t recordIdx = 0; recordIdx < s_numProfileRecords; ++recordIdx )
    {
        const ProfileRecord& record = s_profileRecords[recordIdx];
        LOG( "\t%s: avg = %.3f, min = %.3f, max = %.3f", record.name.c_str(), record.avg, record.min, record.max );
    }
}

void DrawResultsOnScreen()
{
#if USING( PG_DEBUG_UI )
    ImGui::SetNextWindowPos( { 5, 5 }, ImGuiCond_FirstUseEver );
    ImGui::Begin( "Profiling Stats" );

    if ( ImGui::BeginTable( "RenderPass Times", 4, ImGuiTableFlags_Borders ) )
    {
        ImGui::TableSetupColumn( "Render Pass" );
        ImGui::TableSetupColumn( "Avg (ms)" );
        ImGui::TableSetupColumn( "Min (ms)" );
        ImGui::TableSetupColumn( "Max (ms)" );
        ImGui::TableHeadersRow();

        for ( uint16_t recordIdx = 0; recordIdx < s_numProfileRecords; ++recordIdx )
        {
            ImGui::TableNextRow();
            const ProfileRecord& record = s_profileRecords[recordIdx];
            ImGui::TableSetColumnIndex( 0 );
            ImGui::Text( "%s", record.name.c_str() );
            ImGui::TableSetColumnIndex( 1 );
            ImGui::Text( "%.3f", record.avg );
            ImGui::TableSetColumnIndex( 2 );
            ImGui::Text( "%.3f", record.min );
            ImGui::TableSetColumnIndex( 3 );
            ImGui::Text( "%.3f", record.max );
        }
        ImGui::EndTable();
    }
    ImGui::End();
#endif // #if USING( PG_DEBUG_UI )
}

void StartFrame( const CommandBuffer& cmdbuf ) { Reset( cmdbuf ); }

void Reset( const CommandBuffer& cmdbuf )
{
    PerFrameData& frameData = s_frameData[s_frameIndex];
    if ( frameData.waitingForResults )
        return;

    vkCmdResetQueryPool( cmdbuf.GetHandle(), frameData.queryPool, 0, MAX_NUM_QUERIES_PER_FRAME );
    frameData.timestampsWritten = 0;
}

static void GetOldestFramesResults()
{
    PerFrameData& frameData = s_frameData[( s_frameIndex + 1 ) % NUM_FRAME_OVERLAP]; // oldest frame
    if ( frameData.timestampsWritten == 0 )
    {
        frameData.waitingForResults = false;
        return;
    }

    // VkResult res = vkGetQueryPoolResults( rg.device.GetHandle(), s_queryPool, 0, s_timestampsWritten, sizeof( s_cpuQueries ),
    // s_cpuQueries, sizeof( QueryResult ), VK_QUERY_RESULT_WITH_AVAILABILITY_BIT | VK_QUERY_RESULT_64_BIT );
    VkResult res = vkGetQueryPoolResults( rg.device.GetHandle(), frameData.queryPool, 0, frameData.timestampsWritten,
        sizeof( s_cpuQueries ), s_cpuQueries, sizeof( QueryResult ), VK_QUERY_RESULT_64_BIT );
    if ( res == VK_NOT_READY )
    {
        frameData.waitingForResults = true;
        // LOG( "Waiting for profiling results" );
        return;
    }
    VK_CHECK( res );

    // uint32_t numTimestampsAvailable = 0;
    for ( const auto& [name, qRecord] : frameData.nameToQueryRecordMap )
    {
        PG_ASSERT( qRecord.endIndex != 0, "Forgot to call StartTimestamp for timestamp '%s'", name.c_str() );
        const QueryResult& startRes = s_cpuQueries[qRecord.startIndex];
        const QueryResult& endRes   = s_cpuQueries[qRecord.endIndex];
        // numTimestampsAvailable += startRes.available != 0;
        // numTimestampsAvailable += endRes.available != 0;

        double duration       = ( endRes.timestamp - startRes.timestamp ) * s_timestampDifferenceToMillis;
        ProfileRecord& record = s_profileRecords[qRecord.profileEntryIndex];
        record.frameHistory.Pushback( duration );
        record.avg = 0;
        record.max = 0;
        record.min = FLT_MAX;
        for ( uint16_t i = 0; i < record.frameHistory.Size(); ++i )
        {
            double t   = record.frameHistory[i];
            record.max = std::max( record.max, t );
            record.min = std::min( record.min, t );
            record.avg += t;
        }
        record.avg /= record.frameHistory.Size();
    }
    // PG_ASSERT( numTimestampsAvailable == 2 * s_nameToQueryRecordMap.size(), "Not all timestamps were availble. Was unsure if this could
    // happen or not" );

    frameData.nameToQueryRecordMap.clear();
    frameData.timestampsWritten = 0;
    frameData.waitingForResults = false;
}

void EndFrame()
{
    s_frameData[s_frameIndex].waitingForResults = true;
    GetOldestFramesResults();
    s_frameIndex = ( s_frameIndex + 1 ) % NUM_FRAME_OVERLAP;
}

static uint16_t GetProfileEntryIndex( const std::string& name )
{
    auto it = s_timestampNameToProfileRecordIndex.find( name );
    if ( it == s_timestampNameToProfileRecordIndex.end() )
    {
        uint16_t idx                              = s_numProfileRecords++;
        s_profileRecords[idx].name                = name;
        s_timestampNameToProfileRecordIndex[name] = idx;
        return idx;
    }
    else
    {
        return it->second;
    }
}

void StartTimestamp( const CommandBuffer& cmdbuf, const std::string& name )
{
    PerFrameData& frameData = s_frameData[s_frameIndex];
    if ( frameData.waitingForResults )
        return;

    PG_ASSERT( frameData.nameToQueryRecordMap.find( name ) == frameData.nameToQueryRecordMap.end(),
        "Calling StartTimestamp multiple times for tag '%s'", name.c_str() );
    frameData.nameToQueryRecordMap[name] = { GetProfileEntryIndex( name ), frameData.timestampsWritten, 0 };
    uint32_t vkQueryVal                  = frameData.timestampsWritten++;
    vkCmdWriteTimestamp( cmdbuf.GetHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameData.queryPool, vkQueryVal );
}

void EndTimestamp( const CommandBuffer& cmdbuf, const std::string& name )
{
    PerFrameData& frameData = s_frameData[s_frameIndex];
    if ( frameData.waitingForResults )
        return;

    PG_ASSERT( frameData.nameToQueryRecordMap.find( name ) != frameData.nameToQueryRecordMap.end(),
        "Forgot to call StartTimestamp for timestamp '%s'", name.c_str() );
    frameData.nameToQueryRecordMap[name].endIndex = frameData.timestampsWritten;
    uint32_t vkQueryVal                           = frameData.timestampsWritten++;
    vkCmdWriteTimestamp( cmdbuf.GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameData.queryPool, vkQueryVal );
}

} // PG::Gfx::Profile

#endif // #else // #if !USING( PG_GPU_PROFILING )
