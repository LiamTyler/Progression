#include "core/time.hpp"

using namespace std::chrono;
using Clock     = high_resolution_clock;
using TimePoint = time_point<Clock>;

static TimePoint s_startTime;
static TimePoint s_currentFrameStartTime;
static TimePoint s_lastFrameStartTime;
static f32 s_deltaTime;

namespace PG::Time
{

void Reset()
{
    s_startTime = s_currentFrameStartTime = s_lastFrameStartTime = Clock::now();
    s_deltaTime                                                  = 0;
}

f32 Time() { return static_cast<f32>( GetTimeSince( s_startTime ) ); }

f32 DeltaTime() { return s_deltaTime; }

void StartFrame()
{
    s_currentFrameStartTime = GetTimePoint();
    s_deltaTime             = static_cast<float>( GetElapsedTime( s_lastFrameStartTime, s_currentFrameStartTime ) / 1e3 );
    s_lastFrameStartTime    = s_currentFrameStartTime;
}

Point GetTimePoint() { return Clock::now(); }

f64 GetTimeSince( const Point& point )
{
    auto now = Clock::now();
    return duration_cast<nanoseconds>( now - point ).count() / 1e6;
}

f64 GetElapsedTime( const Point& startPoint, const Point& endPoint )
{
    return duration_cast<nanoseconds>( endPoint - startPoint ).count() / 1e6;
}

} // namespace PG::Time
