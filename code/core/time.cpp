#include "core/time.hpp"

using namespace std::chrono;
using Clock     = high_resolution_clock;
using TimePoint = time_point<Clock>;

static TimePoint s_startTime       = TimePoint();
static f32 s_currentFrameStartTime = 0;
static f32 s_lastFrameStartTime    = 0;
static f32 s_deltaTime             = 0;

namespace PG::Time
{

void Reset()
{
    s_startTime             = Clock::now();
    s_currentFrameStartTime = 0;
    s_deltaTime             = 0;
    s_currentFrameStartTime = 0;
}

f32 Time() { return static_cast<f32>( GetTimeSince( s_startTime ) ); }

f32 DeltaTime() { return s_deltaTime; }

void StartFrame()
{
    s_currentFrameStartTime = Time();
    s_deltaTime             = 0.001f * ( s_currentFrameStartTime - s_lastFrameStartTime );
}

void EndFrame() { s_lastFrameStartTime = s_currentFrameStartTime; }

Point GetTimePoint() { return Clock::now(); }

f64 GetTimeSince( const Point& point )
{
    auto now = Clock::now();
    return duration_cast<microseconds>( now - point ).count() / 1000.0;
}

f64 GetElapsedTime( const Point& startPoint, const Point& endPoint )
{
    return duration_cast<microseconds>( endPoint - startPoint ).count() / 1000.0;
}

} // namespace PG::Time
