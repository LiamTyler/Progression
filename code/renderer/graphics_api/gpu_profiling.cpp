#include "renderer/graphics_api/gpu_profiling.hpp"

#if !USING( PG_GPU_PROFILING )
namespace PG::Gfx::Profile
{

void Init() {}
void Shutdown() {}
void DrawResultsOnScreen() {}

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

#define MAX_RECORDS_PER_FRAME 32
#define MAX_TIMESTAMPS_PER_FRAME ( 2 * MAX_RECORDS_PER_FRAME )
#define NUM_HISTORY_FRAMES 63

struct ProfileRecord
{
    std::string name;
    // all timings are in milliseconds
    float min = 0;
    float max = 0;
    float avg = 0;
};

struct ProfileRecordHistory
{
    CircularArray<float, NUM_HISTORY_FRAMES> frameHistory;
};
static std::vector<ProfileRecord> s_profileRecords;
static std::vector<ProfileRecordHistory> s_profileRecordHistories;
static std::unordered_map<std::string, uint16_t> s_profileNameToIndexMap;

struct QueryResult
{
    uint64_t timestamp;
};

static QueryResult s_cpuQueries[MAX_TIMESTAMPS_PER_FRAME];
static double s_timestampDifferenceToMillis;

namespace PG::Gfx::Profile
{

struct PerFrameData
{
    VkQueryPool queryPool;
    std::vector<QueryRecord> queries;
};
static PerFrameData s_frameData[NUM_FRAME_OVERLAP];

static PerFrameData& GetCurrentFrameData() { return s_frameData[rg.currentFrameIdx]; }

void Init()
{
    s_timestampDifferenceToMillis = rg.physicalDevice.GetProperties().nanosecondsPerTick / 1e6;
    s_profileRecords.reserve( 2 * MAX_RECORDS_PER_FRAME );
    s_profileRecordHistories.reserve( 2 * MAX_RECORDS_PER_FRAME );
    s_profileNameToIndexMap.reserve( 2 * MAX_RECORDS_PER_FRAME );

    VkQueryPoolCreateInfo createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.pNext                 = nullptr;
    createInfo.queryType             = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount            = MAX_TIMESTAMPS_PER_FRAME;
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        VK_CHECK( vkCreateQueryPool( rg.device, &createInfo, nullptr, &s_frameData[i].queryPool ) );
        PG_DEBUG_MARKER_SET_QUERY_POOL_NAME( s_frameData[i].queryPool, "GPU Profiling Timestamp Pool " + std::to_string( i ) );
        s_frameData[i].queries.reserve( MAX_RECORDS_PER_FRAME );
    }
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        if ( s_frameData[i].queryPool != VK_NULL_HANDLE )
            vkDestroyQueryPool( rg.device, s_frameData[i].queryPool, nullptr );
    }

    LOG( "Profiling results (ms)" );
    for ( size_t recordIdx = 0; recordIdx < s_profileRecords.size(); ++recordIdx )
    {
        const ProfileRecord& record = s_profileRecords[recordIdx];
        LOG( "\t%s: avg = %.3f, min = %.3f, max = %.3f", record.name.data(), record.avg, record.min, record.max );
    }
}

void DrawResultsOnScreen()
{
#if USING( PG_DEBUG_UI )
    ImGui::SetNextWindowPos( { 5, 5 }, ImGuiCond_FirstUseEver );
    ImGui::Begin( "Profiling Stats", NULL, ImGuiWindowFlags_NoFocusOnAppearing );

    if ( ImGui::BeginTable( "RenderPass Times", 4, ImGuiTableFlags_Borders ) )
    {
        ImGui::TableSetupColumn( "Render Pass" );
        ImGui::TableSetupColumn( "Avg (ms)" );
        ImGui::TableSetupColumn( "Min (ms)" );
        ImGui::TableSetupColumn( "Max (ms)" );
        ImGui::TableHeadersRow();

        for ( size_t recordIdx = 0; recordIdx < s_profileRecords.size(); ++recordIdx )
        {
            ImGui::TableNextRow();
            const ProfileRecord& record = s_profileRecords[recordIdx];
            ImGui::TableSetColumnIndex( 0 );
            ImGui::Text( "%s", record.name.data() );
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

static void GetOldestFramesResults()
{
    uint32_t CF             = rg.currentFrameIdx;
    PerFrameData& frameData = GetCurrentFrameData();
    uint32_t numQueries     = static_cast<uint32_t>( frameData.queries.size() );
    if ( !numQueries )
        return;

    vkGetQueryPoolResults( rg.device, frameData.queryPool, 0, 2 * numQueries, sizeof( s_cpuQueries ), s_cpuQueries, sizeof( QueryResult ),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );

    for ( uint32_t queryIdx = 0; queryIdx < numQueries; ++queryIdx )
    {
        const QueryRecord& qRecord     = frameData.queries[queryIdx];
        ProfileRecord& pRecord         = s_profileRecords[qRecord.profileEntryIndex];
        ProfileRecordHistory& pHistory = s_profileRecordHistories[qRecord.profileEntryIndex];

        PG_ASSERT( qRecord.endQuery != 0, "Forgot to call StartTimestamp for timestamp '%s'", pRecord.name.data() );
        const QueryResult& startRes = s_cpuQueries[qRecord.startQuery];
        const QueryResult& endRes   = s_cpuQueries[qRecord.endQuery];

        float duration = static_cast<float>( ( endRes.timestamp - startRes.timestamp ) * s_timestampDifferenceToMillis );
        pHistory.frameHistory.Pushback( duration );
        pRecord.avg = 0;
        pRecord.max = 0;
        pRecord.min = FLT_MAX;
        for ( uint16_t i = 0; i < pHistory.frameHistory.Size(); ++i )
        {
            float t     = pHistory.frameHistory[i];
            pRecord.max = Max( pRecord.max, t );
            pRecord.min = Min( pRecord.min, t );
            pRecord.avg += t;
        }
        pRecord.avg /= pHistory.frameHistory.Size();
    }
    frameData.queries.clear();
}

void Reset( const CommandBuffer& cmdbuf )
{
    PerFrameData& frameData = GetCurrentFrameData();
    vkCmdResetQueryPool( cmdbuf, frameData.queryPool, 0, MAX_TIMESTAMPS_PER_FRAME );
    frameData.queries.clear();
}

void StartFrame( const CommandBuffer& cmdbuf )
{
    GetOldestFramesResults();
    Reset( cmdbuf );
}

uint16_t GetOrCreateProfileEntry( const std::string& name )
{
    auto it = s_profileNameToIndexMap.find( name );
    if ( it != s_profileNameToIndexMap.end() )
        return it->second;

    uint16_t index = static_cast<uint16_t>( s_profileRecords.size() );
    s_profileRecordHistories.emplace_back();
    ProfileRecord& pRecord        = s_profileRecords.emplace_back();
    pRecord.name                  = name;
    s_profileNameToIndexMap[name] = index;

    return index;
}

QueryRecord& StartTimestamp( const CommandBuffer& cmdbuf, uint16_t profileEntryIdx )
{
    PerFrameData& frameData = GetCurrentFrameData();

    uint16_t numQueries = static_cast<uint16_t>( frameData.queries.size() );
    PG_ASSERT( numQueries < MAX_RECORDS_PER_FRAME );

    QueryRecord& qRecord      = frameData.queries.emplace_back();
    qRecord.profileEntryIndex = profileEntryIdx;
    qRecord.startQuery        = 2 * numQueries;
    vkCmdWriteTimestamp( cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameData.queryPool, qRecord.startQuery );

    return qRecord;
}

void EndTimestamp( const CommandBuffer& cmdbuf, QueryRecord& qRecord )
{
    PerFrameData& frameData = GetCurrentFrameData();
    qRecord.endQuery        = qRecord.startQuery + 1;
    vkCmdWriteTimestamp( cmdbuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameData.queryPool, qRecord.endQuery );
}

} // namespace PG::Gfx::Profile

#endif // #else // #if !USING( PG_GPU_PROFILING )
